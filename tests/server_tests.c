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

#include "vendor/unity.c"
#include "../src/config.h"

// Test globals
static int test_socket = -1;
static struct sockaddr_in server_addr;
static pid_t server_pid = -1;

void setUp(void) {
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

  // Set socket timeout
  struct timeval tv;
  tv.tv_sec  = 2; // 2 second timeout
  tv.tv_usec = 0;
  setsockopt(test_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  // Setup server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons(SERVER_PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
}

void tearDown(void) {
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
  char recv_buffer[256];

  // Send test message
  ssize_t sent = sendto(test_socket, test_msg, strlen(test_msg), 0,
                        (struct sockaddr *) &server_addr, sizeof(server_addr));
  TEST_ASSERT_EQUAL(strlen(test_msg), sent);

  // Receive response
  struct sockaddr_in from_addr;
  socklen_t from_len = sizeof(from_addr);
  ssize_t received   = recvfrom(test_socket, recv_buffer, sizeof(recv_buffer), 0,
                                (struct sockaddr *) &from_addr, &from_len);

  // Verify response
  TEST_ASSERT_NOT_EQUAL(-1, received);
  TEST_ASSERT_EQUAL(strlen(test_msg), received);
  TEST_ASSERT_EQUAL_MEMORY(test_msg, recv_buffer, received);
}

// Test that server handles multiple packets
void test_server_multiple_packets(void) {
  const char *messages[] = {"MSG1", "MSG2", "MSG3"};
  char recv_buffer[256];

  for (int i = 0; i < 3; i++) {
    // Send message
    ssize_t sent = sendto(test_socket, messages[i], strlen(messages[i]), 0,
                          (struct sockaddr *) &server_addr, sizeof(server_addr));
    TEST_ASSERT_EQUAL(strlen(messages[i]), sent);

    // Receive response
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    ssize_t received   = recvfrom(test_socket, recv_buffer, sizeof(recv_buffer), 0,
                                  (struct sockaddr *) &from_addr, &from_len);

    // Verify response
    TEST_ASSERT_NOT_EQUAL(-1, received);
    TEST_ASSERT_EQUAL(strlen(messages[i]), received);
    TEST_ASSERT_EQUAL_MEMORY(messages[i], recv_buffer, received);
  }
}

// Test large packet handling
void test_server_large_packet(void) {
  // Create a large test message
  char large_msg[1024];
  memset(large_msg, 'A', sizeof(large_msg) - 1);
  large_msg[sizeof(large_msg) - 1] = '\0';

  char recv_buffer[2048];

  // Send large message
  ssize_t sent = sendto(test_socket, large_msg, strlen(large_msg), 0,
                        (struct sockaddr *) &server_addr, sizeof(server_addr));
  TEST_ASSERT_EQUAL(strlen(large_msg), sent);

  // Receive response
  struct sockaddr_in from_addr;
  socklen_t from_len = sizeof(from_addr);
  ssize_t received   = recvfrom(test_socket, recv_buffer, sizeof(recv_buffer), 0,
                                (struct sockaddr *) &from_addr, &from_len);

  // Verify response
  TEST_ASSERT_NOT_EQUAL(-1, received);
  TEST_ASSERT_EQUAL(strlen(large_msg), received);
  TEST_ASSERT_EQUAL_MEMORY(large_msg, recv_buffer, received);
}

// Test handling of socket errors (invalid server address)
void test_socket_error_handling(void) {
  struct sockaddr_in bad_addr;
  memset(&bad_addr, 0, sizeof(bad_addr));
  bad_addr.sin_family = AF_INET;
  bad_addr.sin_port   = htons(SERVER_PORT);
  inet_pton(AF_INET, "192.0.2.1",
            &bad_addr.sin_addr); // TEST-NET-1, non-routable IP used for testing

  const char *msg = "TEST";
  ssize_t sent =
    sendto(test_socket, msg, strlen(msg), 0, (struct sockaddr *) &bad_addr, sizeof(bad_addr));
  TEST_ASSERT_EQUAL(strlen(msg), sent);

  char buf[256];
  ssize_t recv_len = recvfrom(test_socket, buf, sizeof(buf), 0, NULL, NULL);

  TEST_ASSERT_EQUAL(-1, recv_len);
  TEST_ASSERT_EQUAL(EAGAIN, errno); // Expect timeout due to unreachable address
}

// Test robustness when server is abruptly terminated
void test_robustness_server_unavailable(void) {
  // Stop the server first
  kill(server_pid, SIGTERM);
  waitpid(server_pid, NULL, 0);
  server_pid = -1;

  const char *msg = "PING";
  ssize_t sent =
    sendto(test_socket, msg, strlen(msg), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
  TEST_ASSERT_EQUAL(strlen(msg), sent);

  char buf[256];
  ssize_t received = recvfrom(test_socket, buf, sizeof(buf), 0, NULL, NULL);

  TEST_ASSERT_EQUAL(-1, received);
  TEST_ASSERT_EQUAL(EAGAIN, errno); // Expect timeout due to no response
}

// Test epoll edge-trigger behavior correctness
void test_epoll_edge_trigger_behavior(void) {
  const char *messages[] = {"EPOLL1", "EPOLL2", "EPOLL3"};
  int num_messages       = 3;

  for (int i = 0; i < num_messages; i++) {
    ssize_t sent = sendto(test_socket, messages[i], strlen(messages[i]), 0,
                          (struct sockaddr *) &server_addr, sizeof(server_addr));
    TEST_ASSERT_EQUAL(strlen(messages[i]), sent);
  }

  // Receive all responses without delay
  for (int i = 0; i < num_messages; i++) {
    char buf[256];
    ssize_t received = recvfrom(test_socket, buf, sizeof(buf), 0, NULL, NULL);

    TEST_ASSERT_NOT_EQUAL(-1, received);
    TEST_ASSERT_EQUAL(strlen(messages[i]), received);
    TEST_ASSERT_EQUAL_MEMORY(messages[i], buf, received);
  }

  // Confirm no more immediate responses (socket now empty)
  char extra_buf[256];
  ssize_t extra_received = recvfrom(test_socket, extra_buf, sizeof(extra_buf), 0, NULL, NULL);
  TEST_ASSERT_EQUAL(-1, extra_received);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
}

int main(void) {
  printf("Server Tests\n");
  printf("============\n");
  printf("Testing server at 127.0.0.1:%d\n\n", SERVER_PORT);

  UNITY_BEGIN();
  RUN_TEST(test_server_echo_response);
  RUN_TEST(test_server_multiple_packets);
  RUN_TEST(test_server_large_packet);
  RUN_TEST(test_socket_error_handling);
  RUN_TEST(test_robustness_server_unavailable);
  RUN_TEST(test_epoll_edge_trigger_behavior);
  return UNITY_END();
}