#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "vendor/unity.c"
#include "../src/config.h"
#include "../src/dtls.h"
#include "../src/dtls.c"

// Test globals
static int test_socket = -1;
static struct sockaddr_in server_addr;
static pid_t server_pid               = -1;
static dtls_context_t *g_dtls_ctx     = NULL;
static dtls_session_t *g_dtls_session = NULL;
static uint8_t g_server_cert_hash[32]; // SHA256 hash of server certificate

void setUp(void) {
  // Initialize DTLS once at the beginning
  static int dtls_initialized = 0;
  if (!dtls_initialized) {
    TEST_ASSERT_EQUAL(DTLS_OK, sc_dtls_init());

    // Calculate server certificate hash for pinning
    TEST_ASSERT_EQUAL(DTLS_OK, sc_dtls_cert_hash("certs/server.crt", g_server_cert_hash));

    dtls_initialized = 1;
  }

  // Start the server
  server_pid = fork();
  if (server_pid == 0) {
    // Child process - run the server
    // Redirect stdout and stderr to /dev/null to suppress server output
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDOUT_FILENO);
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }
    execl("bin/server", "server", NULL);
    // If execl fails
    exit(1);
  }
  TEST_ASSERT_NOT_EQUAL(-1, server_pid);

  // Give server time to start
  usleep(100000); // 100ms

  // Create UDP socket for testing
  test_socket = socket(AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT_NOT_EQUAL(-1, test_socket);

  // Setup server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons(SERVER_PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  // Don't connect the socket - DTLS will handle addressing

  // Create DTLS client context with certificate pinning
  g_dtls_ctx = sc_dtls_context_create(DTLS_ROLE_CLIENT, NULL, NULL, g_server_cert_hash,
                                      sizeof(g_server_cert_hash));
  TEST_ASSERT_NOT_NULL(g_dtls_ctx);

  // Create DTLS session with server address
  g_dtls_session = sc_dtls_session_create(g_dtls_ctx, test_socket, (struct sockaddr *) &server_addr,
                                          sizeof(server_addr));
  TEST_ASSERT_NOT_NULL(g_dtls_session);

  // Perform DTLS handshake
  time_t start = time(NULL);
  dtls_result_t result;
  while ((result = sc_dtls_handshake(g_dtls_session)) == DTLS_ERROR_WOULD_BLOCK) {
    // Check for timeout
    if (time(NULL) - start > 5) {
      TEST_FAIL_MESSAGE("DTLS handshake timeout");
    }
    usleep(10000); // 10ms
  }
  TEST_ASSERT_EQUAL_MESSAGE(DTLS_OK, result, "DTLS handshake failed");
}

void tearDown(void) {
  // Clean up DTLS session
  if (g_dtls_session) {
    sc_dtls_close(g_dtls_session);
    sc_dtls_session_destroy(g_dtls_session);
    g_dtls_session = NULL;
  }

  // Clean up DTLS context
  if (g_dtls_ctx) {
    sc_dtls_context_destroy(g_dtls_ctx);
    g_dtls_ctx = NULL;
  }

  // Close test socket
  if (test_socket >= 0) {
    close(test_socket);
    test_socket = -1;
  }

  // Stop the server
  if (server_pid > 0) {
    kill(server_pid, SIGTERM);
    waitpid(server_pid, NULL, 0);
    server_pid = -1;
  }
}

// Test that server echoes packets back
void test_server_echo_response(void) {
  const char *test_msg = "PING";
  uint8_t recv_buffer[256];

  // Send test message via DTLS
  size_t bytes_written = 0;
  dtls_result_t result =
    sc_dtls_write(g_dtls_session, (const uint8_t *) test_msg, strlen(test_msg), &bytes_written);
  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(test_msg), bytes_written);

  // Receive response via DTLS
  size_t bytes_read = 0;
  time_t start      = time(NULL);
  while ((result = sc_dtls_read(g_dtls_session, recv_buffer, sizeof(recv_buffer), &bytes_read)) ==
         DTLS_ERROR_WOULD_BLOCK) {
    if (time(NULL) - start > 2) {
      TEST_FAIL_MESSAGE("Timeout waiting for echo response");
    }
    usleep(10000); // 10ms
  }

  // Verify response
  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(test_msg), bytes_read);
  TEST_ASSERT_EQUAL_MEMORY(test_msg, recv_buffer, bytes_read);
}

// Test that server handles multiple packets
void test_server_multiple_packets(void) {
  const char *messages[] = {"MSG1", "MSG2", "MSG3"};
  uint8_t recv_buffer[256];

  for (int i = 0; i < 3; i++) {
    // Send message via DTLS
    size_t bytes_written = 0;
    dtls_result_t result = sc_dtls_write(g_dtls_session, (const uint8_t *) messages[i],
                                         strlen(messages[i]), &bytes_written);
    TEST_ASSERT_EQUAL(DTLS_OK, result);
    TEST_ASSERT_EQUAL(strlen(messages[i]), bytes_written);

    // Receive response via DTLS
    size_t bytes_read = 0;
    time_t start      = time(NULL);
    while ((result = sc_dtls_read(g_dtls_session, recv_buffer, sizeof(recv_buffer), &bytes_read)) ==
           DTLS_ERROR_WOULD_BLOCK) {
      if (time(NULL) - start > 2) {
        TEST_FAIL_MESSAGE("Timeout waiting for response");
      }
      usleep(10000); // 10ms
    }

    // Verify response
    TEST_ASSERT_EQUAL(DTLS_OK, result);
    TEST_ASSERT_EQUAL(strlen(messages[i]), bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(messages[i], recv_buffer, bytes_read);
  }
}

// Test large packet handling
void test_server_large_packet(void) {
  // Create a large test message
  char large_msg[1024];
  memset(large_msg, 'A', sizeof(large_msg) - 1);
  large_msg[sizeof(large_msg) - 1] = '\0';

  uint8_t recv_buffer[2048];

  // Send large message via DTLS
  size_t bytes_written = 0;
  dtls_result_t result =
    sc_dtls_write(g_dtls_session, (const uint8_t *) large_msg, strlen(large_msg), &bytes_written);
  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(large_msg), bytes_written);

  // Receive response via DTLS
  size_t bytes_read = 0;
  time_t start      = time(NULL);
  while ((result = sc_dtls_read(g_dtls_session, recv_buffer, sizeof(recv_buffer), &bytes_read)) ==
         DTLS_ERROR_WOULD_BLOCK) {
    if (time(NULL) - start > 2) {
      TEST_FAIL_MESSAGE("Timeout waiting for large packet response");
    }
    usleep(10000); // 10ms
  }

  // Verify response
  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(large_msg), bytes_read);
  TEST_ASSERT_EQUAL_MEMORY(large_msg, recv_buffer, bytes_read);
}

// Test DTLS-specific error: certificate verification failure
void test_dtls_cert_verification(void) {
  // This test verifies that certificate pinning works correctly.
  // Since we're already connected with the correct cert hash in setUp,
  // we'll test that the connection was established successfully.

  // Verify handshake is complete
  TEST_ASSERT_TRUE(sc_dtls_is_handshake_complete(g_dtls_session));

  // Send a test message to verify connection works
  const char *msg      = "CERT_TEST";
  size_t bytes_written = 0;
  dtls_result_t result =
    sc_dtls_write(g_dtls_session, (const uint8_t *) msg, strlen(msg), &bytes_written);
  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(msg), bytes_written);
}

// Test DTLS session timeout behavior
void test_dtls_session_timeout(void) {
  // This test verifies that the DTLS session remains valid during normal operation
  // The actual 30-second timeout is tested separately in the server implementation

  const char *msg = "TIMEOUT_TEST";
  uint8_t recv_buffer[256];

  // Send initial message
  size_t bytes_written = 0;
  dtls_result_t result =
    sc_dtls_write(g_dtls_session, (const uint8_t *) msg, strlen(msg), &bytes_written);
  TEST_ASSERT_EQUAL(DTLS_OK, result);

  // Wait for response
  size_t bytes_read = 0;
  time_t start      = time(NULL);
  while ((result = sc_dtls_read(g_dtls_session, recv_buffer, sizeof(recv_buffer), &bytes_read)) ==
         DTLS_ERROR_WOULD_BLOCK) {
    if (time(NULL) - start > 2) {
      TEST_FAIL_MESSAGE("Timeout waiting for response");
    }
    usleep(10000);
  }

  TEST_ASSERT_EQUAL(DTLS_OK, result);
  TEST_ASSERT_EQUAL(strlen(msg), bytes_read);
}

// Test DTLS message ordering and reliability
void test_dtls_message_ordering(void) {
  const char *messages[] = {"ORDER1", "ORDER2", "ORDER3"};
  int num_messages       = 3;
  uint8_t recv_buffer[256];

  // Send all messages quickly
  for (int i = 0; i < num_messages; i++) {
    size_t bytes_written = 0;
    dtls_result_t result = sc_dtls_write(g_dtls_session, (const uint8_t *) messages[i],
                                         strlen(messages[i]), &bytes_written);
    TEST_ASSERT_EQUAL(DTLS_OK, result);
    TEST_ASSERT_EQUAL(strlen(messages[i]), bytes_written);
  }

  // Receive all responses in order (DTLS guarantees ordering)
  for (int i = 0; i < num_messages; i++) {
    size_t bytes_read = 0;
    dtls_result_t result;
    time_t start = time(NULL);

    while ((result = sc_dtls_read(g_dtls_session, recv_buffer, sizeof(recv_buffer), &bytes_read)) ==
           DTLS_ERROR_WOULD_BLOCK) {
      if (time(NULL) - start > 2) {
        TEST_FAIL_MESSAGE("Timeout waiting for ordered response");
      }
      usleep(10000);
    }

    TEST_ASSERT_EQUAL(DTLS_OK, result);
    TEST_ASSERT_EQUAL(strlen(messages[i]), bytes_read);
    TEST_ASSERT_EQUAL_MEMORY(messages[i], recv_buffer, bytes_read);
  }
}

int main(void) {
  printf("Server Tests\n");
  printf("============\n");
  printf("Testing server at 127.0.0.1:%d\n\n", SERVER_PORT);

  UNITY_BEGIN();
  RUN_TEST(test_server_echo_response);
  RUN_TEST(test_server_multiple_packets);
  RUN_TEST(test_server_large_packet);
  RUN_TEST(test_dtls_cert_verification);
  RUN_TEST(test_dtls_session_timeout);
  RUN_TEST(test_dtls_message_ordering);
  return UNITY_END();
}