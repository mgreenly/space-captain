#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/tcp.h>

#include "config.h"
#include "message.h"
#include "log.h"

// Global shutdown flag
static volatile sig_atomic_t shutdown_flag = 0;

// Signal handler
static void signal_handler(int sig) {
  log_info("Received signal %d", sig);
  shutdown_flag = 1;
}

// Connect to server
static int connect_to_server(void) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    log_error("Failed to create socket: %s", strerror(errno));
    return -1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons(SERVER_PORT);

  if (inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr) <= 0) {
    log_error("Invalid address: %s", SERVER_HOST);
    close(sock);
    return -1;
  }

  if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    log_error("Connection failed: %s", strerror(errno));
    close(sock);
    return -1;
  }

  // Enable TCP_NODELAY to reduce latency for small messages
  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
    log_warn("Failed to set TCP_NODELAY: %s", strerror(errno));
    // Non-fatal, continue anyway
  }

  log_info("Connected to server %s:%d", SERVER_HOST, SERVER_PORT);
  return sock;
}

// Generate random message
static char *generate_random_message(message_type_t type) {
  static const char *echo_messages[] = {"Hello, server!", "This is a test message",
                                        "Echo this back to me", "Testing the echo functionality",
                                        "Space Captain client here"};

  static const char *reverse_messages[] = {"Reverse this string", "!gnirts siht esreveR",
                                           "The quick brown fox", "Testing reverse functionality",
                                           "Space Captain"};

  static const char *time_messages[] = {"What time is it?", "Current time please", "Time check",
                                        "Requesting timestamp", "Time query"};

  const char *message = NULL;
  int count           = 0;

  switch (type) {
  case MSG_ECHO:
    count   = sizeof(echo_messages) / sizeof(echo_messages[0]);
    message = echo_messages[rand() % count];
    break;
  case MSG_REVERSE:
    count   = sizeof(reverse_messages) / sizeof(reverse_messages[0]);
    message = reverse_messages[rand() % count];
    break;
  case MSG_TIME:
    count   = sizeof(time_messages) / sizeof(time_messages[0]);
    message = time_messages[rand() % count];
    break;
  default:
    message = "Invalid message type";
  }

  return strdup(message);
}

// Send message to server
static int send_message(int sock, message_type_t type, const char *body) {
  message_header_t header = {.type = type, .length = strlen(body) + 1};

  // Send header
  if (send(sock, &header, sizeof(header), 0) != sizeof(header)) {
    log_error("Failed to send header: %s", strerror(errno));
    return -1;
  }

  // Send body
  if (send(sock, body, header.length, 0) != (ssize_t) header.length) {
    log_error("Failed to send body: %s", strerror(errno));
    return -1;
  }

  const char *type_str = message_type_to_string(type);

  log_info("Sending request [Type: %s]: %s", type_str, body);
  log_debug("Request details - Type: %d (%s), Length: %u bytes", type, type_str, header.length);
  return 0;
}

// Receive response from server
static int receive_response(int sock) {
  message_header_t header;

  // Receive header
  ssize_t n = recv(sock, &header, sizeof(header), MSG_WAITALL);
  if (n != sizeof(header)) {
    if (n == 0) {
      log_info("%s", "Server closed connection");
      return -1;
    }
    log_error("Failed to receive header: %s", strerror(errno));
    return -1;
  }

  // Validate header
  if (header.length == 0 || header.length > MAX_MESSAGE_SIZE) {
    log_error("Invalid response length: %u", header.length);
    return -1;
  }

  // Receive body
  char *body = malloc(header.length);
  if (!body) {
    log_error("%s", "Failed to allocate response buffer");
    return -1;
  }

  n = recv(sock, body, header.length, MSG_WAITALL);
  if (n != (ssize_t) header.length) {
    log_error("Failed to receive body: expected %u, got %zd", header.length, n);
    free(body);
    return -1;
  }

  // Pretty print the response based on message type
  const char *type_str = message_type_to_string(header.type);

  log_info("Received response [Type: %s]: %s", type_str, body);
  log_debug("Response details - Type: %d (%s), Length: %u bytes", header.type, type_str,
            header.length);

  free(body);
  return 0;
}

// Random delay between messages
static void random_delay(void) {
  int delay_ms = MIN_DELAY_MS + (rand() % (MAX_DELAY_MS - MIN_DELAY_MS + 1));
  log_debug("Sleeping for %d ms (%.2f seconds)", delay_ms, delay_ms / 1000.0);
  usleep(delay_ms * 1000);
}

int main(void) {
  log_info("%s", "Client starting");

  // Seed random number generator
  srand(time(NULL));

  // Set up signal handlers
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
    log_error("%s", "Failed to set signal handlers");
    return EXIT_FAILURE;
  }

  // Connect to server
  int sock = connect_to_server();
  if (sock < 0) {
    return EXIT_FAILURE;
  }

  // Message types
  message_type_t message_types[] = {MSG_ECHO, MSG_REVERSE, MSG_TIME};
  int num_types                  = sizeof(message_types) / sizeof(message_types[0]);

  // Main loop
  while (!shutdown_flag) {
    // Random delay
    random_delay();

    if (shutdown_flag)
      break;

    // Select random message type
    message_type_t type = message_types[rand() % num_types];

    // Generate message
    char *body = generate_random_message(type);
    if (!body) {
      log_error("%s", "Failed to generate message");
      continue;
    }

    // Send message
    if (send_message(sock, type, body) < 0) {
      free(body);
      break;
    }

    // Receive response
    if (receive_response(sock) < 0) {
      free(body);
      break;
    }

    free(body);
  }

  log_info("%s", "Client shutting down");
  close(sock);
  log_info("%s", "Client shutdown complete");

  return EXIT_SUCCESS;
}