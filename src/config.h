#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Server configuration
#define SERVER_PORT      4242
#define SERVER_HOST      "127.0.0.1"
#define SERVER_BACKLOG   128
#define WORKER_POOL_SIZE 4
#define QUEUE_CAPACITY   1024

// Event buffer configuration
#define MIN_EVENTS          1024  // Minimum event buffer size
#define MAX_EVENTS_PER_CONN 2     // Expected events per connection
#define ABSOLUTE_MAX_EVENTS 65536 // Cap to prevent excessive memory use

// Client configuration
#define MIN_DELAY_MS 300
#define MAX_DELAY_MS 1000

// Message configuration
#define MAX_MESSAGE_SIZE 4096
#define BUFFER_SIZE      4096

// Timing configuration
#define EPOLL_TIMEOUT_MS    10   // Epoll wait timeout in milliseconds
#define WORKER_SLEEP_MS     10   // Worker sleep time when queue is empty (milliseconds)
#define READ_RETRY_DELAY_US 1000 // Delay before retrying read on EAGAIN (microseconds)

// Network configuration
#define CLIENT_FD_SIZE sizeof(int32_t) // Size of client fd stored in message body

// Connection pool configuration
#define CONNECTION_POOL_SIZE 5000 // Pre-allocated connections for 5k clients
#define CONNECTION_POOL_GROW 100  // Additional connections to allocate if pool exhausted

#endif // CONFIG_H