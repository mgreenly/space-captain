# Mbed TLS Integration Research for Space Captain

This document provides an overview of how the mbed-tls library could be integrated into the Space Captain server and client to provide TLS encryption using self-signed certificates.

## Overview

Mbed TLS is a lightweight, portable cryptographic library that provides SSL/TLS support for embedded systems and applications. It's well-suited for the Space Captain project due to its:
- Small memory footprint (45KB - 300KB depending on features)
- Easy integration with existing socket-based code
- Support for self-signed certificates
- Configurable security features

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

## Proposed TLS Integration

### 1. Certificate Generation

Generate self-signed certificates using mbed-tls tools:

```bash
# Generate RSA key
programs/pkey/gen_key type=rsa rsa_keysize=4096 filename=server.key

# Generate self-signed certificate
programs/x509/cert_write selfsign=1 issuer_key=server.key \
    issuer_name=CN=spacecaptain,O=SpaceCaptain,C=US \
    not_before=20240101000000 not_after=20340101000000 \
    is_ca=0 output_file=server.crt
```

### 2. Required Headers

```c
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
```

### 3. Server-Side Integration

#### TLS Context Structure
```c
typedef struct {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
} tls_context_t;
```

#### Modified Client Buffer Structure
```c
typedef struct client_buffer {
    int fd;
    mbedtls_ssl_context *ssl;  // Add TLS context
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

#### Key Integration Points

1. **Server Initialization**
   - Initialize global TLS context
   - Load server certificate and private key
   - Configure TLS parameters

2. **Connection Acceptance**
   - After accept(), perform TLS handshake
   - Use mbedtls_ssl_handshake() in non-blocking mode
   - Add handshake state to epoll monitoring

3. **Data Reading/Writing**
   - Replace recv() with mbedtls_ssl_read()
   - Replace send() with mbedtls_ssl_write()
   - Handle SSL_WANT_READ/WRITE for non-blocking operations

### 4. Client-Side Integration

#### TLS Initialization
```c
static int init_tls_client(tls_context_t *tls) {
    mbedtls_ssl_init(&tls->ssl);
    mbedtls_ssl_config_init(&tls->conf);
    mbedtls_x509_crt_init(&tls->cert);
    mbedtls_ctr_drbg_init(&tls->ctr_drbg);
    mbedtls_entropy_init(&tls->entropy);
    
    // Initialize RNG
    mbedtls_ctr_drbg_seed(&tls->ctr_drbg, mbedtls_entropy_func,
                          &tls->entropy, NULL, 0);
    
    // Configure TLS
    mbedtls_ssl_config_defaults(&tls->conf,
                                MBEDTLS_SSL_IS_CLIENT,
                                MBEDTLS_SSL_TRANSPORT_STREAM,
                                MBEDTLS_SSL_PRESET_DEFAULT);
    
    // For self-signed certificates
    mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    
    mbedtls_ssl_conf_rng(&tls->conf, mbedtls_ctr_drbg_random, &tls->ctr_drbg);
    
    return 0;
}
```

#### Modified Connection Function
```c
static int connect_to_server_tls(tls_context_t *tls) {
    // Create socket as before
    int sock = connect_to_server();
    
    // Setup TLS
    mbedtls_ssl_setup(&tls->ssl, &tls->conf);
    mbedtls_ssl_set_hostname(&tls->ssl, SERVER_HOST);
    mbedtls_ssl_set_bio(&tls->ssl, &sock,
                        mbedtls_net_send, mbedtls_net_recv, NULL);
    
    // Perform handshake
    int ret;
    while ((ret = mbedtls_ssl_handshake(&tls->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && 
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return -1;
        }
    }
    
    return sock;
}
```

### 5. Non-blocking Considerations

For the server's epoll-based architecture:

1. **Handshake State Machine**
   - Track handshake progress in client_buffer
   - Handle MBEDTLS_ERR_SSL_WANT_READ/WRITE
   - Re-register epoll events based on TLS needs

2. **Event Handling**
   ```c
   if (events[i].events & EPOLLIN) {
       if (client->handshake_complete) {
           handle_client_data_tls(client_fd, msg_queue);
       } else {
           continue_handshake(client_fd);
       }
   }
   ```

### 6. Certificate Management

#### Development Setup
1. Generate one self-signed certificate for the server
2. Distribute certificate to clients for pinning
3. Store in `certs/` directory:
   ```
   certs/
   ├── server.crt
   ├── server.key
   └── ca.crt (copy of server.crt for client trust)
   ```

#### Production Considerations
- Implement certificate rotation mechanism
- Consider certificate pinning for additional security
- Add certificate validation callbacks for custom checks

### 7. Configuration Updates

Add to `config.h`:
```c
// TLS Configuration
#define TLS_ENABLED 1
#define TLS_CERT_PATH "certs/server.crt"
#define TLS_KEY_PATH "certs/server.key"
#define TLS_CA_PATH "certs/ca.crt"
#define TLS_HANDSHAKE_TIMEOUT_MS 5000
```

### 8. Build System Integration

Update Makefile:
```makefile
# Add mbed-tls flags
CFLAGS += -I/path/to/mbedtls/include
LDFLAGS += -L/path/to/mbedtls/library -lmbedtls -lmbedx509 -lmbedcrypto
```

### 9. Platform Support

#### Linux
- **Availability**: Excellent - available in most package managers
- **Installation**: 
  ```bash
  # Debian/Ubuntu
  sudo apt-get install libmbedtls-dev
  
  # Red Hat/CentOS
  sudo yum install mbedtls-devel
  
  # Arch Linux
  sudo pacman -S mbedtls
  ```
- **Build**: Standard Unix build works perfectly
- **Notes**: epoll-based server code is Linux-specific

#### macOS
- **Availability**: Good - available through Homebrew and MacPorts
- **Installation**:
  ```bash
  # Homebrew
  brew install mbedtls
  
  # MacPorts
  sudo port install mbedtls
  ```
- **Build Considerations**:
  - Replace epoll with kqueue for event handling
  - Different socket headers may be needed
  - Use `pkg-config` for proper linking:
    ```makefile
    CFLAGS += $(shell pkg-config --cflags mbedtls)
    LDFLAGS += $(shell pkg-config --libs mbedtls)
    ```
- **Code Changes Required**:
  ```c
  #ifdef __APPLE__
    #include <sys/event.h>  // kqueue instead of epoll
    #include <sys/time.h>
  #endif
  ```

#### Windows
- **Availability**: Good - can be built from source or use pre-built binaries
- **Installation Options**:
  1. **vcpkg** (Recommended):
     ```cmd
     vcpkg install mbedtls:x64-windows
     ```
  2. **Manual Build**:
     ```cmd
     git clone https://github.com/Mbed-TLS/mbedtls.git
     cd mbedtls
     mkdir build && cd build
     cmake -G "Visual Studio 16 2019" -A x64 ..
     cmake --build . --config Release
     ```
  3. **Pre-built binaries**: Available from various third parties

- **Build Considerations**:
  - Use CMake for cross-platform build
  - Link against Windows socket library:
    ```cmake
    target_link_libraries(space-captain mbedtls mbedx509 mbedcrypto ws2_32)
    ```
  - Different socket API (Winsock2 vs BSD sockets)
  
- **Code Changes Required**:
  ```c
  #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  #else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
  #endif
  ```

- **Major Porting Challenges**:
  - Replace epoll with Windows I/O Completion Ports (IOCP) or select()
  - Handle differences in non-blocking socket behavior
  - Different error codes (WSAGetLastError() vs errno)
  - No fork() - use Windows threads or processes

### 10. Cross-Platform Build System

Consider using CMake for better cross-platform support:

```cmake
cmake_minimum_required(VERSION 3.10)
project(SpaceCaptain C)

set(CMAKE_C_STANDARD 11)

# Find mbed TLS
find_package(MbedTLS REQUIRED)

# Platform-specific sources
if(WIN32)
    set(PLATFORM_SOURCES src/network_win32.c)
    set(PLATFORM_LIBS ws2_32)
elseif(APPLE)
    set(PLATFORM_SOURCES src/network_macos.c)
elseif(UNIX)
    set(PLATFORM_SOURCES src/network_linux.c)
endif()

# Main executable
add_executable(space-captain-server
    src/server.c
    src/queue.c
    src/worker.c
    ${PLATFORM_SOURCES}
)

target_link_libraries(space-captain-server
    MbedTLS::mbedtls
    MbedTLS::mbedx509
    MbedTLS::mbedcrypto
    ${PLATFORM_LIBS}
)

# Platform-specific compile definitions
if(WIN32)
    target_compile_definitions(space-captain-server PRIVATE _WIN32_WINNT=0x0601)
endif()
```

### 11. Testing Strategy

1. **Unit Tests**
   - Test certificate loading
   - Test TLS initialization
   - Mock TLS operations

2. **Integration Tests**
   - Modify `server_client_tests.c` to support TLS
   - Test with varying client counts
   - Verify performance impact

3. **Performance Testing**
   - Measure handshake overhead
   - Compare throughput with/without TLS
   - Monitor CPU usage increase

### 12. Security Considerations

1. **Self-Signed Certificate Risks**
   - No chain of trust
   - Vulnerable to MITM on first connection
   - Requires secure certificate distribution

2. **Mitigations**
   - Implement certificate pinning
   - Use strong key sizes (4096-bit RSA or P-256 ECC)
   - Regular certificate rotation
   - Consider TOFU (Trust On First Use) model

3. **Configuration Hardening**
   - Disable weak cipher suites
   - Enforce minimum TLS version (1.2+)
   - Enable session resumption for performance

### 13. Implementation Phases

#### Phase 1: Basic TLS Support
- Implement TLS for new connections
- Support blocking mode first
- Test with single client

#### Phase 2: Non-blocking Integration
- Integrate with epoll event loop
- Handle partial handshakes
- Support concurrent TLS connections

#### Phase 3: Performance Optimization
- Implement session caching
- Optimize buffer management
- Profile and tune TLS parameters

#### Phase 4: Production Hardening
- Add monitoring/metrics
- Implement certificate rotation
- Add configuration options

## Conclusion

Integrating mbed-tls into Space Captain is feasible and would provide strong encryption for client-server communication. The main challenges are:

1. Adapting TLS to the non-blocking epoll architecture
2. Managing certificate distribution for self-signed certs
3. Maintaining performance with 5000+ concurrent connections

The modular nature of mbed-tls and its low memory footprint make it well-suited for this integration. With careful implementation of the non-blocking TLS operations, the server should maintain its high-performance characteristics while adding security.