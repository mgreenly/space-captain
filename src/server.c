#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "log.h"
#include "message.h"
#include "server.h"
#include "dtls.h"

// Include implementation files
#include "message.c"
#include "dtls.c"

// Server bind addresses
const char *LISTEN_ADDRESSES[] = {
  "127.0.0.1", // localhost
  NULL         // NULL terminator
};

// Client session structure
typedef struct client_session {
  struct sockaddr_in addr;
  socklen_t addr_len;
  dtls_session_t *dtls_session;
  time_t last_activity;
  bool handshake_complete;
  struct client_session *next;
} client_session_t;

// Global variables
static volatile sig_atomic_t g_running = 1;
static client_session_t *g_clients     = NULL;
static dtls_context_t *g_dtls_ctx      = NULL;

// Signal handler for graceful shutdown
static void handle_shutdown(int sig) {
  (void) sig;
  g_running = 0;
}

// Find client session by address
static client_session_t *find_client(const struct sockaddr_in *addr) {
  client_session_t *client = g_clients;
  while (client) {
    if (client->addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
        client->addr.sin_port == addr->sin_port) {
      return client;
    }
    client = client->next;
  }
  return NULL;
}

// Add new client session
static client_session_t *add_client(const struct sockaddr_in *addr, socklen_t addr_len, int sock) {
  client_session_t *client = calloc(1, sizeof(client_session_t));
  if (!client) {
    log_error("%s", "Failed to allocate client session");
    return NULL;
  }

  memcpy(&client->addr, addr, addr_len);
  client->addr_len           = addr_len;
  client->last_activity      = time(NULL);
  client->handshake_complete = false;

  // Create DTLS session
  client->dtls_session =
    sc_dtls_session_create(g_dtls_ctx, sock, (struct sockaddr *) addr, addr_len);
  if (!client->dtls_session) {
    log_error("%s", "Failed to create DTLS session");
    free(client);
    return NULL;
  }

  // Add to list
  client->next = g_clients;
  g_clients    = client;

  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr->sin_addr, addr_str, sizeof(addr_str));
  log_info("New client connected from %s:%d", addr_str, ntohs(addr->sin_port));

  return client;
}

// Remove client session
static void remove_client(client_session_t *client) {
  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client->addr.sin_addr, addr_str, sizeof(addr_str));
  log_info("Client disconnected: %s:%d", addr_str, ntohs(client->addr.sin_port));

  // Remove from list
  client_session_t **pp = &g_clients;
  while (*pp && *pp != client) {
    pp = &(*pp)->next;
  }
  if (*pp) {
    *pp = client->next;
  }

  // Clean up DTLS session
  if (client->dtls_session) {
    sc_dtls_close(client->dtls_session);
    sc_dtls_session_destroy(client->dtls_session);
  }

  free(client);
}

// Check for inactive clients
static void check_client_timeouts(void) {
  time_t now               = time(NULL);
  client_session_t *client = g_clients;
  client_session_t *next;

  while (client) {
    next = client->next;
    if (now - client->last_activity > CLIENT_TIMEOUT_SECONDS) {
      log_warn("%s", "Client timeout - removing inactive client");
      remove_client(client);
    }
    client = next;
  }
}

// Set socket to non-blocking mode
static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Create and bind a UDP socket
static int create_udp_socket(const char *address) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    log_error("Failed to create socket: %s", strerror(errno));
    return -1;
  }

  // Enable address reuse
  int opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    log_error("Failed to set SO_REUSEADDR: %s", strerror(errno));
    close(sock);
    return -1;
  }

  // Set socket buffer sizes
  int bufsize = SOCKET_BUFFER_SIZE;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
    log_warn("Failed to set receive buffer size: %s", strerror(errno));
  }
  if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0) {
    log_warn("Failed to set send buffer size: %s", strerror(errno));
  }

  // Bind to address
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(SERVER_PORT);
  if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
    log_error("Invalid address: %s", address);
    close(sock);
    return -1;
  }

  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    log_error("Failed to bind to %s:%d: %s", address, SERVER_PORT, strerror(errno));
    close(sock);
    return -1;
  }

  // Set non-blocking
  if (set_nonblocking(sock) < 0) {
    log_error("Failed to set non-blocking: %s", strerror(errno));
    close(sock);
    return -1;
  }

  log_info("UDP socket bound to %s:%d", address, SERVER_PORT);
  return sock;
}

int main() {
  log_info("%s", "Space Captain Server starting...");

  // Initialize DTLS
  if (sc_dtls_init() != DTLS_OK) {
    log_error("%s", "Failed to initialize DTLS");
    return 1;
  }

  // Create DTLS context for server
  g_dtls_ctx =
    sc_dtls_context_create(DTLS_ROLE_SERVER, "certs/server.crt", "certs/server.key", NULL, 0);
  if (!g_dtls_ctx) {
    log_error("%s", "Failed to create DTLS context");
    sc_dtls_cleanup();
    return 1;
  }

  // Set up signal handlers
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_shutdown;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
    log_error("Failed to set signal handlers: %s", strerror(errno));
    sc_dtls_context_destroy(g_dtls_ctx);
    sc_dtls_cleanup();
    return 1;
  }

  // Create epoll instance
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    log_error("Failed to create epoll: %s", strerror(errno));
    return 1;
  }

  // Create UDP sockets for all listen addresses
  int num_sockets = 0;
  int sockets[10]; // Max 10 addresses

  for (int i = 0; LISTEN_ADDRESSES[i] != NULL && i < 10; i++) {
    int sock = create_udp_socket(LISTEN_ADDRESSES[i]);
    if (sock < 0) {
      // Clean up already created sockets
      for (int j = 0; j < num_sockets; j++) {
        close(sockets[j]);
      }
      close(epoll_fd);
      return 1;
    }

    // Add socket to epoll
    struct epoll_event ev;
    ev.events  = EPOLLIN | EPOLLET; // Edge-triggered
    ev.data.fd = sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
      log_error("Failed to add socket to epoll: %s", strerror(errno));
      for (int j = 0; j <= num_sockets; j++) {
        close(sockets[j]);
      }
      close(epoll_fd);
      return 1;
    }

    sockets[num_sockets++] = sock;
  }

  if (num_sockets == 0) {
    log_error("%s", "No sockets created");
    close(epoll_fd);
    return 1;
  }

  log_info("Server initialized with %d sockets", num_sockets);

  // Main event loop
  struct epoll_event events[EPOLL_MAX_EVENTS];
  uint8_t buffer[SOCKET_BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t client_len;
  time_t last_timeout_check = time(NULL);

  while (g_running) {
    int nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, 1000); // 1 second timeout

    if (nfds < 0) {
      if (errno == EINTR) {
        continue; // Interrupted by signal
      } else {
        log_error("epoll_wait encountered error: %s", strerror(errno));
        continue; // Log error and continue running instead of breaking
      }
    }

    // Check for client timeouts periodically
    time_t now = time(NULL);
    if (now - last_timeout_check >= 5) { // Check every 5 seconds
      check_client_timeouts();
      last_timeout_check = now;
    }

    // Process events
    for (int i = 0; i < nfds; i++) {
      int sock = events[i].data.fd;

      // Read all available datagrams (edge-triggered mode)
      while (1) {
        client_len = sizeof(client_addr);

        // Peek at the packet to see which client it's from
        ssize_t peek_len = recvfrom(sock, buffer, sizeof(buffer), MSG_PEEK,
                                    (struct sockaddr *) &client_addr, &client_len);

        if (peek_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break; // No more data
          }
          log_error("recvfrom peek failed: %s", strerror(errno));
          break;
        }

        // Find or create client session
        client_session_t *client = find_client(&client_addr);
        if (!client) {
          // New client - create session
          client = add_client(&client_addr, client_len, sock);
          if (!client) {
            // Failed to create session, discard packet
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
            continue;
          }
        }

        // Update last activity
        client->last_activity = time(NULL);

        // Handle DTLS handshake or data
        if (!client->handshake_complete) {
          // Try to complete handshake
          dtls_result_t result = sc_dtls_handshake(client->dtls_session);

          if (result == DTLS_OK) {
            client->handshake_complete = true;
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
            log_info("DTLS handshake completed for %s:%d", addr_str, ntohs(client_addr.sin_port));

            // Consume the packet that triggered handshake completion
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
          } else if (result == DTLS_ERROR_WOULD_BLOCK) {
            // Handshake in progress, consume packet
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
          } else {
            // Handshake failed
            log_error("DTLS handshake failed: %s", sc_dtls_error_string(result));
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
            remove_client(client);
          }
        } else {
          // Handshake complete - read application data
          size_t bytes_read = 0;
          dtls_result_t result =
            sc_dtls_read(client->dtls_session, buffer, sizeof(buffer), &bytes_read);

          if (result == DTLS_OK && bytes_read > 0) {
            // Log received data
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
            log_debug("Received %zu bytes from %s:%d (DTLS)", bytes_read, addr_str,
                      ntohs(client_addr.sin_port));

            // Echo the data back (for now)
            size_t bytes_written = 0;
            result = sc_dtls_write(client->dtls_session, buffer, bytes_read, &bytes_written);
            if (result != DTLS_OK && result != DTLS_ERROR_WOULD_BLOCK) {
              log_error("DTLS write failed: %s", sc_dtls_error_string(result));
              remove_client(client);
            }
          } else if (result == DTLS_ERROR_WOULD_BLOCK) {
            // No data available yet
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
          } else if (result == DTLS_ERROR_PEER_CLOSED) {
            // Client closed connection
            remove_client(client);
          } else {
            // Read error
            log_error("DTLS read failed: %s", sc_dtls_error_string(result));
            recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
            remove_client(client);
          }
        }
      }
    }
  }

  log_info("%s", "Server shutting down...");

  // Clean up all client sessions
  while (g_clients) {
    remove_client(g_clients);
  }

  // Clean up sockets
  for (int i = 0; i < num_sockets; i++) {
    close(sockets[i]);
  }
  close(epoll_fd);

  // Clean up DTLS
  if (g_dtls_ctx) {
    sc_dtls_context_destroy(g_dtls_ctx);
  }
  sc_dtls_cleanup();

  log_info("%s", "Server stopped");
  return 0;
}