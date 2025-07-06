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

#include "config.h"
#include "log.h"
#include "server.h"

// Global flag for graceful shutdown
static volatile sig_atomic_t g_running = 1;

// Signal handler for graceful shutdown
static void handle_shutdown(int sig) {
  (void) sig;
  g_running = 0;
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

  // Set up signal handlers
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_shutdown;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
    log_error("Failed to set signal handlers: %s", strerror(errno));
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
  char buffer[SOCKET_BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t client_len;

  while (g_running) {
    int nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, 1000); // 1 second timeout

    if (nfds < 0) {
      if (errno == EINTR) {
        continue; // Interrupted by signal
      }
      log_error("epoll_wait failed: %s", strerror(errno));
      break;
    }

    // Process events
    for (int i = 0; i < nfds; i++) {
      int sock = events[i].data.fd;

      // Read all available datagrams (edge-triggered mode)
      while (1) {
        client_len = sizeof(client_addr);
        ssize_t len =
          recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &client_addr, &client_len);

        if (len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break; // No more data
          }
          log_error("recvfrom failed: %s", strerror(errno));
          break;
        }

        // Log received packet
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
        log_debug("Received %zd bytes from %s:%d", len, addr_str, ntohs(client_addr.sin_port));

        // Echo the packet back for now
        ssize_t sent = sendto(sock, buffer, len, 0, (struct sockaddr *) &client_addr, client_len);
        if (sent < 0) {
          log_error("sendto failed: %s", strerror(errno));
        }
      }
    }
  }

  log_info("%s", "Server shutting down...");

  // Clean up
  for (int i = 0; i < num_sockets; i++) {
    close(sockets[i]);
  }
  close(epoll_fd);

  log_info("%s", "Server stopped");
  return 0;
}