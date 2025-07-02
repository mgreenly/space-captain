#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <netinet/tcp.h>

#include "config.h"
#include "message.h"
#include "queue.h"
#include "worker.h"
#include "log.h"

// Include implementation files
#include "queue.c"
#include "worker.c"

// Client buffer structure for handling partial reads
typedef struct client_buffer {
  int fd;
  uint8_t *buffer;
  size_t buffer_size;
  size_t data_len;
  enum { READING_HEADER, READING_BODY } state;
  message_header_t header;
  size_t header_bytes_read;
  struct client_buffer *next;
  int in_use; // Flag to indicate if this buffer is currently in use
} client_buffer_t;

// Connection pool structure
typedef struct {
  client_buffer_t *buffers;   // Array of pre-allocated buffers
  client_buffer_t *free_list; // Head of free list
  size_t pool_size;           // Total size of the pool
  size_t used_count;          // Number of buffers currently in use
} connection_pool_t;

// Global shutdown flag
static volatile sig_atomic_t shutdown_flag = 0;

// Client buffer management
static client_buffer_t *client_buffers = NULL;

// Connection pool
static connection_pool_t conn_pool = {0};

// Signal handler
static void signal_handler(int sig) {
  log_info("Received signal %d", sig);
  shutdown_flag = 1;
}

// Initialize connection pool
static int init_connection_pool(size_t size) {
  conn_pool.buffers = calloc(size, sizeof(client_buffer_t));
  if (!conn_pool.buffers) {
    log_error("Failed to allocate connection pool of size %zu", size);
    return -1;
  }

  conn_pool.pool_size = size;
  conn_pool.used_count = 0;

  // Initialize free list - link all buffers together
  for (size_t i = 0; i < size - 1; i++) {
    conn_pool.buffers[i].next = &conn_pool.buffers[i + 1];
    conn_pool.buffers[i].fd = -1;
    conn_pool.buffers[i].in_use = 0;
  }
  conn_pool.buffers[size - 1].next = NULL;
  conn_pool.buffers[size - 1].fd = -1;
  conn_pool.buffers[size - 1].in_use = 0;

  conn_pool.free_list = &conn_pool.buffers[0];

  log_info("Initialized connection pool with %zu pre-allocated buffers", size);
  return 0;
}

// Get a buffer from the pool
static client_buffer_t *pool_get_buffer(void) {
  if (!conn_pool.free_list) {
    // Pool exhausted - could implement dynamic growth here
    log_warn("Connection pool exhausted (size: %zu)", conn_pool.pool_size);

    // Fallback to malloc for now
    client_buffer_t *buf = calloc(1, sizeof(client_buffer_t));
    if (buf) {
      buf->fd = -1;
      buf->in_use = 1;
    }
    return buf;
  }

  // Get buffer from free list
  client_buffer_t *buf = conn_pool.free_list;
  conn_pool.free_list = buf->next;

  // Reset buffer state
  buf->fd = -1;
  buf->buffer = NULL;
  buf->buffer_size = 0;
  buf->data_len = 0;
  buf->state = READING_HEADER;
  buf->header_bytes_read = 0;
  memset(&buf->header, 0, sizeof(buf->header));
  buf->next = NULL;
  buf->in_use = 1;

  conn_pool.used_count++;
  return buf;
}

// Return a buffer to the pool
static void pool_return_buffer(client_buffer_t *buf) {
  if (!buf)
    return;

  // Free any allocated message buffer
  if (buf->buffer) {
    free(buf->buffer);
    buf->buffer = NULL;
  }

  // Check if this buffer is from the pool
  if (buf >= conn_pool.buffers && buf < conn_pool.buffers + conn_pool.pool_size) {
    // Return to free list
    buf->in_use = 0;
    buf->fd = -1;
    buf->next = conn_pool.free_list;
    conn_pool.free_list = buf;
    conn_pool.used_count--;
  } else {
    // This was a dynamically allocated buffer
    free(buf);
  }
}

// Cleanup connection pool
static void cleanup_connection_pool(void) {
  if (conn_pool.buffers) {
    // Free any remaining allocated message buffers
    for (size_t i = 0; i < conn_pool.pool_size; i++) {
      if (conn_pool.buffers[i].buffer) {
        free(conn_pool.buffers[i].buffer);
      }
    }
    free(conn_pool.buffers);
    conn_pool.buffers = NULL;
  }
  conn_pool.pool_size = 0;
  conn_pool.used_count = 0;
  conn_pool.free_list = NULL;
}

// Set socket to non-blocking
static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    log_error("fcntl F_GETFL failed: %s", strerror(errno));
    return -1;
  }

  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    log_error("fcntl F_SETFL failed: %s", strerror(errno));
    return -1;
  }

  return 0;
}

// Create and bind server socket
static int create_server_socket(void) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    log_error("Failed to create socket: %s", strerror(errno));
    return -1;
  }

  // Allow socket reuse
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    log_error("setsockopt failed: %s", strerror(errno));
    close(server_fd);
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    log_error("Bind failed: %s", strerror(errno));
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, SERVER_BACKLOG) < 0) {
    log_error("Listen failed: %s", strerror(errno));
    close(server_fd);
    return -1;
  }

  return server_fd;
}

// Handle new connection
static void handle_new_connection(int server_fd, int epoll_fd) {
  // With edge-triggered mode, we must accept all pending connections
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more pending connections
        break;
      } else {
        log_error("Accept failed: %s", strerror(errno));
        break;
      }
    }

    // Set non-blocking
    if (set_nonblocking(client_fd) < 0) {
      close(client_fd);
      continue;
    }

    // Enable TCP_NODELAY to reduce latency for small messages
    int flag = 1;
    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
      log_warn("Failed to set TCP_NODELAY: %s", strerror(errno));
      // Non-fatal, continue anyway
    }

    // Add to epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // Edge-triggered mode with RDHUP
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
      log_error("epoll_ctl failed: %s", strerror(errno));
      close(client_fd);
      continue;
    }

    log_info("New connection from %s:%d (fd=%d)", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port), client_fd);
  }
}

// Find or create client buffer
static client_buffer_t *get_client_buffer(int client_fd) {
  client_buffer_t *buf = client_buffers;
  while (buf) {
    if (buf->fd == client_fd) {
      return buf;
    }
    buf = buf->next;
  }

  // Get new buffer from pool
  buf = pool_get_buffer();
  if (!buf) {
    log_error("%s", "Failed to get client buffer from pool");
    return NULL;
  }

  buf->fd = client_fd;
  buf->state = READING_HEADER;
  buf->next = client_buffers;
  client_buffers = buf;

  return buf;
}

// Remove client buffer
static void remove_client_buffer(int client_fd) {
  client_buffer_t **prev = &client_buffers;
  client_buffer_t *buf = client_buffers;

  while (buf) {
    if (buf->fd == client_fd) {
      *prev = buf->next;
      pool_return_buffer(buf);
      return;
    }
    prev = &buf->next;
    buf = buf->next;
  }
}

// Read message from client
// Returns: 1 on success, 0 if no data available, -1 on error/close
static int read_message(int client_fd, message_t **msg) {
  client_buffer_t *buf = get_client_buffer(client_fd);
  if (!buf) {
    return -1;
  }

  *msg = NULL;

  // Read header if needed
  if (buf->state == READING_HEADER) {
    size_t remaining = sizeof(message_header_t) - buf->header_bytes_read;
    ssize_t n = recv(client_fd, ((char *) &buf->header) + buf->header_bytes_read, remaining, 0);

    if (n == 0) {
      return -1; // Connection closed
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0; // No data available yet
      }
      log_error("Failed to read header: %s", strerror(errno));
      return -1;
    }

    buf->header_bytes_read += n;

    if (buf->header_bytes_read == sizeof(message_header_t)) {
      // Header complete, validate and prepare for body
      if (buf->header.length == 0 || buf->header.length > MAX_MESSAGE_SIZE) {
        log_error("Invalid message length: %u", buf->header.length);
        return -1;
      }

      // Allocate buffer for body
      buf->buffer_size = buf->header.length;
      buf->buffer = malloc(buf->buffer_size);
      if (!buf->buffer) {
        log_error("%s", "Failed to allocate buffer");
        return -1;
      }

      buf->data_len = 0;
      buf->state = READING_BODY;
    } else {
      return 0; // Still reading header
    }
  }

  // Read body if in READING_BODY state
  if (buf->state == READING_BODY) {
    size_t remaining = buf->buffer_size - buf->data_len;
    ssize_t n = recv(client_fd, buf->buffer + buf->data_len, remaining, 0);

    if (n == 0) {
      log_error("%s", "Connection closed while reading body");
      return -1;
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0; // No data available yet
      }
      log_error("Failed to read body: %s", strerror(errno));
      return -1;
    }

    buf->data_len += n;

    if (buf->data_len == buf->buffer_size) {
      // Message complete, create message structure
      *msg = malloc(sizeof(message_t));
      if (!*msg) {
        log_error("%s", "Failed to allocate message");
        return -1;
      }

      // Allocate body with extra space for client_fd
      (*msg)->body = malloc(CLIENT_FD_SIZE + buf->header.length);
      if (!(*msg)->body) {
        log_error("%s", "Failed to allocate message body");
        free(*msg);
        return -1;
      }

      // Store client_fd at beginning of body
      *(int32_t *) ((*msg)->body) = client_fd;

      // Copy message data
      memcpy((*msg)->body + CLIENT_FD_SIZE, buf->buffer, buf->header.length);

      // Set message fields
      (*msg)->header = buf->header;

      // Reset buffer for next message
      free(buf->buffer);
      buf->buffer = NULL;
      buf->buffer_size = 0;
      buf->data_len = 0;
      buf->header_bytes_read = 0;
      buf->state = READING_HEADER;

      return 1; // Message complete
    }
  }

  return 0; // Still reading
}

// Handle client data
static void handle_client_data(int client_fd, queue_t *msg_queue) {
  // With edge-triggered mode, we must read all available data
  while (1) {
    message_t *msg = NULL;
    int result = read_message(client_fd, &msg);

    if (result < 0) {
      // Error or connection closed
      log_info("Client disconnected (fd=%d)", client_fd);
      close(client_fd);
      // Note: epoll automatically removes closed fds
      break;
    } else if (result > 0) {
      // Successfully read message
      const char *type_str = message_type_to_string(msg->header.type);

      // Extract the actual message content (skip the client_fd prefix)
      char *actual_body = msg->body + CLIENT_FD_SIZE;

      log_info("Received request [Type: %s] from fd=%d: %s", type_str, client_fd, actual_body);
      log_debug("Request details - Type: %d (%s), Length: %u bytes, Client: fd=%d",
                msg->header.type, type_str, msg->header.length, client_fd);

      // Queue message for processing
      sc_queue_ret_val_t result = sc_queue_add(msg_queue, msg);
      if (result != QUEUE_SUCCESS) {
        log_error("%s", "Failed to queue message");
        free(msg->body);
        free(msg);
      }
      // Continue reading more messages
    } else if (result == 0) {
      // Partial read, will continue on next epoll event
      break;
    } else {
      // Error occurred
      log_error("Error reading from client (fd=%d)", client_fd);
      remove_client_buffer(client_fd);
      close(client_fd);
      break;
    }
  }
}

int main(void) {
  log_info("Server starting on port %d", SERVER_PORT);

  // Set up signal handlers
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
    log_error("%s", "Failed to set signal handlers");
    return EXIT_FAILURE;
  }

  // Initialize connection pool
  if (init_connection_pool(CONNECTION_POOL_SIZE) < 0) {
    log_error("%s", "Failed to initialize connection pool");
    return EXIT_FAILURE;
  }

  // Create message queue
  queue_t *msg_queue = sc_queue_init(QUEUE_CAPACITY);
  if (!msg_queue) {
    log_error("%s", "Failed to create message queue");
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }

  // Create worker pool
  sc_worker_pool_t *worker_pool = sc_worker_pool_init(WORKER_POOL_SIZE, msg_queue);
  if (!worker_pool) {
    log_error("%s", "Failed to create worker pool");
    sc_queue_nuke(msg_queue);
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }

  // Start workers
  sc_worker_pool_start(worker_pool);

  // Create server socket
  int server_fd = create_server_socket();
  if (server_fd < 0) {
    sc_worker_pool_stop(worker_pool);
    sc_worker_pool_nuke(worker_pool);
    sc_queue_nuke(msg_queue);
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }

  // Set non-blocking
  if (set_nonblocking(server_fd) < 0) {
    close(server_fd);
    sc_worker_pool_stop(worker_pool);
    sc_worker_pool_nuke(worker_pool);
    sc_queue_nuke(msg_queue);
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }

  // Create epoll instance
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    log_error("epoll_create1 failed: %s", strerror(errno));
    close(server_fd);
    sc_worker_pool_stop(worker_pool);
    sc_worker_pool_nuke(worker_pool);
    sc_queue_nuke(msg_queue);
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }

  // Add server socket to epoll
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // Edge-triggered mode with RDHUP
  event.data.fd = server_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
    log_error("epoll_ctl failed: %s", strerror(errno));
    close(epoll_fd);
    close(server_fd);
    sc_worker_pool_stop(worker_pool);
    sc_worker_pool_nuke(worker_pool);
    sc_queue_nuke(msg_queue);
    return EXIT_FAILURE;
  }

  log_info("Server listening on %s:%d", SERVER_HOST, SERVER_PORT);

  // Calculate event buffer size based on connection pool
  int event_buffer_size = MIN_EVENTS;
  if (CONNECTION_POOL_SIZE * MAX_EVENTS_PER_CONN > event_buffer_size) {
    event_buffer_size = CONNECTION_POOL_SIZE * MAX_EVENTS_PER_CONN;
    if (event_buffer_size > ABSOLUTE_MAX_EVENTS) {
      event_buffer_size = ABSOLUTE_MAX_EVENTS;
    }
  }

  // Allocate dynamic event buffer
  struct epoll_event *events = malloc(sizeof(struct epoll_event) * event_buffer_size);
  if (!events) {
    log_error("Failed to allocate event buffer of size %d", event_buffer_size);
    close(epoll_fd);
    close(server_fd);
    sc_worker_pool_stop(worker_pool);
    sc_worker_pool_nuke(worker_pool);
    sc_queue_nuke(msg_queue);
    cleanup_connection_pool();
    return EXIT_FAILURE;
  }
  log_info("Allocated event buffer for %d events", event_buffer_size);

  int consecutive_empty_polls = 0;
  const int max_empty_polls = 10; // After 10 empty polls, use blocking timeout

  // Main event loop
  while (!shutdown_flag) {
    int total_events_processed = 0;
    int events_in_batch = 0;

    // Keep processing events until no more are available
    do {
      // Use adaptive timeout: 0 (non-blocking) when active, EPOLL_TIMEOUT_MS when idle
      int timeout = (consecutive_empty_polls < max_empty_polls) ? 0 : EPOLL_TIMEOUT_MS;
      int n = epoll_wait(epoll_fd, events, event_buffer_size, timeout);

      if (n < 0) {
        if (errno == EINTR) {
          // Check shutdown flag immediately on interrupt
          if (shutdown_flag)
            break;
          continue;
        }
        log_error("epoll_wait failed: %s", strerror(errno));
        break;
      }

      // Update consecutive empty polls counter
      if (n == 0) {
        consecutive_empty_polls++;
      } else {
        consecutive_empty_polls = 0; // Reset on activity
        total_events_processed += n;
      }

      events_in_batch = n;

      for (int i = 0; i < n && !shutdown_flag; i++) {
        if (events[i].data.fd == server_fd) {
          // New connection
          handle_new_connection(server_fd, epoll_fd);
        } else {
          // Check for EPOLLRDHUP (peer shutdown)
          if (events[i].events & EPOLLRDHUP) {
            log_info("Client disconnected (EPOLLRDHUP) (fd=%d)", events[i].data.fd);
            remove_client_buffer(events[i].data.fd);
            close(events[i].data.fd);
          } else if (events[i].events & EPOLLIN) {
            // Client data available
            handle_client_data(events[i].data.fd, msg_queue);
            // Note: handle_client_data will close the fd if needed
          }
        }
      }

      // Continue if we filled the event buffer (might be more events)
    } while (events_in_batch == event_buffer_size && !shutdown_flag);

    // Log if we're processing lots of events
    if (total_events_processed > event_buffer_size * 2) {
      log_info("Processed %d events in single loop iteration", total_events_processed);
    }
  }

  log_info("%s", "Server shutting down");

  // Cleanup
  close(epoll_fd);
  close(server_fd);

  // Stop workers
  sc_worker_pool_stop(worker_pool);
  sc_worker_pool_nuke(worker_pool);

  // Clean up remaining messages
  message_t *msg;
  sc_queue_ret_val_t pop_result;
  while ((pop_result = sc_queue_try_pop(msg_queue, &msg)) == QUEUE_SUCCESS) {
    free(msg->body);
    free(msg);
  }
  sc_queue_nuke(msg_queue);

  // Clean up all client buffers
  while (client_buffers) {
    client_buffer_t *next = client_buffers->next;
    pool_return_buffer(client_buffers);
    client_buffers = next;
  }

  // Clean up connection pool
  cleanup_connection_pool();

  // Clean up event buffer
  free(events);

  log_info("%s", "Server shutdown complete");
  return EXIT_SUCCESS;
}
