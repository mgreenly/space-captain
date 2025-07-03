# OpenSSL Integration Research for Space Captain

This document provides an overview of how the OpenSSL library could be integrated into the Space Captain server and client to provide TLS encryption using self-signed certificates.

## Overview

OpenSSL is a robust, full-featured cryptographic library that provides comprehensive SSL/TLS support. While heavier than mbed-tls, it offers:
- Industry-standard implementation with extensive testing
- Hardware acceleration support
- Advanced features (OCSP, session tickets, etc.)
- Wide platform support and active development
- Extensive documentation and community support

## Current Architecture Analysis

### Server Architecture
- Uses epoll-based event loop with edge-triggered mode
- Non-blocking sockets with TCP_NODELAY
- Pre-allocated connection pool for 5000+ concurrent connections
- Fixed protocol: 8-byte header (type + length) + message body

### Client Architecture
- Simple blocking socket implementation
- Connects to server on port 4242
- Sends three message types: ECHO, REVERSE, TIME

## Proposed OpenSSL Integration

### 1. Certificate Generation

Generate self-signed certificates using OpenSSL command-line tools:

```bash
# Generate private key
openssl genrsa -out server.key 4096

# Generate self-signed certificate
openssl req -new -x509 -key server.key -out server.crt -days 3650 \
    -subj "/C=US/O=SpaceCaptain/CN=spacecaptain"

# For development, create a combined PEM file
cat server.crt server.key > server.pem
```

### 2. Required Headers

```c
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
```

### 3. Server-Side Integration

#### TLS Context Structure
```c
typedef struct {
    SSL_CTX *ctx;
    X509 *cert;
    EVP_PKEY *pkey;
} server_tls_context_t;

typedef struct {
    SSL *ssl;
    BIO *rbio;
    BIO *wbio;
    enum { TLS_HANDSHAKING, TLS_READY } tls_state;
} client_tls_context_t;
```

#### Modified Client Buffer Structure
```c
typedef struct client_buffer {
    int fd;
    client_tls_context_t *tls;  // Add TLS context
    uint8_t *buffer;
    size_t buffer_size;
    size_t data_len;
    enum { READING_HEADER, READING_BODY } state;
    message_header_t header;
    size_t header_bytes_read;
    struct client_buffer *next;
    int in_use;
} client_buffer_t;
```

#### Server TLS Initialization
```c
static server_tls_context_t *init_server_tls(void) {
    server_tls_context_t *tls = calloc(1, sizeof(server_tls_context_t));
    if (!tls) return NULL;
    
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // Create SSL context
    const SSL_METHOD *method = TLS_server_method();
    tls->ctx = SSL_CTX_new(method);
    if (!tls->ctx) {
        log_error("Failed to create SSL context: %s", 
                  ERR_error_string(ERR_get_error(), NULL));
        free(tls);
        return NULL;
    }
    
    // Set minimum TLS version
    SSL_CTX_set_min_proto_version(tls->ctx, TLS1_2_VERSION);
    
    // Load certificate and key
    if (SSL_CTX_use_certificate_file(tls->ctx, TLS_CERT_PATH, SSL_FILETYPE_PEM) != 1) {
        log_error("Failed to load certificate: %s", 
                  ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(tls->ctx);
        free(tls);
        return NULL;
    }
    
    if (SSL_CTX_use_PrivateKey_file(tls->ctx, TLS_KEY_PATH, SSL_FILETYPE_PEM) != 1) {
        log_error("Failed to load private key: %s", 
                  ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(tls->ctx);
        free(tls);
        return NULL;
    }
    
    // Verify private key
    if (!SSL_CTX_check_private_key(tls->ctx)) {
        log_error("Private key does not match certificate");
        SSL_CTX_free(tls->ctx);
        free(tls);
        return NULL;
    }
    
    // Configure cipher suites (modern secure ciphers only)
    SSL_CTX_set_cipher_list(tls->ctx, 
        "ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM:DHE+CHACHA20:!aNULL:!MD5:!DSS");
    
    // Disable compression (CRIME attack)
    SSL_CTX_set_options(tls->ctx, SSL_OP_NO_COMPRESSION);
    
    // Enable session caching for performance
    SSL_CTX_set_session_cache_mode(tls->ctx, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(tls->ctx, 1024);
    
    return tls;
}
```

#### Non-blocking TLS with Memory BIOs
```c
static client_tls_context_t *create_client_tls(SSL_CTX *ctx) {
    client_tls_context_t *tls = calloc(1, sizeof(client_tls_context_t));
    if (!tls) return NULL;
    
    tls->ssl = SSL_new(ctx);
    if (!tls->ssl) {
        free(tls);
        return NULL;
    }
    
    // Create memory BIOs for non-blocking operation
    tls->rbio = BIO_new(BIO_s_mem());
    tls->wbio = BIO_new(BIO_s_mem());
    
    // Set BIOs to non-blocking
    BIO_set_nbio(tls->rbio, 1);
    BIO_set_nbio(tls->wbio, 1);
    
    SSL_set_bio(tls->ssl, tls->rbio, tls->wbio);
    SSL_set_accept_state(tls->ssl);
    
    tls->tls_state = TLS_HANDSHAKING;
    
    return tls;
}
```

#### Handle TLS Handshake
```c
static int handle_tls_handshake(int client_fd, client_buffer_t *client) {
    if (!client->tls) return -1;
    
    // Process any pending data from socket to BIO
    uint8_t buf[4096];
    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
    if (n > 0) {
        BIO_write(client->tls->rbio, buf, n);
    }
    
    // Try to progress handshake
    int ret = SSL_do_handshake(client->tls->ssl);
    
    if (ret == 1) {
        // Handshake complete
        client->tls->tls_state = TLS_READY;
        log_info("TLS handshake completed for fd=%d", client_fd);
        return 1;
    }
    
    int ssl_error = SSL_get_error(client->tls->ssl, ret);
    
    // Check if we need to write data
    if (ssl_error == SSL_ERROR_WANT_WRITE || BIO_ctrl_pending(client->tls->wbio) > 0) {
        // Read from write BIO and send to socket
        while (BIO_ctrl_pending(client->tls->wbio) > 0) {
            int len = BIO_read(client->tls->wbio, buf, sizeof(buf));
            if (len > 0) {
                send(client_fd, buf, len, 0);
            }
        }
    }
    
    if (ssl_error == SSL_ERROR_WANT_READ) {
        return 0; // Need more data
    }
    
    // Handle errors
    if (ssl_error != SSL_ERROR_WANT_WRITE) {
        log_error("TLS handshake failed: %s", 
                  ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    
    return 0; // Handshake in progress
}
```

#### Modified Data Reading
```c
static int read_tls_message(int client_fd, client_buffer_t *buf, message_t **msg) {
    if (!buf->tls || buf->tls->tls_state != TLS_READY) {
        return -1;
    }
    
    *msg = NULL;
    
    // Read data from socket to BIO if available
    uint8_t net_buf[4096];
    ssize_t n = recv(client_fd, net_buf, sizeof(net_buf), 0);
    if (n > 0) {
        BIO_write(buf->tls->rbio, net_buf, n);
    } else if (n == 0) {
        return -1; // Connection closed
    }
    
    // Try to read decrypted data
    if (buf->state == READING_HEADER) {
        size_t remaining = sizeof(message_header_t) - buf->header_bytes_read;
        int ret = SSL_read(buf->tls->ssl, 
                          ((char *)&buf->header) + buf->header_bytes_read, 
                          remaining);
        
        if (ret > 0) {
            buf->header_bytes_read += ret;
            if (buf->header_bytes_read == sizeof(message_header_t)) {
                // Header complete, prepare for body
                // ... (same as original)
            }
        } else {
            int ssl_error = SSL_get_error(buf->tls->ssl, ret);
            if (ssl_error != SSL_ERROR_WANT_READ) {
                return -1; // Error
            }
        }
    }
    
    // Similar logic for READING_BODY state
    // ...
    
    // Send any pending encrypted data
    while (BIO_ctrl_pending(buf->tls->wbio) > 0) {
        int len = BIO_read(buf->tls->wbio, net_buf, sizeof(net_buf));
        if (len > 0) {
            send(client_fd, net_buf, len, 0);
        }
    }
    
    return 0;
}
```

### 4. Client-Side Integration

#### Client TLS Initialization with Certificate Pinning
```c
typedef struct {
    SSL_CTX *ctx;
    SSL *ssl;
    int fd;
} client_tls_t;

static client_tls_t *init_client_tls(void) {
    client_tls_t *tls = calloc(1, sizeof(client_tls_t));
    if (!tls) return NULL;
    
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // Create SSL context
    const SSL_METHOD *method = TLS_client_method();
    tls->ctx = SSL_CTX_new(method);
    if (!tls->ctx) {
        free(tls);
        return NULL;
    }
    
    // Set minimum TLS version
    SSL_CTX_set_min_proto_version(tls->ctx, TLS1_2_VERSION);
    
    // Certificate pinning - load pinned certificate
    if (SSL_CTX_load_verify_locations(tls->ctx, TLS_PINNED_CERT_PATH, NULL) != 1) {
        log_error("Failed to load pinned certificate: %s", 
                  ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(tls->ctx);
        free(tls);
        return NULL;
    }
    
    // Always verify against pinned certificate
    SSL_CTX_set_verify(tls->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(tls->ctx, 1);
    
    return tls;
}
```

#### Modified Connection Function
```c
static int connect_to_server_tls(client_tls_t *tls) {
    // Create socket connection
    tls->fd = connect_to_server();
    if (tls->fd < 0) return -1;
    
    // Create SSL connection
    tls->ssl = SSL_new(tls->ctx);
    if (!tls->ssl) {
        close(tls->fd);
        return -1;
    }
    
    SSL_set_fd(tls->ssl, tls->fd);
    SSL_set_tlsext_host_name(tls->ssl, SERVER_HOST);
    
    // Perform handshake
    int ret = SSL_connect(tls->ssl);
    if (ret != 1) {
        log_error("TLS handshake failed: %s", 
                  ERR_error_string(SSL_get_error(tls->ssl, ret), NULL));
        SSL_free(tls->ssl);
        close(tls->fd);
        return -1;
    }
    
    // Verify pinned certificate matches
    X509 *cert = SSL_get_peer_certificate(tls->ssl);
    if (!cert) {
        log_error("No server certificate received");
        SSL_free(tls->ssl);
        close(tls->fd);
        return -1;
    }
    
    // Certificate is already verified against pinned cert by OpenSSL
    // Additional fingerprint checking can be done here if needed
    log_info("Connected with pinned certificate: %s", 
             X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0));
    X509_free(cert);
    
    return tls->fd;
}
```

### 5. Performance Optimizations

#### Session Resumption
```c
// Server side - already enabled in init
SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);

// Client side - enable session caching
SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);

// Save session after successful connection
SSL_SESSION *session = SSL_get1_session(ssl);
// Store session for reuse

// Reuse session on reconnection
SSL_set_session(ssl, saved_session);
```

#### TLS False Start
```c
// Enable TLS False Start for reduced latency
SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_FALSE_START);
```

#### Buffer Tuning
```c
// Increase buffer sizes for better throughput
SSL_CTX_set_default_read_buffer_len(ctx, 16384);
```

### 6. Configuration Updates

Add to `config.h`:
```c
// TLS Configuration
#define TLS_ENABLED 1
#define TLS_CERT_PATH "certs/server.crt"
#define TLS_KEY_PATH "certs/server.key"
#define TLS_PINNED_CERT_PATH "certs/pinned_cert.crt"
#define TLS_SESSION_TIMEOUT 300
#define TLS_SESSION_CACHE_SIZE 1024
#define TLS_HANDSHAKE_TIMEOUT_MS 5000

// OpenSSL specific
#define TLS_CIPHERS "ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM:DHE+CHACHA20"
#define TLS_CURVES "X25519:P-256:P-384"
```

### 7. Build System Integration

Update Makefile:
```makefile
# OpenSSL flags
CFLAGS += $(shell pkg-config --cflags openssl)
LDFLAGS += $(shell pkg-config --libs openssl)

# Or manual flags if pkg-config not available
# CFLAGS += -I/usr/include/openssl
# LDFLAGS += -lssl -lcrypto
```

### 8. Platform Support

#### Linux
- **Availability**: Excellent - pre-installed or available in all distributions
- **Installation**:
  ```bash
  # Debian/Ubuntu
  sudo apt-get install libssl-dev
  
  # Red Hat/CentOS/Fedora
  sudo yum install openssl-devel
  # or
  sudo dnf install openssl-devel
  
  # Arch Linux
  sudo pacman -S openssl
  
  # Alpine Linux
  apk add openssl-dev
  ```
- **Build**: Standard Unix build works perfectly
- **Notes**: 
  - epoll-based server code is Linux-specific
  - Usually links against system OpenSSL (1.1.1 or 3.x)

#### macOS
- **Availability**: Complex - Apple deprecated OpenSSL in favor of their own libraries
- **Installation Options**:
  1. **Homebrew** (Recommended):
     ```bash
     brew install openssl@3
     # or for compatibility
     brew install openssl@1.1
     ```
  2. **MacPorts**:
     ```bash
     sudo port install openssl3
     # or
     sudo port install openssl11
     ```
  3. **System Library**: macOS includes LibreSSL, not OpenSSL

- **Build Considerations**:
  - Must explicitly link against Homebrew/MacPorts OpenSSL:
    ```makefile
    # For Homebrew on Apple Silicon
    CFLAGS += -I/opt/homebrew/opt/openssl@3/include
    LDFLAGS += -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
    
    # For Homebrew on Intel
    CFLAGS += -I/usr/local/opt/openssl@3/include
    LDFLAGS += -L/usr/local/opt/openssl@3/lib -lssl -lcrypto
    
    # Or use pkg-config
    export PKG_CONFIG_PATH="/opt/homebrew/opt/openssl@3/lib/pkgconfig"
    CFLAGS += $(shell pkg-config --cflags openssl)
    LDFLAGS += $(shell pkg-config --libs openssl)
    ```
  - Replace epoll with kqueue for event handling
  - Consider using Secure Transport API instead for native macOS feel

- **Code Changes Required**:
  ```c
  #ifdef __APPLE__
    #include <sys/event.h>  // kqueue instead of epoll
    #include <sys/time.h>
    
    // May need to handle SIGPIPE differently
    signal(SIGPIPE, SIG_IGN);
  #endif
  ```

#### Windows
- **Availability**: Good - multiple distribution options
- **Installation Options**:
  1. **vcpkg** (Recommended for Visual Studio):
     ```cmd
     vcpkg install openssl:x64-windows
     ```
  2. **Precompiled Binaries**:
     - Win32/Win64 OpenSSL from https://slproweb.com/products/Win32OpenSSL.html
     - Install to C:\OpenSSL-Win64
  3. **MSYS2/MinGW**:
     ```bash
     pacman -S mingw-w64-x86_64-openssl
     ```
  4. **Build from Source**:
     ```cmd
     perl Configure VC-WIN64A --prefix=C:\OpenSSL
     nmake
     nmake install
     ```

- **Build Considerations**:
  - Link against Windows libraries:
    ```makefile
    # Visual Studio
    CFLAGS += /IC:\OpenSSL-Win64\include
    LDFLAGS += /LIBPATH:C:\OpenSSL-Win64\lib libssl.lib libcrypto.lib ws2_32.lib
    
    # MinGW
    CFLAGS += -IC:/OpenSSL-Win64/include
    LDFLAGS += -LC:/OpenSSL-Win64/lib -lssl -lcrypto -lws2_32
    ```
  - Use CMake for better Windows support:
    ```cmake
    find_package(OpenSSL REQUIRED)
    target_link_libraries(space-captain OpenSSL::SSL OpenSSL::Crypto ws2_32)
    ```

- **Code Changes Required**:
  ```c
  #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
    
    // Use closesocket() instead of close()
    #define close(s) closesocket(s)
    
    // Different error handling
    #define EAGAIN WSAEWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK
  #endif
  ```

- **Major Porting Challenges**:
  - Replace epoll with:
    - Windows I/O Completion Ports (IOCP) - best performance
    - WSAPoll() - similar to poll()
    - select() - portable but limited
  - Handle CRT differences (no unistd.h, different string functions)
  - Path separators (\ vs /)
  - Signal handling differences

### 9. Cross-Platform Considerations

#### CMake Build System
```cmake
cmake_minimum_required(VERSION 3.10)
project(SpaceCaptain C)

set(CMAKE_C_STANDARD 11)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Platform detection
if(WIN32)
    set(PLATFORM_SOURCES 
        src/network_win32.c
        src/iocp_server.c)
    set(PLATFORM_LIBS ws2_32)
    add_definitions(-D_WIN32_WINNT=0x0601)
elseif(APPLE)
    set(PLATFORM_SOURCES 
        src/network_macos.c
        src/kqueue_server.c)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY})
else()
    set(PLATFORM_SOURCES 
        src/network_linux.c
        src/epoll_server.c)
    set(PLATFORM_LIBS pthread)
endif()

# Main executable
add_executable(space-captain-server
    src/server.c
    src/queue.c
    src/worker.c
    ${PLATFORM_SOURCES}
)

target_include_directories(space-captain-server PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(space-captain-server
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PLATFORM_LIBS}
)

# Platform-specific compile options
if(MSVC)
    target_compile_options(space-captain-server PRIVATE /W4)
else()
    target_compile_options(space-captain-server PRIVATE -Wall -Wextra)
endif()
```

#### Portable Socket Abstraction
```c
// socket_compat.h
#ifndef SOCKET_COMPAT_H
#define SOCKET_COMPAT_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define get_socket_error() WSAGetLastError()
    #define close_socket(s) closesocket(s)
    #define SHUT_RDWR SD_BOTH
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1
    #define get_socket_error() errno
    #define close_socket(s) close(s)
#endif

#endif // SOCKET_COMPAT_H
```

### 10. Error Handling and Debugging

#### Enhanced Error Reporting
```c
static void log_ssl_error(const char *msg) {
    unsigned long err;
    char buf[256];
    
    log_error("%s", msg);
    
    while ((err = ERR_get_error()) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        log_error("OpenSSL error: %s", buf);
    }
}
```

#### Debug Callback
```c
static void ssl_info_callback(const SSL *ssl, int where, int ret) {
    const char *str;
    int w = where & ~SSL_ST_MASK;
    
    if (w & SSL_ST_CONNECT) str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT) str = "SSL_accept";
    else str = "undefined";
    
    if (where & SSL_CB_LOOP) {
        log_debug("%s: %s", str, SSL_state_string_long(ssl));
    } else if (where & SSL_CB_ALERT) {
        str = (where & SSL_CB_READ) ? "read" : "write";
        log_debug("SSL3 alert %s: %s:%s", str,
                  SSL_alert_type_string_long(ret),
                  SSL_alert_desc_string_long(ret));
    }
}

// Enable debug callback
SSL_CTX_set_info_callback(ctx, ssl_info_callback);
```

### 11. Testing Strategy

#### Unit Tests
```c
// Test certificate loading
void test_cert_loading(void) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    assert(ctx != NULL);
    
    int ret = SSL_CTX_use_certificate_file(ctx, "test_cert.pem", SSL_FILETYPE_PEM);
    assert(ret == 1);
    
    SSL_CTX_free(ctx);
}

// Test cipher configuration
void test_cipher_config(void) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    assert(ctx != NULL);
    
    int ret = SSL_CTX_set_cipher_list(ctx, TLS_CIPHERS);
    assert(ret == 1);
    
    SSL_CTX_free(ctx);
}
```

#### Performance Benchmarks
```bash
# OpenSSL speed test
openssl speed -evp aes-256-gcm

# Connection benchmark
openssl s_time -connect localhost:4242 -new -time 10

# Cipher suite benchmark
openssl s_client -connect localhost:4242 -cipher 'ECDHE-RSA-AES256-GCM-SHA384'
```

### 12. Security Considerations with Certificate Pinning

#### Certificate Validation with Pinning
```c
static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx) {
    // With pinning, we only accept the exact certificate we trust
    if (!preverify_ok) {
        int err = X509_STORE_CTX_get_error(ctx);
        log_error("Certificate verification failed: %s", 
                  X509_verify_cert_error_string(err));
        return 0; // Reject all verification failures
    }
    
    // Additional checks can be performed here
    return 1;
}

SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
                   verify_callback);
```

#### Enhanced Certificate Pinning
```c
static int pin_certificate(SSL *ssl, const char *expected_fingerprint) {
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert) return 0;
    
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int n;
    
    if (X509_digest(cert, EVP_sha256(), md, &n) != 1) {
        X509_free(cert);
        return 0;
    }
    
    char fingerprint[EVP_MAX_MD_SIZE * 3];
    for (unsigned int i = 0; i < n; i++) {
        sprintf(&fingerprint[i * 3], "%02X:", md[i]);
    }
    fingerprint[n * 3 - 1] = '\0';
    
    int match = strcmp(fingerprint, expected_fingerprint) == 0;
    X509_free(cert);
    
    if (!match) {
        log_error("Certificate fingerprint mismatch");
    }
    
    return match;
}

// Pin public key instead of certificate (more flexible)
static int pin_public_key(SSL *ssl, const unsigned char *expected_pubkey_hash) {
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert) return 0;
    
    EVP_PKEY *pkey = X509_get_pubkey(cert);
    if (!pkey) {
        X509_free(cert);
        return 0;
    }
    
    // Hash the public key and compare
    // Implementation details omitted for brevity
    
    EVP_PKEY_free(pkey);
    X509_free(cert);
    return 1;
}
```

#### Certificate Pinning Strategies

1. **Pin the Certificate**
   - Simple but requires update when certificate changes
   - Good for controlled environments

2. **Pin the Public Key**
   - Allows certificate renewal without client updates
   - Same key can be used with new certificates

3. **Pin Certificate Chain**
   - Pin intermediate or root CA
   - More flexible but less secure

4. **Backup Pins**
   - Include multiple pins (current + backup)
   - Enables certificate rotation without service disruption

### 13. Implementation Phases

#### Phase 1: Basic TLS Support
- Implement blocking TLS for proof of concept
- Test with single client
- Verify certificate handling

#### Phase 2: Non-blocking Integration
- Implement memory BIO approach
- Integrate with epoll event loop
- Handle partial reads/writes

#### Phase 3: Performance Optimization
- Enable session resumption
- Implement session ticket support
- Profile and optimize buffer sizes

#### Phase 4: Production Features
- Add OCSP stapling support
- Implement certificate rotation
- Add monitoring and metrics
- Support for hardware acceleration

### 14. OpenSSL vs mbed-tls Comparison

| Feature | OpenSSL | mbed-tls |
|---------|---------|----------|
| Memory Footprint | ~2-3 MB | ~45-300 KB |
| Performance | Excellent with HW accel | Good |
| Features | Comprehensive | Essential |
| Non-blocking Support | Complex but powerful | Simpler |
| Documentation | Extensive | Good |
| License | Apache 2.0 | Apache 2.0 |
| Hardware Acceleration | Extensive | Limited |
| FIPS Compliance | Available | No |

## Conclusion

OpenSSL integration into Space Captain would provide enterprise-grade TLS support with excellent performance characteristics. The main challenges are:

1. **Complexity**: OpenSSL's non-blocking model using memory BIOs requires careful implementation
2. **Memory Usage**: Higher memory footprint compared to mbed-tls (important for 5000+ connections)
3. **API Complexity**: More complex API requiring deeper understanding

However, OpenSSL offers:
- Better performance with hardware acceleration
- More comprehensive feature set
- Industry-standard implementation
- Better tooling and debugging support

For Space Captain's requirements, OpenSSL would be the better choice if:
- Hardware acceleration is available
- Advanced TLS features are needed
- Memory usage is not a primary constraint
- Long-term enterprise deployment is planned

The implementation would maintain the server's high-performance characteristics while adding robust security features.