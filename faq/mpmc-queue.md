# MPMC Queue Implementation FAQ

## Overview

The Space Captain project implements a thread-safe Multi-Producer Multi-Consumer (MPMC) queue that allows multiple threads to safely add and remove messages concurrently. This queue is a critical component for the server's worker thread pool architecture, enabling efficient message passing between the main server thread and worker threads.

## Architecture & Design

### What is the queue's internal structure?

The queue (`queue_t`) uses a circular buffer design with the following components:

```c
typedef struct {
  message_t **buffer;            // Circular buffer of message pointers
  size_t capacity;               // Maximum number of messages
  size_t size;                   // Current number of messages
  size_t head;                   // Index of next message to remove
  size_t tail;                   // Index where next message will be added
  pthread_rwlock_t rwlock;       // Reader-writer lock for data access
  pthread_mutex_t cond_mutex;    // Separate mutex for condition variables
  pthread_cond_t cond_not_empty; // Signaled when queue becomes non-empty
  pthread_cond_t cond_not_full;  // Signaled when queue becomes non-full
} queue_t;
```

### Why use a circular buffer?

The circular buffer provides O(1) performance for both enqueue and dequeue operations. The `head` and `tail` indices wrap around using modulo arithmetic (`% capacity`), eliminating the need to shift elements and providing consistent performance regardless of queue size.

### What synchronization mechanisms are used?

The queue employs a sophisticated two-lock design:

1. **Reader-Writer Lock (`rwlock`)**: Protects the queue's data structure during operations
2. **Condition Mutex (`cond_mutex`)**: Separate mutex specifically for condition variable operations

This design prevents deadlocks that could occur if a single lock was used for both data protection and condition waiting.

## Lifecycle Management

### How is a queue created?

```c
queue_t *sc_queue_init(size_t capacity)
```

The creation process:

1. **Validation**: Checks capacity is > 0 and doesn't exceed `QUEUE_MAX_CAPACITY`
2. **Memory Allocation**: 
   - Allocates the queue structure using `calloc` (zero-initialized)
   - Allocates the circular buffer array for message pointers
   - Checks for integer overflow in buffer size calculation
3. **Initialization**:
   - Sets capacity, size=0, head=0, tail=0
   - Initializes pthread synchronization primitives (rwlock, mutex, conditions)
4. **Error Handling**: Properly cleans up on any failure, returning NULL

### How is a queue destroyed?

Two destruction methods are provided:

#### Basic Destruction
```c
sc_queue_ret_val_t sc_queue_exit(queue_t *q)
```
- Destroys synchronization primitives
- Frees the buffer and queue structure
- **Important**: Does NOT free messages still in the queue

#### Destruction with Cleanup
```c
void sc_queue_exit_with_cleanup(queue_t *q, queue_cleanup_fn cleanup_fn, void *user_data)
```
- Locks the queue to prevent new operations
- Drains all remaining messages
- Calls the cleanup function for each message (if provided)
- Then performs normal destruction

## Core Operations

### How does message addition work?

#### Blocking Add
```c
sc_queue_ret_val_t sc_queue_add(queue_t *q, message_t *msg)
```

1. Acquires write lock on the queue
2. If queue is full:
   - Releases the write lock
   - Waits on `cond_not_full` condition with timeout
   - Re-acquires write lock when space becomes available
3. Adds message at tail position
4. Updates tail index: `tail = (tail + 1) % capacity`
5. Increments size
6. Signals `cond_not_empty` for waiting consumers

#### Non-blocking Add
```c
sc_queue_ret_val_t sc_queue_try_add(queue_t *q, message_t *msg)
```

- Same as blocking add but returns `QUEUE_ERR_FULL` immediately if full
- Never waits or blocks

### How does message removal work?

#### Blocking Pop
```c
sc_queue_ret_val_t sc_queue_pop(queue_t *q, message_t **msg)
```

1. Acquires write lock on the queue
2. If queue is empty:
   - Releases the write lock
   - Waits on `cond_not_empty` condition with timeout
   - Re-acquires write lock when message becomes available
3. Retrieves message from head position
4. Updates head index: `head = (head + 1) % capacity`
5. Decrements size
6. Signals `cond_not_full` for waiting producers

#### Non-blocking Pop
```c
sc_queue_ret_val_t sc_queue_try_pop(queue_t *q, message_t **msg)
```

- Same as blocking pop but returns `QUEUE_ERR_EMPTY` immediately if empty
- Never waits or blocks

## Status Functions

### Queue State Queries

All status functions use read locks for thread-safety:

- `sc_queue_is_empty(queue_t *q)`: Returns true if size == 0
- `sc_queue_is_full(queue_t *q)`: Returns true if size == capacity
- `sc_queue_get_size(queue_t *q)`: Returns current number of messages

## Error Handling

### Thread-Local Error Storage

The queue uses thread-local storage for error codes:

```c
static __thread int queue_errno = QUEUE_SUCCESS;
```

This allows each thread to have its own error state without interference.

### Error Codes

- `QUEUE_SUCCESS` (0): Operation completed successfully
- `QUEUE_ERR_TIMEOUT` (-1): Blocking operation timed out
- `QUEUE_ERR_THREAD` (-2): pthread operation failed
- `QUEUE_ERR_NULL` (-3): NULL pointer parameter
- `QUEUE_ERR_MEMORY` (-4): Memory allocation failed
- `QUEUE_ERR_FULL` (-5): Queue full (try_add)
- `QUEUE_ERR_EMPTY` (-6): Queue empty (try_pop)
- `QUEUE_ERR_INVALID` (-7): Invalid parameter
- `QUEUE_ERR_OVERFLOW` (-8): Integer overflow

### Error Functions

- `sc_queue_get_error()`: Get last error for current thread
- `sc_queue_clear_error()`: Clear error state
- `sc_queue_strerror(int err)`: Get human-readable error message

## Configuration

### Timeouts

- `QUEUE_POP_TIMEOUT`: 2 seconds for blocking pop operations
- `QUEUE_ADD_TIMEOUT`: 2 seconds for blocking add operations

### Capacity Limits

- Maximum capacity: `SIZE_MAX / sizeof(message_t *) / 2`
- Prevents integer overflow in buffer allocation

## Performance Characteristics

### Time Complexity

- Add/Pop operations: O(1) for all variants
- Status queries: O(1)

### Lock Contention

The two-lock design minimizes contention:
- Data operations hold rwlock briefly
- Condition waiting uses separate mutex
- Readers can access status concurrently

### Memory Usage

- Fixed memory allocation based on capacity
- No dynamic resizing (predictable memory footprint)
- Stores pointers only (messages allocated separately)

## Common Usage Patterns

### Producer Thread
```c
message_t *msg = create_message(...);
sc_queue_ret_val_t result = sc_queue_add(queue, msg);
if (result != QUEUE_SUCCESS) {
    // Handle error - timeout or other failure
    message_destroy(msg);
}
```

### Consumer Thread
```c
message_t *msg = NULL;
sc_queue_ret_val_t result = sc_queue_pop(queue, &msg);
if (result == QUEUE_SUCCESS) {
    // Process message
    process_message(msg);
    message_destroy(msg);
}
```

### Graceful Shutdown
```c
// Custom cleanup function
void cleanup_message(message_t *msg, void *user_data) {
    log_info("Cleaning up message type: %d", msg->type);
    message_destroy(msg);
}

// Destroy queue with cleanup
sc_queue_exit_with_cleanup(queue, cleanup_message, NULL);
```

## Design Decisions & Trade-offs

### Why Reader-Writer Lock?

While all current operations use write locks, the rwlock provides:
- Future extensibility for read-only operations
- Better semantics (distinguishes read vs write intent)
- Potential for concurrent status queries

### Why Separate Condition Mutex?

Prevents deadlock scenarios where:
1. Thread A holds rwlock, waits on condition
2. Thread B needs rwlock to signal condition
3. Result: deadlock

The separate mutex allows clean handoff between data protection and condition waiting.

### Why Fixed Capacity?

- Predictable memory usage
- No allocation during operation (better real-time behavior)  
- Simpler implementation with guaranteed O(1) operations
- Suitable for bounded producer-consumer scenarios

## Thread Safety Guarantees

- All operations are fully thread-safe
- Multiple producers and consumers can operate concurrently
- No data races or undefined behavior
- Timeout mechanisms prevent indefinite blocking
- Error states are thread-local (no interference between threads)