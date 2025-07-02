# Main Server Loop Architecture

## Overview

The Space Captain server uses a high-performance architecture combining an epoll-based event loop, a thread-safe message queue, and a worker thread pool. This design allows the server to handle thousands of concurrent connections efficiently.

## Key Components

### 1. Epoll Event Loop (Main Thread)

The main thread runs an edge-triggered epoll loop that handles all network I/O:

```
Main Thread (server.c)
├── Creates listening socket
├── Initializes epoll instance
├── Pre-allocates connection pool
└── Runs event loop
    ├── epoll_wait() for events
    ├── Accept new connections
    ├── Read client data (non-blocking)
    └── Queue messages for processing
```

**Key characteristics:**
- **Edge-triggered mode (EPOLLET)**: More efficient but requires draining all available data
- **Non-blocking I/O**: Never blocks on network operations
- **Connection pooling**: Pre-allocates 5000 client buffers to avoid malloc during operation
- **Adaptive timeout**: Uses 0ms timeout when active, 100ms when idle

### 2. Message Queue (Thread-Safe)

A custom thread-safe queue connects the I/O thread with worker threads:

```
Message Queue (queue.c)
├── Bounded circular buffer
├── Read-write lock for thread safety
├── Condition variables for blocking operations
└── Operations
    ├── sc_queue_add() - Blocks if full
    ├── sc_queue_pop() - Blocks if empty
    ├── sc_queue_try_add() - Non-blocking
    └── sc_queue_try_pop() - Non-blocking
```

**Design features:**
- **Bounded capacity**: Prevents unbounded memory growth
- **Blocking semantics**: Workers sleep when queue is empty
- **Timeout support**: Can specify max wait time for operations

### 3. Worker Thread Pool

Multiple worker threads process messages concurrently:

```
Worker Threads (worker.c)
├── Created at startup (default: 4 threads)
├── Each thread runs worker_thread()
└── Processing loop
    ├── sc_queue_pop() - Block waiting for message
    ├── Process message based on type
    ├── Send response back to client
    └── Free message memory
```

**Message handlers:**
- `MSG_ECHO`: Returns the message unchanged
- `MSG_REVERSE`: Returns the message with content reversed
- `MSG_TIME`: Returns current timestamp

## Data Flow

Here's how a client request flows through the system:

```
1. Client sends message
   ↓
2. Epoll detects EPOLLIN event
   ↓
3. Main thread reads data into client buffer
   ↓
4. When complete message received:
   - Allocate message_t structure
   - Copy data and add client_fd
   - sc_queue_add() to message queue
   ↓
5. Worker thread wakes up
   - sc_queue_pop() gets message
   - Process based on message type
   - send() response directly to client_fd
   - Free message memory
   ↓
6. Client receives response
```

## Performance Optimizations

### 1. Zero-Copy Where Possible
- Client file descriptor is passed with message
- Workers send responses directly to clients
- No intermediate buffering for responses

### 2. Memory Management
- **Connection pool**: Pre-allocated buffers avoid malloc during operation
- **Message recycling**: Could be added for further optimization
- **Careful buffer sizing**: Avoids reallocation

### 3. Concurrency Design
- **Single reader**: Only main thread reads from sockets
- **Multiple processors**: Workers handle CPU-intensive tasks
- **Lock-free where possible**: Epoll operations don't need locks

### 4. Network Optimizations
- **TCP_NODELAY**: Disabled Nagle's algorithm for lower latency
- **EPOLLRDHUP**: Detect half-closed connections efficiently
- **Edge-triggered epoll**: Reduces system calls

## Handling Edge Cases

### Client Disconnection
```c
if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
    // Clean up client state
    remove_client_buffer(events[i].data.fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
    close(events[i].data.fd);
}
```

### Queue Full
- Main thread uses blocking `sc_queue_add()`
- Naturally provides backpressure
- Prevents accepting data faster than processing

### Partial Messages
- Each client has a state machine tracking:
  - `READING_HEADER`: Accumulating 8-byte header
  - `READING_BODY`: Accumulating message body
- Buffers store partial data between epoll events

## Configuration

Key constants in `config.h`:
- `MAX_EVENTS`: 10000 - Maximum epoll events per wait
- `MAX_CONNECTIONS`: 5000 - Size of connection pool
- `WORKER_POOL_SIZE`: 4 - Number of worker threads
- `QUEUE_CAPACITY`: 1000 - Maximum queued messages
- `EPOLL_TIMEOUT_MS`: 100 - Timeout when idle

## Shutdown Sequence

Graceful shutdown on SIGINT/SIGTERM:

1. Set `shutdown_flag` in signal handler
2. Main loop exits on next iteration
3. Stop accepting new connections
4. Call `worker_pool_stop()` - signals workers
5. Workers drain queue and exit
6. Clean up remaining messages
7. Free all resources

## Monitoring and Debugging

The server logs key events:
- Connection accepted/closed
- Message received/sent
- Worker pool status
- Queue operations (when errors occur)
- Performance warnings (e.g., connection pool exhausted)

## Common Patterns

### Adding a New Message Type

1. Add to `message_type_t` enum in `message.h`
2. Update `message_type_to_string()`
3. Add handler function in `worker.c`
4. Add case in `worker_thread()` switch statement

### Scaling Considerations

To handle more connections:
1. Increase `MAX_CONNECTIONS` and `MAX_EVENTS`
2. Adjust `WORKER_POOL_SIZE` based on workload
3. Consider multiple accept threads (not currently implemented)
4. Add message batching for better throughput

## Summary

This architecture provides:
- **High concurrency**: Handles 5000+ connections
- **Low latency**: Direct worker-to-client responses
- **Scalability**: Easy to adjust worker count
- **Reliability**: Graceful shutdown and error handling
- **Simplicity**: Clear separation of concerns

The epoll loop handles all I/O efficiently, the message queue decouples I/O from processing, and the worker pool provides parallel message handling - creating a robust and performant server architecture.