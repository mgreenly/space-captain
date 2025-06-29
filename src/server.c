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

#include "config.h"
#include "message.h"
#include "queue.h"
#include "worker.h"
#include "log.h"

// Global shutdown flag
static volatile sig_atomic_t shutdown_flag = 0;

// Signal handler
static void signal_handler(int sig) {
  log_info("Received signal %d", sig);
  shutdown_flag = 1;
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

    // Add to epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered mode
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

// Read message from client
// Returns: 1 on success, 0 if no data available, -1 on error/close
static int read_message(int client_fd, message_t **msg) {
  message_header_t header;
  size_t total_read = 0;

  // Read header - handle partial reads
  while (total_read < sizeof(header)) {
    ssize_t n = recv(client_fd, ((char *) &header) + total_read, sizeof(header) - total_read, 0);
    if (n == 0) {
      return -1; // Connection closed
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (total_read == 0) {
          return 0; // No data available yet
        }
        // Partial header read, continue
        usleep(READ_RETRY_DELAY_US); // Small delay before retry
        continue;
      } else {
        log_error("Failed to read header: %s", strerror(errno));
        return -1;
      }
    }
    total_read += n;
  }

  // Validate header
  if (header.length == 0 || header.length > MAX_MESSAGE_SIZE) {
    log_error("Invalid message length: %u", header.length);
    return -1;
  }

  // Allocate message
  *msg = malloc(sizeof(message_t));
  if (!*msg) {
    log_error("%s", "Failed to allocate message");
    return -1;
  }

  // Allocate body with extra space for client_fd
  (*msg)->body = malloc(CLIENT_FD_SIZE + header.length);
  if (!(*msg)->body) {
    log_error("%s", "Failed to allocate message body");
    free(*msg);
    return -1;
  }

  // Store client_fd at beginning of body
  *(int32_t *) ((*msg)->body) = client_fd;

  // Read body - handle partial reads
  total_read = 0;
  while (total_read < header.length) {
    ssize_t n =
      recv(client_fd, (*msg)->body + CLIENT_FD_SIZE + total_read, header.length - total_read, 0);
    if (n == 0) {
      log_error("%s", "Connection closed while reading body");
      free((*msg)->body);
      free(*msg);
      return -1;
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Data not ready, continue
        usleep(READ_RETRY_DELAY_US); // Small delay before retry
        continue;
      } else {
        log_error("Failed to read body: %s", strerror(errno));
        free((*msg)->body);
        free(*msg);
        return -1;
      }
    }
    total_read += n;
  }

  (*msg)->header = header;
  return 1; // Success
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
      if (queue_add(msg_queue, msg) != QUEUE_SUCCESS) {
        log_error("%s", "Failed to queue message");
        free(msg->body);
        free(msg);
      }
      // Continue reading more messages
    } else {
      // result == 0 means no more data available (EAGAIN)
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

  // Create message queue
  queue_t *msg_queue = queue_create(QUEUE_CAPACITY);
  if (!msg_queue) {
    log_error("%s", "Failed to create message queue");
    return EXIT_FAILURE;
  }

  // Create worker pool
  worker_pool_t *worker_pool = worker_pool_create(WORKER_POOL_SIZE, msg_queue);
  if (!worker_pool) {
    log_error("%s", "Failed to create worker pool");
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  // Start workers
  worker_pool_start(worker_pool);

  // Create server socket
  int server_fd = create_server_socket();
  if (server_fd < 0) {
    worker_pool_stop(worker_pool);
    worker_pool_destroy(worker_pool);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  // Set non-blocking
  if (set_nonblocking(server_fd) < 0) {
    close(server_fd);
    worker_pool_stop(worker_pool);
    worker_pool_destroy(worker_pool);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  // Create epoll instance
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    log_error("epoll_create1 failed: %s", strerror(errno));
    close(server_fd);
    worker_pool_stop(worker_pool);
    worker_pool_destroy(worker_pool);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  // Add server socket to epoll
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET; // Edge-triggered mode
  event.data.fd = server_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
    log_error("epoll_ctl failed: %s", strerror(errno));
    close(epoll_fd);
    close(server_fd);
    worker_pool_stop(worker_pool);
    worker_pool_destroy(worker_pool);
    queue_destroy(msg_queue);
    return EXIT_FAILURE;
  }

  log_info("Server listening on %s:%d", SERVER_HOST, SERVER_PORT);

  // Event buffer
  struct epoll_event events[MAX_EVENTS];

  // Main event loop
  while (!shutdown_flag) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);

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

    for (int i = 0; i < n && !shutdown_flag; i++) {
      if (events[i].data.fd == server_fd) {
        // New connection
        handle_new_connection(server_fd, epoll_fd);
      } else {
        // Client data
        handle_client_data(events[i].data.fd, msg_queue);
        // Note: handle_client_data will close the fd if needed
        // We don't remove from epoll here to allow multiple messages
      }
    }
  }

  log_info("%s", "Server shutting down");

  // Cleanup
  close(epoll_fd);
  close(server_fd);

  // Stop workers
  worker_pool_stop(worker_pool);
  worker_pool_destroy(worker_pool);

  // Clean up remaining messages
  message_t *msg;
  while (queue_try_pop(msg_queue, &msg) == QUEUE_SUCCESS) {
    free(msg->body);
    free(msg);
  }
  queue_destroy(msg_queue);

  log_info("%s", "Server shutdown complete");
  return EXIT_SUCCESS;
}