# Worker Thread Pool Implementation FAQ

## Overview

The Space Captain project implements a worker thread pool that processes messages from the MPMC queue. This pool provides concurrent message handling, allowing the main server thread to focus on network I/O while worker threads handle the actual message processing logic.

## Architecture & Design

### What is the worker pool structure?

The worker pool consists of two main structures:

```c
typedef struct {
  int32_t id;
  pthread_t thread;
  queue_t *msg_queue;
  volatile bool *shutdown_flag;
} sc_worker_context_t;

typedef struct {
  sc_worker_context_t *workers;
  int32_t pool_size;
  queue_t *msg_queue;
  volatile bool shutdown_flag;
} sc_worker_pool_t;
```

### How does the worker pool operate?

1. **Initialization**: A pool of worker threads is created, each with its own context
2. **Message Processing**: Workers continuously poll the shared message queue
3. **Load Distribution**: Messages are automatically distributed among available workers
4. **Graceful Shutdown**: Workers check a shutdown flag and exit cleanly

### Why use a worker thread pool?

- **Separation of Concerns**: Network I/O is separated from message processing
- **Concurrency**: Multiple messages can be processed simultaneously
- **Scalability**: Pool size can be adjusted based on workload
- **Non-blocking Server**: Main thread never blocks on message processing

## Lifecycle Management

### How is a worker pool created?

```c
sc_worker_pool_t *sc_worker_pool_init(int32_t pool_size, queue_t *msg_queue)
```

The initialization process:

1. **Validation**: Ensures pool_size > 0 and msg_queue is not NULL
2. **Memory Allocation**: 
   - Allocates the pool structure
   - Allocates array of worker contexts
3. **Context Setup**: Each worker gets:
   - Unique ID (0 to pool_size-1)
   - Reference to shared message queue
   - Pointer to shared shutdown flag
4. **Initial State**: shutdown_flag set to false

### How is a worker pool destroyed?

```c
void sc_worker_pool_nuke(sc_worker_pool_t *pool)
```

- Frees the worker contexts array
- Frees the pool structure
- **Note**: Does not stop running threads - call `sc_worker_pool_stop()` first

## Worker Operations

### Starting the Worker Pool

```c
void sc_worker_pool_start(sc_worker_pool_t *pool)
```

1. Iterates through all worker contexts
2. Creates a new thread for each worker
3. Each thread runs the `sc_worker_thread` function
4. Logs success/failure for each thread creation

### Stopping the Worker Pool

```c
void sc_worker_pool_stop(sc_worker_pool_t *pool)
```

1. Sets the shutdown flag to true
2. Waits for all worker threads to finish (pthread_join)
3. Ensures clean shutdown of all workers
4. Logs when each worker stops

### Worker Thread Function

```c
void *sc_worker_thread(void *arg)
```

The main worker loop:

1. **Message Retrieval**: Attempts to pop a message from the queue (non-blocking)
2. **Message Processing**: 
   - Extracts client file descriptor from message
   - Routes to appropriate handler based on message type
   - Frees the message after processing
3. **Idle Behavior**: Sleeps briefly (WORKER_SLEEP_MS) when queue is empty
4. **Shutdown Check**: Exits loop when shutdown flag is set

## Message Handling

### Message Structure

Messages in the queue contain:
- Client file descriptor (first 4 bytes of body)
- Actual message content (remaining body)
- Message type in header

### Message Type Handlers

#### Echo Handler
```c
void sc_worker_handle_echo_message(int32_t client_fd, message_t *msg)
```
- Sends back the exact message received
- Used for testing and connectivity verification

#### Reverse Handler
```c
void sc_worker_handle_reverse_message(int32_t client_fd, message_t *msg)
```
- Reverses the string in the message body
- Allocates new buffer for reversed string
- Sends reversed string back to client

#### Time Handler
```c
void sc_worker_handle_time_message(int32_t client_fd, message_t *msg)
```
- Generates current UTC time in ISO8601 format
- Format: "YYYY-MM-DDTHH:MM:SSZ"
- Sends timestamp back to client

### Response Sending

All handlers use a common helper function:
```c
static void send_response(int32_t client_fd, message_type_t type, const char *response)
```

1. Creates response header with type and length
2. Sends header first
3. Sends body second
4. Logs the response details

## Configuration

### Pool Size
- Defined by `WORKER_POOL_SIZE` in config.h
- Default: 4 workers
- Should be tuned based on expected workload

### Worker Sleep Time
- Defined by `WORKER_SLEEP_MS` in config.h
- Default: 10 milliseconds
- Prevents busy-waiting when queue is empty

### Client FD Size
- `CLIENT_FD_SIZE`: 4 bytes (sizeof(int32_t))
- Prepended to all messages in the queue

## Error Handling

### Thread Creation Failures
- Logged but non-fatal
- Pool continues with successfully created threads
- Server can operate with partial worker pool

### Queue Operation Errors
- `QUEUE_ERR_EMPTY`: Normal condition, worker sleeps
- `QUEUE_ERR_TIMEOUT`: Logged as error
- Other errors: Logged with error code

### Message Processing Errors
- Memory allocation failures: Logged and return early
- Send failures: Logged but processing continues
- Unknown message types: Logged as errors

## Performance Characteristics

### Concurrency Model
- **True Parallelism**: Workers run on separate OS threads
- **Lock Contention**: Minimized by MPMC queue design
- **Work Stealing**: Not implemented (simple FIFO processing)

### Scalability
- Linear scaling up to number of CPU cores
- Beyond CPU count: Diminishing returns due to context switching
- Queue becomes bottleneck at very high thread counts

### Memory Usage
- Fixed overhead per worker: thread stack + context structure
- No dynamic allocations during normal operation
- Message memory managed by queue and freed after processing

## Common Usage Patterns

### Server Integration
```c
// Create message queue
queue_t *msg_queue = sc_queue_init(QUEUE_CAPACITY);

// Create and start worker pool
sc_worker_pool_t *pool = sc_worker_pool_init(WORKER_POOL_SIZE, msg_queue);
sc_worker_pool_start(pool);

// Main server loop adds messages to queue
// ...

// Shutdown sequence
sc_worker_pool_stop(pool);
sc_worker_pool_nuke(pool);
sc_queue_nuke(msg_queue);
```

### Adding New Message Types
1. Define new message type in message.h
2. Add case in sc_worker_thread switch statement
3. Implement handler function following existing pattern
4. Update message_type_to_string for logging

### Custom Worker Behavior
```c
// Example: Priority processing
while (!*ctx->shutdown_flag) {
    message_t *msg = NULL;
    sc_queue_ret_val_t result = sc_queue_try_pop(ctx->msg_queue, &msg);
    
    if (result == QUEUE_SUCCESS) {
        // Add priority logic here
        if (is_high_priority(msg)) {
            process_immediately(msg);
        } else {
            process_normally(msg);
        }
    }
}
```

## Design Decisions & Trade-offs

### Why Non-blocking Queue Operations?

- **Responsive Shutdown**: Workers can check shutdown flag regularly
- **Fair Processing**: No worker blocks others indefinitely
- **Debugging**: Easier to track worker activity

### Why Sleep on Empty Queue?

- **CPU Efficiency**: Prevents 100% CPU usage when idle
- **Battery Life**: Important for laptop/mobile deployments
- **Trade-off**: Adds small latency (10ms) to message processing

### Why Separate Thread per Worker?

- **True Parallelism**: Utilizes multiple CPU cores
- **OS Scheduling**: Leverages OS thread scheduler
- **Simplicity**: No need for custom scheduling logic
- **Trade-off**: Higher memory usage than green threads

### Why Fixed Pool Size?

- **Predictability**: Known resource usage
- **Simplicity**: No complex scaling logic
- **Stability**: Avoids thread creation/destruction overhead
- **Trade-off**: Cannot adapt to varying workloads

## Thread Safety Guarantees

- **Queue Operations**: Fully thread-safe via MPMC queue
- **Shutdown Flag**: Volatile ensures visibility across threads
- **Worker Contexts**: Read-only after initialization
- **Message Ownership**: Clear transfer from queue to worker
- **Socket Operations**: Each worker handles its own client sockets

## Debugging & Monitoring

### Log Messages
- Worker start/stop events
- Message processing (type, client fd)
- Errors with context (worker ID, error codes)
- Response details in debug mode

### Common Issues
1. **Workers not processing**: Check queue is not full
2. **High CPU usage**: Verify WORKER_SLEEP_MS is set
3. **Messages delayed**: Check pool size vs workload
4. **Memory leaks**: Ensure messages are freed after processing