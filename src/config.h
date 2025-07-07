#ifndef CONFIG_H
#define CONFIG_H

// Protocol Configuration
#define PROTOCOL_VERSION 0x0001 // Protocol version for v0.1.0

// Server Configuration
#define SERVER_PORT            19840
#define EPOLL_MAX_EVENTS       64
#define SOCKET_BUFFER_SIZE     4096
#define CLIENT_TIMEOUT_SECONDS 30 // 30-second inactivity timeout

#endif // CONFIG_H
