#include "dtls.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

// Mbed TLS headers
#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#include <mbedtls/timing.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>

// Internal structure definitions
struct dtls_context {
  dtls_role_t role;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt cert;
  mbedtls_pk_context pkey;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_cookie_ctx cookie_ctx;
  uint8_t *pinned_cert_hash;
  size_t pinned_cert_hash_len;
  bool initialized;
  bool rng_initialized;
  bool config_initialized;
  bool cert_initialized;
  bool pkey_initialized;
  bool cookie_initialized;
};

struct dtls_session {
  dtls_context_t *ctx;
  mbedtls_ssl_context ssl;
  mbedtls_timing_delay_context timer;
  int fd;
  struct sockaddr_storage client_addr;
  socklen_t addr_len;
  bool handshake_complete;
};

// Static initialization flag
static bool g_dtls_initialized = false;

// Error string mapping
const char *sc_dtls_error_string(dtls_result_t result) {
  switch (result) {
  case DTLS_OK:
    return "Success";
  case DTLS_ERROR_INIT:
    return "Initialization failed";
  case DTLS_ERROR_HANDSHAKE:
    return "Handshake failed";
  case DTLS_ERROR_HANDSHAKE_TIMEOUT:
    return "Handshake timeout";
  case DTLS_ERROR_READ:
    return "Read failed";
  case DTLS_ERROR_WRITE:
    return "Write failed";
  case DTLS_ERROR_WOULD_BLOCK:
    return "Operation would block";
  case DTLS_ERROR_PEER_CLOSED:
    return "Peer closed connection";
  case DTLS_ERROR_INVALID_PARAMS:
    return "Invalid parameters";
  case DTLS_ERROR_MEMORY:
    return "Memory allocation failed";
  case DTLS_ERROR_CERT_VERIFY:
    return "Certificate verification failed";
  default:
    return "Unknown error";
  }
}

// Debug callback for mbedtls
static void debug_callback(void *ctx, int level, const char *file, int line, const char *str) {
  (void) ctx;
  (void) file;
  (void) line;

#if LOG_LEVEL >= 5
  if (level <= 3) {
    log_debug("mbedtls: %s", str);
  }
#else
  (void) level;
  (void) str;
#endif
}

// UDP send callback for mbedtls
static int udp_send(void *ctx, const unsigned char *buf, size_t len) {
  dtls_session_t *session = (dtls_session_t *) ctx;

  ssize_t ret = sendto(session->fd, buf, len, MSG_DONTWAIT,
                       (struct sockaddr *) &session->client_addr, session->addr_len);

  if (ret < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    return MBEDTLS_ERR_NET_SEND_FAILED;
  }

  return (int) ret;
}

// UDP receive callback for mbedtls
static int udp_recv(void *ctx, unsigned char *buf, size_t len) {
  dtls_session_t *session = (dtls_session_t *) ctx;

  struct sockaddr_storage addr;
  socklen_t addr_len = sizeof(addr);

  ssize_t ret = recvfrom(session->fd, buf, len, MSG_DONTWAIT, (struct sockaddr *) &addr, &addr_len);

  if (ret < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return MBEDTLS_ERR_SSL_WANT_READ;
    }
    return MBEDTLS_ERR_NET_RECV_FAILED;
  }

  // For server, verify packet is from expected client
  if (session->ctx->role == DTLS_ROLE_SERVER) {
    if (addr_len != session->addr_len || memcmp(&addr, &session->client_addr, addr_len) != 0) {
      // Packet from wrong client, ignore
      return MBEDTLS_ERR_SSL_WANT_READ;
    }
  }

  return (int) ret;
}

// UDP receive timeout callback for mbedtls
static int udp_recv_timeout(void *ctx, unsigned char *buf, size_t len, uint32_t timeout) {
  dtls_session_t *session = (dtls_session_t *) ctx;

  struct timeval tv;
  tv.tv_sec  = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(session->fd, &read_fds);

  int ret = select(session->fd + 1, &read_fds, NULL, NULL, &tv);
  if (ret < 0) {
    return MBEDTLS_ERR_NET_RECV_FAILED;
  }

  if (ret == 0) {
    return MBEDTLS_ERR_SSL_TIMEOUT;
  }

  return udp_recv(ctx, buf, len);
}

// Certificate verification callback
static int cert_verify_callback(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
  dtls_session_t *session = (dtls_session_t *) ctx;

  // Only verify server cert for client role with pinning
  if (session->ctx->role != DTLS_ROLE_CLIENT || !session->ctx->pinned_cert_hash || depth != 0) {
    return 0;
  }

  // Calculate hash of presented certificate
  uint8_t cert_hash[32];
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts_ret(&sha_ctx, 0);
  mbedtls_sha256_update_ret(&sha_ctx, crt->raw.p, crt->raw.len);
  mbedtls_sha256_finish_ret(&sha_ctx, cert_hash);
  mbedtls_sha256_free(&sha_ctx);

  // Compare with pinned hash
  if (memcmp(cert_hash, session->ctx->pinned_cert_hash, session->ctx->pinned_cert_hash_len) != 0) {
    *flags |= MBEDTLS_X509_BADCERT_NOT_TRUSTED;
    log_error("%s", "Certificate hash mismatch - potential MITM attack");
    return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
  }

  // Clear flags to indicate success
  *flags = 0;
  return 0;
}

dtls_result_t sc_dtls_init(void) {
  if (g_dtls_initialized) {
    return DTLS_OK;
  }

  g_dtls_initialized = true;
  return DTLS_OK;
}

void sc_dtls_cleanup(void) {
  g_dtls_initialized = false;
}

dtls_context_t *sc_dtls_context_create(dtls_role_t role, const char *cert_path,
                                       const char *key_path, const uint8_t *pinned_cert_hash,
                                       size_t pinned_cert_hash_len) {
  if (!g_dtls_initialized) {
    log_error("%s", "DTLS not initialized");
    return NULL;
  }

  dtls_context_t *ctx = calloc(1, sizeof(dtls_context_t));
  if (!ctx) {
    log_error("%s", "Failed to allocate DTLS context");
    return NULL;
  }

  ctx->role = role;
  int ret;

  // Initialize RNG
  mbedtls_entropy_init(&ctx->entropy);
  mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
  ctx->rng_initialized = true;

  const char *pers = "space_captain_dtls";
  ret              = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy,
                                           (const unsigned char *) pers, strlen(pers));
  if (ret != 0) {
    log_error("Failed to seed random number generator: %d", ret);
    goto error;
  }

  // Initialize SSL config
  mbedtls_ssl_config_init(&ctx->conf);
  ctx->config_initialized = true;

  ret = mbedtls_ssl_config_defaults(
    &ctx->conf, role == DTLS_ROLE_CLIENT ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
    MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    log_error("Failed to set SSL config defaults: %d", ret);
    goto error;
  }

  mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
  mbedtls_ssl_conf_dbg(&ctx->conf, debug_callback, NULL);

  // Set up certificates for server
  if (role == DTLS_ROLE_SERVER) {
    if (!cert_path || !key_path) {
      log_error("%s", "Server requires certificate and key paths");
      goto error;
    }

    mbedtls_x509_crt_init(&ctx->cert);
    ctx->cert_initialized = true;
    mbedtls_pk_init(&ctx->pkey);
    ctx->pkey_initialized = true;

    ret = mbedtls_x509_crt_parse_file(&ctx->cert, cert_path);
    if (ret != 0) {
      log_error("Failed to load certificate from %s: %d", cert_path, ret);
      goto error;
    }

    ret = mbedtls_pk_parse_keyfile(&ctx->pkey, key_path, NULL);
    if (ret != 0) {
      log_error("Failed to load private key from %s: %d", key_path, ret);
      goto error;
    }

    ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->cert, &ctx->pkey);
    if (ret != 0) {
      log_error("Failed to configure certificate: %d", ret);
      goto error;
    }

    // Initialize cookie for DoS protection
    mbedtls_ssl_cookie_init(&ctx->cookie_ctx);
    ctx->cookie_initialized = true;
    ret = mbedtls_ssl_cookie_setup(&ctx->cookie_ctx, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    if (ret != 0) {
      log_error("Failed to setup cookie context: %d", ret);
      goto error;
    }

    mbedtls_ssl_conf_dtls_cookies(&ctx->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
                                  &ctx->cookie_ctx);
  }

  // Store pinned cert hash for client
  if (role == DTLS_ROLE_CLIENT && pinned_cert_hash && pinned_cert_hash_len == 32) {
    ctx->pinned_cert_hash = malloc(32);
    if (!ctx->pinned_cert_hash) {
      log_error("%s", "Failed to allocate pinned cert hash");
      goto error;
    }
    memcpy(ctx->pinned_cert_hash, pinned_cert_hash, 32);
    ctx->pinned_cert_hash_len = 32;

    // Set verification mode to optional (we'll verify in callback)
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  } else {
    // No verification for server or client without pinning
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
  }

  // Configure cipher suites for performance (prefer AES-GCM)
  static const int ciphersuites[] = {MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
                                     MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
                                     MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
                                     MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384, 0};
  mbedtls_ssl_conf_ciphersuites(&ctx->conf, ciphersuites);

  // Set timeouts
  mbedtls_ssl_conf_read_timeout(&ctx->conf, 30000); // 30 seconds

  ctx->initialized = true;
  return ctx;

error:
  sc_dtls_context_destroy(ctx);
  return NULL;
}

void sc_dtls_context_destroy(dtls_context_t *ctx) {
  if (!ctx)
    return;

  // Free SSL config if initialized
  if (ctx->config_initialized) {
    mbedtls_ssl_config_free(&ctx->conf);
  }

  // Free server-specific resources
  if (ctx->cert_initialized) {
    mbedtls_x509_crt_free(&ctx->cert);
  }
  if (ctx->pkey_initialized) {
    mbedtls_pk_free(&ctx->pkey);
  }
  if (ctx->cookie_initialized) {
    mbedtls_ssl_cookie_free(&ctx->cookie_ctx);
  }

  // Free RNG resources
  if (ctx->rng_initialized) {
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
  }

  free(ctx->pinned_cert_hash);
  free(ctx);
}

dtls_session_t *sc_dtls_session_create(dtls_context_t *ctx, int fd,
                                       const struct sockaddr *client_addr, socklen_t addr_len) {
  if (!ctx || fd < 0) {
    return NULL;
  }

  dtls_session_t *session = calloc(1, sizeof(dtls_session_t));
  if (!session) {
    log_error("%s", "Failed to allocate DTLS session");
    return NULL;
  }

  session->ctx = ctx;
  session->fd  = fd;

  // Store client address
  if (client_addr && addr_len > 0) {
    memcpy(&session->client_addr, client_addr, addr_len);
    session->addr_len = addr_len;
  }

  // Initialize SSL context
  mbedtls_ssl_init(&session->ssl);
  // mbedtls_timing_delay_context doesn't need explicit init

  int ret = mbedtls_ssl_setup(&session->ssl, &ctx->conf);
  if (ret != 0) {
    log_error("Failed to setup SSL context: %d", ret);
    sc_dtls_session_destroy(session);
    return NULL;
  }

  // Set bio callbacks
  mbedtls_ssl_set_bio(&session->ssl, session, udp_send, udp_recv, udp_recv_timeout);

  // Set timer callbacks
  mbedtls_ssl_set_timer_cb(&session->ssl, &session->timer, mbedtls_timing_set_delay,
                           mbedtls_timing_get_delay);

  // Set verification callback for certificate pinning
  if (ctx->role == DTLS_ROLE_CLIENT && ctx->pinned_cert_hash) {
    mbedtls_ssl_set_verify(&session->ssl, cert_verify_callback, session);
  }

  // For server, set client address for cookie verification
  if (ctx->role == DTLS_ROLE_SERVER && client_addr) {
    ret = mbedtls_ssl_set_client_transport_id(&session->ssl, (const unsigned char *) client_addr,
                                              addr_len);
    if (ret != 0) {
      log_error("Failed to set client transport ID: %d", ret);
      sc_dtls_session_destroy(session);
      return NULL;
    }
  }

  // Make socket non-blocking
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    log_error("%s", "Failed to set socket non-blocking");
    sc_dtls_session_destroy(session);
    return NULL;
  }

  return session;
}

void sc_dtls_session_destroy(dtls_session_t *session) {
  if (!session)
    return;

  mbedtls_ssl_free(&session->ssl);
  free(session);
}

dtls_result_t sc_dtls_handshake(dtls_session_t *session) {
  if (!session)
    return DTLS_ERROR_INVALID_PARAMS;

  int ret = mbedtls_ssl_handshake(&session->ssl);

  if (ret == 0) {
    session->handshake_complete = true;
    log_info("%s", "DTLS handshake completed");
    return DTLS_OK;
  }

  if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
    return DTLS_ERROR_WOULD_BLOCK;
  }

  if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
    return DTLS_ERROR_HANDSHAKE_TIMEOUT;
  }

  char error_buf[256];
  mbedtls_strerror(ret, error_buf, sizeof(error_buf));
  log_error("DTLS handshake failed: %s", error_buf);

  if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
    return DTLS_ERROR_CERT_VERIFY;
  }

  return DTLS_ERROR_HANDSHAKE;
}

dtls_result_t sc_dtls_read(dtls_session_t *session, uint8_t *buf, size_t len, size_t *bytes_read) {
  if (!session || !buf || !bytes_read)
    return DTLS_ERROR_INVALID_PARAMS;

  *bytes_read = 0;

  int ret = mbedtls_ssl_read(&session->ssl, buf, len);

  if (ret > 0) {
    *bytes_read = (size_t) ret;
    return DTLS_OK;
  }

  if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
    return DTLS_ERROR_WOULD_BLOCK;
  }

  if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
    return DTLS_ERROR_PEER_CLOSED;
  }

  char error_buf[256];
  mbedtls_strerror(ret, error_buf, sizeof(error_buf));
  log_error("DTLS read failed: %s", error_buf);

  return DTLS_ERROR_READ;
}

dtls_result_t sc_dtls_write(dtls_session_t *session, const uint8_t *buf, size_t len,
                            size_t *bytes_written) {
  if (!session || !buf || !bytes_written)
    return DTLS_ERROR_INVALID_PARAMS;

  *bytes_written = 0;

  int ret = mbedtls_ssl_write(&session->ssl, buf, len);

  if (ret > 0) {
    *bytes_written = (size_t) ret;
    return DTLS_OK;
  }

  if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
    return DTLS_ERROR_WOULD_BLOCK;
  }

  char error_buf[256];
  mbedtls_strerror(ret, error_buf, sizeof(error_buf));
  log_error("DTLS write failed: %s", error_buf);

  return DTLS_ERROR_WRITE;
}

void sc_dtls_close(dtls_session_t *session) {
  if (!session)
    return;

  // Send close notify
  mbedtls_ssl_close_notify(&session->ssl);
}

int sc_dtls_get_fd(dtls_session_t *session) {
  return session ? session->fd : -1;
}

const struct sockaddr *sc_dtls_get_client_addr(dtls_session_t *session) {
  return session ? (const struct sockaddr *) &session->client_addr : NULL;
}

socklen_t sc_dtls_get_client_addr_len(dtls_session_t *session) {
  return session ? session->addr_len : 0;
}

bool sc_dtls_is_handshake_complete(dtls_session_t *session) {
  return session ? session->handshake_complete : false;
}

dtls_result_t sc_dtls_cert_hash(const char *cert_path, uint8_t (*hash)[32]) {
  if (!cert_path || !hash)
    return DTLS_ERROR_INVALID_PARAMS;

  mbedtls_x509_crt cert;
  mbedtls_x509_crt_init(&cert);

  int ret = mbedtls_x509_crt_parse_file(&cert, cert_path);
  if (ret != 0) {
    mbedtls_x509_crt_free(&cert);
    return DTLS_ERROR_INIT;
  }

  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts_ret(&sha_ctx, 0);
  mbedtls_sha256_update_ret(&sha_ctx, cert.raw.p, cert.raw.len);
  mbedtls_sha256_finish_ret(&sha_ctx, *hash); // Dereference pointer to pass the array
  mbedtls_sha256_free(&sha_ctx);

  mbedtls_x509_crt_free(&cert);
  return DTLS_OK;
}
