#ifndef DTLS_H
#define DTLS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

// Forward declaration to hide implementation details
typedef struct dtls_context dtls_context_t;
typedef struct dtls_session dtls_session_t;

// DTLS role
typedef enum { DTLS_ROLE_CLIENT, DTLS_ROLE_SERVER } dtls_role_t;

// DTLS operation result codes
typedef enum {
  DTLS_OK                      = 0,
  DTLS_ERROR_INIT              = -1,
  DTLS_ERROR_HANDSHAKE         = -2,
  DTLS_ERROR_HANDSHAKE_TIMEOUT = -3,
  DTLS_ERROR_READ              = -4,
  DTLS_ERROR_WRITE             = -5,
  DTLS_ERROR_WOULD_BLOCK       = -6,
  DTLS_ERROR_PEER_CLOSED       = -7,
  DTLS_ERROR_INVALID_PARAMS    = -8,
  DTLS_ERROR_MEMORY            = -9,
  DTLS_ERROR_CERT_VERIFY       = -10
} dtls_result_t;

// Initialize DTLS library (call once at startup)
// Returns: DTLS_OK on success, error code on failure
dtls_result_t sc_dtls_init(void);

// Cleanup DTLS library (call once at shutdown)
void sc_dtls_cleanup(void);

// Create a DTLS context for server or client
// Parameters:
//   role: DTLS_ROLE_SERVER or DTLS_ROLE_CLIENT
//   cert_path: Path to certificate file (server only, can be NULL for client)
//   key_path: Path to private key file (server only, can be NULL for client)
//   pinned_cert_hash: Expected server cert hash for pinning (client only, can be NULL for server)
//   pinned_cert_hash_len: Length of pinned cert hash
// Returns: Context pointer on success, NULL on failure
dtls_context_t *sc_dtls_context_create(dtls_role_t role, const char *cert_path,
                                       const char *key_path, const uint8_t *pinned_cert_hash,
                                       size_t pinned_cert_hash_len);

// Destroy a DTLS context
void sc_dtls_context_destroy(dtls_context_t *ctx);

// Create a new DTLS session
// Parameters:
//   ctx: DTLS context
//   fd: Socket file descriptor
//   client_addr: Client address (for server role)
//   addr_len: Length of client address
// Returns: Session pointer on success, NULL on failure
dtls_session_t *sc_dtls_session_create(dtls_context_t *ctx, int fd,
                                       const struct sockaddr *client_addr, socklen_t addr_len);

// Destroy a DTLS session
void sc_dtls_session_destroy(dtls_session_t *session);

// Perform DTLS handshake (non-blocking)
// Returns: DTLS_OK on completion, DTLS_ERROR_WOULD_BLOCK if in progress, error code on failure
dtls_result_t sc_dtls_handshake(dtls_session_t *session);

// Read data from DTLS session (non-blocking)
// Parameters:
//   session: DTLS session
//   buf: Buffer to read into
//   len: Maximum bytes to read
//   bytes_read: Actual bytes read (output)
// Returns: DTLS_OK on success, DTLS_ERROR_WOULD_BLOCK if no data, error code on failure
dtls_result_t sc_dtls_read(dtls_session_t *session, uint8_t *buf, size_t len, size_t *bytes_read);

// Write data to DTLS session (non-blocking)
// Parameters:
//   session: DTLS session
//   buf: Data to write
//   len: Bytes to write
//   bytes_written: Actual bytes written (output)
// Returns: DTLS_OK on success, DTLS_ERROR_WOULD_BLOCK if would block, error code on failure
dtls_result_t sc_dtls_write(dtls_session_t *session, const uint8_t *buf, size_t len,
                            size_t *bytes_written);

// Close DTLS session gracefully
void sc_dtls_close(dtls_session_t *session);

// Get file descriptor for a session (for epoll)
int sc_dtls_get_fd(dtls_session_t *session);

// Get client address for a session
const struct sockaddr *sc_dtls_get_client_addr(dtls_session_t *session);

// Get client address length for a session
socklen_t sc_dtls_get_client_addr_len(dtls_session_t *session);

// Check if handshake is complete
bool sc_dtls_is_handshake_complete(dtls_session_t *session);

// Get last error string for debugging
const char *sc_dtls_error_string(dtls_result_t result);

// Calculate SHA256 hash of a certificate file (for pinning)
// Parameters:
//   cert_path: Path to certificate file
//   hash: Buffer to store hash (must be at least 32 bytes)
// Returns: DTLS_OK on success, error code on failure
dtls_result_t sc_dtls_cert_hash(const char *cert_path, uint8_t *hash);

#endif // DTLS_H