# MPMC Generic Queue Implementation FAQ

## Overview

The Space Captain project implements a thread-safe, generic Multi-Producer Multi-Consumer (MPMC) FIFO (First-In-First-Out) queue. It allows multiple threads to safely add and remove `void*` pointers concurrently.

This generic queue is the foundation for the server's messaging system. It is wrapped by a type-safe implementation (`sc_message_queue_t`) for passing `message_t` pointers between the main server thread and worker threads, ensuring messages are processed in the order they are received.

## Architecture & Design

### What is the queue's internal structure?

The queue (`sc_generic_queue_t`) uses a circular buffer design with the following components:

```c
typedef struct {
  void **buffer;                 // Circular buffer of void pointers
  size_t capacity;               // Maximum number of items
  size_t size;                   // Current number of items
  size_t head;                   // Index of next item to remove
  size_t tail;                   // Index where next item will be added
  pthread_rwlock_t rwlock;       // Reader-writer lock for data access
  pthread_mutex_t cond_mutex;    // Separate mutex for condition variables
  pthread_cond_t cond_not_empty; // Signaled when queue becomes non-empty
  pthread_cond_t cond_not_full;  // Signaled when queue becomes non-full
} sc_generic_queue_t;
```

### Why use a circular buffer?

The circular buffer provides O(1) performance for both enqueue and dequeue operations. The `head` and `tail` indices wrap around using modulo arithmetic (`% capacity`), eliminating the need to shift elements and providing consistent performance regardless of queue size. This design naturally implements FIFO semantics:
- New items are added at the `tail` position (end of queue)
- Items are removed from the `head` position (front of queue)
- This ensures the first item added is the first item removed

### What synchronization mechanisms are used?

The queue employs a sophisticated two-lock design:

1. **Reader-Writer Lock (`rwlock`)**: Protects the queue's data structure during operations.
2. **Condition Mutex (`cond_mutex`)**: A separate mutex used exclusively for condition variable operations.

This design prevents deadlocks that could occur if a single lock was used for both data protection and condition waiting.

## Lifecycle Management

### How is a queue created?

```c
sc_generic_queue_t *sc_generic_queue_init(size_t capacity);
```

The creation process:

1. **Validation**: Checks that capacity is > 0 and does not exceed `SC_GENERIC_QUEUE_MAX_CAPACITY`.
2. **Memory Allocation**: 
   - Allocates the queue structure using `calloc` (zero-initialized).
   - Allocates the circular buffer array for `void*` pointers.
   - Checks for integer overflow in buffer size calculation.
3. **Initialization**:
   - Sets capacity, size=0, head=0, tail=0.
   - Initializes all pthread synchronization primitives (rwlock, mutex, conditions).
4. **Error Handling**: Properly cleans up all allocated resources on any failure, returning NULL.

### How is a queue destroyed?

Two destruction methods are provided:

#### Basic Destruction
```c
sc_generic_queue_ret_val_t sc_generic_queue_nuke(sc_generic_queue_t *q);
```
- Destroys synchronization primitives.
- Frees the buffer and the queue structure itself.
- **Important**: Does NOT free the items still in the queue. The caller is responsible for managing the memory of any remaining items.

#### Destruction with Cleanup
```c
void sc_generic_queue_nuke_with_cleanup(sc_generic_queue_t *q, sc_generic_queue_cleanup_fn cleanup_fn, void *user_data);
```
- Locks the queue to prevent any new operations.
- Drains all remaining items from the queue.
- Calls the user-provided `cleanup_fn` callback for each item.
- Then performs the normal destruction of the queue resources.

## Core Operations

### How does item addition work?

#### Blocking Add
```c
sc_generic_queue_ret_val_t sc_generic_queue_add(sc_generic_queue_t *q, void *item);
```

1. Acquires a write lock on the queue.
2. If the queue is full:
   - It releases the write lock.
   - It waits on the `cond_not_full` condition variable (with a timeout).
   - It re-acquires the write lock before attempting to add the item again.
3. Adds the item at the `tail` position.
4. Updates the tail index: `tail = (tail + 1) % capacity`.
5. Increments the size.
6. Signals the `cond_not_empty` condition to wake up any waiting consumers.

#### Non-blocking Add
```c
sc_generic_queue_ret_val_t sc_generic_queue_try_add(sc_generic_queue_t *q, void *item);
```

- Same as the blocking add, but if the queue is full, it returns `SC_GENERIC_QUEUE_ERR_FULL` immediately instead of waiting.

### How does item removal work?

#### Blocking Pop
```c
sc_generic_queue_ret_val_t sc_generic_queue_pop(sc_generic_queue_t *q, void **item);
```

1. Acquires a write lock on the queue.
2. If the queue is empty:
   - It releases the write lock.
   - It waits on the `cond_not_empty` condition variable (with a timeout).
   - It re-acquires the write lock before attempting to pop the item again.
3. Retrieves the item from the `head` position (FIFO - oldest item first).
4. Updates the head index: `head = (head + 1) % capacity`.
5. Decrements the size.
6. Signals the `cond_not_full` condition to wake up any waiting producers.

#### Non-blocking Pop
```c
sc_generic_queue_ret_val_t sc_generic_queue_try_pop(sc_generic_queue_t *q, void **item);
```

- Same as the blocking pop, but if the queue is empty, it returns `SC_GENERIC_QUEUE_ERR_EMPTY` immediately instead of waiting.

## Status Functions

### Queue State Queries

All status functions use read locks for thread-safety, allowing multiple threads to query the state concurrently without blocking writers unnecessarily.

- `sc_generic_queue_is_empty(sc_generic_queue_t *q)`: Returns true if size == 0.
- `sc_generic_queue_is_full(sc_generic_queue_t *q)`: Returns true if size == capacity.
- `sc_generic_queue_get_size(sc_generic_queue_t *q)`: Returns the current number of items.

## Error Handling

### Thread-Local Error Storage

The queue uses thread-local storage for error codes to ensure that error states are isolated between threads.

```c
static __thread sc_generic_queue_ret_val_t queue_errno = SC_GENERIC_QUEUE_SUCCESS;
```

### Error Codes

- `SC_GENERIC_QUEUE_SUCCESS` (0): Operation completed successfully.
- `SC_GENERIC_QUEUE_ERR_TIMEOUT` (-1): Blocking operation timed out.
- `SC_GENERIC_QUEUE_ERR_THREAD` (-2): A pthread operation failed.
- `SC_GENERIC_QUEUE_ERR_NULL` (-3): A required parameter was NULL.
- `SC_GENERIC_QUEUE_ERR_MEMORY` (-4): A memory allocation failed.
- `SC_GENERIC_QUEUE_ERR_FULL` (-5): Queue is full (for `try_add`).
- `SC_GENERIC_QUEUE_ERR_EMPTY` (-6): Queue is empty (for `try_pop`).
- `SC_GENERIC_QUEUE_ERR_INVALID` (-7): An invalid parameter was provided (e.g., capacity=0).
- `SC_GENERIC_QUEUE_ERR_OVERFLOW` (-8): Integer overflow during capacity calculation.

### Error Functions

- `sc_generic_queue_get_error()`: Get the last error code for the current thread.
- `sc_generic_queue_clear_error()`: Clear the error state for the current thread.
- `sc_generic_queue_strerror(sc_generic_queue_ret_val_t err)`: Get a human-readable error message.

## Type-Safe Wrappers (The `message_queue`)

While the generic queue is powerful, it is not type-safe. To solve this, the project uses a common C pattern: a thin, static inline wrapper that provides type safety.

The `message_queue.h` and `message_queue.c` files provide this wrapper for `message_t*`.

### Example: `sc_message_queue_add`

The wrapper function is simple:
```c
// In message_queue.h (often as a static inline function)
sc_message_queue_ret_val_t sc_message_queue_add(sc_message_queue_t *queue, message_t *msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_add(queue, (void *) msg);
}
```
This provides compile-time type checking, preventing accidental insertion of incorrect pointer types. With compiler optimizations like `-O2` or `-O3` and Link-Time Optimization (`-flto`), these wrapper functions are **inlined**, resulting in **zero performance overhead**.

## Configuration

### Timeouts

- `SC_GENERIC_QUEUE_POP_TIMEOUT`: 2 seconds for blocking pop operations.
- `SC_GENERIC_QUEUE_ADD_TIMEOUT`: 2 seconds for blocking add operations.

### Capacity Limits

- Maximum capacity: `SC_GENERIC_QUEUE_MAX_CAPACITY`, which is defined as `SIZE_MAX / sizeof(void *) / 2` to prevent integer overflow during buffer allocation.
