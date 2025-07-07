#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "vendor/unity.h"
#include "../src/dtls.h"

void setUp(void) {
  // Setup for each test
}

void tearDown(void) {
  // Cleanup after each test
}

void test_dtls_init_cleanup(void) {
  dtls_result_t result = sc_dtls_init();
  TEST_ASSERT_EQUAL(DTLS_OK, result);

  // Second init should also succeed (idempotent)
  result = sc_dtls_init();
  TEST_ASSERT_EQUAL(DTLS_OK, result);

  sc_dtls_cleanup();
}

void test_dtls_context_creation_server(void) {
  sc_dtls_init();

  // Server context requires cert and key
  dtls_context_t *ctx =
    sc_dtls_context_create(DTLS_ROLE_SERVER, "certs/server.crt", "certs/server.key", NULL, 0);
  TEST_ASSERT_NOT_NULL(ctx);

  sc_dtls_context_destroy(ctx);
  sc_dtls_cleanup();
}

void test_dtls_context_creation_client(void) {
  sc_dtls_init();

  // Client context without pinning
  dtls_context_t *ctx = sc_dtls_context_create(DTLS_ROLE_CLIENT, NULL, NULL, NULL, 0);
  TEST_ASSERT_NOT_NULL(ctx);

  sc_dtls_context_destroy(ctx);
  sc_dtls_cleanup();
}

void test_dtls_cert_hash(void) {
  sc_dtls_init();

  uint8_t hash[32];
  dtls_result_t result = sc_dtls_cert_hash("certs/server.crt", hash);
  TEST_ASSERT_EQUAL(DTLS_OK, result);

  // Hash should not be all zeros
  int all_zeros = 1;
  for (int i = 0; i < 32; i++) {
    if (hash[i] != 0) {
      all_zeros = 0;
      break;
    }
  }
  TEST_ASSERT_FALSE(all_zeros);

  sc_dtls_cleanup();
}

void test_dtls_error_strings(void) {
  TEST_ASSERT_EQUAL_STRING("Success", sc_dtls_error_string(DTLS_OK));
  TEST_ASSERT_EQUAL_STRING("Initialization failed", sc_dtls_error_string(DTLS_ERROR_INIT));
  TEST_ASSERT_EQUAL_STRING("Handshake failed", sc_dtls_error_string(DTLS_ERROR_HANDSHAKE));
  TEST_ASSERT_EQUAL_STRING("Certificate verification failed",
                           sc_dtls_error_string(DTLS_ERROR_CERT_VERIFY));
}

void test_dtls_session_creation(void) {
  sc_dtls_init();

  // Create a UDP socket
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT_NOT_EQUAL(-1, fd);

  // Create server context
  dtls_context_t *ctx =
    sc_dtls_context_create(DTLS_ROLE_SERVER, "certs/server.crt", "certs/server.key", NULL, 0);
  TEST_ASSERT_NOT_NULL(ctx);

  // Create session
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family      = AF_INET;
  client_addr.sin_port        = htons(12345);
  client_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  dtls_session_t *session =
    sc_dtls_session_create(ctx, fd, (struct sockaddr *) &client_addr, sizeof(client_addr));
  TEST_ASSERT_NOT_NULL(session);

  // Check session properties
  TEST_ASSERT_EQUAL(fd, sc_dtls_get_fd(session));
  TEST_ASSERT_FALSE(sc_dtls_is_handshake_complete(session));
  TEST_ASSERT_NOT_NULL(sc_dtls_get_client_addr(session));
  TEST_ASSERT_EQUAL(sizeof(client_addr), sc_dtls_get_client_addr_len(session));

  sc_dtls_session_destroy(session);
  sc_dtls_context_destroy(ctx);
  close(fd);
  sc_dtls_cleanup();
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_dtls_init_cleanup);
  RUN_TEST(test_dtls_context_creation_server);
  RUN_TEST(test_dtls_context_creation_client);
  RUN_TEST(test_dtls_cert_hash);
  RUN_TEST(test_dtls_error_strings);
  RUN_TEST(test_dtls_session_creation);

  return UNITY_END();
}