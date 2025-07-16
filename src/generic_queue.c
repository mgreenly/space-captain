#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "generic_queue.h"
#include "log.h"

// ============================================================================
// Error Handling
// ============================================================================

// Thread-local error variable for queue operations
static __thread sc_generic_queue_ret_val_t queue_errno = SC_GENERIC_QUEUE_SUCCESS;

// Gets the last error code for the current thread
// @return The last error code set by a queue operation
sc_generic_queue_ret_val_t sc_generic_queue_get_error(void) {
  return queue_errno;
}

// Clears the error code for the current thread
void sc_generic_queue_clear_error(void) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;
}

// Gets a human-readable error message for the given error code
// @param err The error code to get a message for
// @return A string describing the error
const char *sc_generic_queue_strerror(sc_generic_queue_ret_val_t err) {
  switch (err) {
  case SC_GENERIC_QUEUE_SUCCESS:
    return "Success";
  case SC_GENERIC_QUEUE_ERR_TIMEOUT:
    return "Operation timed out";
  case SC_GENERIC_QUEUE_ERR_THREAD:
    return "Thread operation failed";
  case SC_GENERIC_QUEUE_ERR_NULL:
    return "Null pointer parameter";
  case SC_GENERIC_QUEUE_ERR_MEMORY:
    return "Memory allocation failed";
  case SC_GENERIC_QUEUE_ERR_FULL:
    return "Queue is full";
  case SC_GENERIC_QUEUE_ERR_EMPTY:
    return "Queue is empty";
  case SC_GENERIC_QUEUE_ERR_INVALID:
    return "Invalid parameter";
  case SC_GENERIC_QUEUE_ERR_OVERFLOW:
    return "Integer overflow in capacity calculation";
  default:
    return "Unknown error";
  }
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

// Calculates an absolute timeout from a relative timeout in seconds
// @param abs_timeout Pointer to timespec structure to store the absolute timeout
// @param timeout_seconds Number of seconds from now for the timeout
static void get_absolute_timeout(struct timespec *abs_timeout, int timeout_seconds) {
  clock_gettime(CLOCK_REALTIME, abs_timeout);
  abs_timeout->tv_sec += timeout_seconds;
}

// Const-correct wrapper for pthread_rwlock_rdlock
// @param rwlock Pointer to the read-write lock (const-correct)
// @return Same as pthread_rwlock_rdlock
static inline int pthread_rwlock_rdlock_const(const pthread_rwlock_t *rwlock) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
  return pthread_rwlock_rdlock((pthread_rwlock_t *) rwlock);
#pragma GCC diagnostic pop
}

// Const-correct wrapper for pthread_rwlock_unlock
// @param rwlock Pointer to the read-write lock (const-correct)
// @return Same as pthread_rwlock_unlock
static inline int pthread_rwlock_unlock_const(const pthread_rwlock_t *rwlock) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
  return pthread_rwlock_unlock((pthread_rwlock_t *) rwlock);
#pragma GCC diagnostic pop
}

// ============================================================================
// Queue Lifecycle Functions
// ============================================================================

// Creates a new thread-safe message queue with the specified capacity
// @param capacity Maximum number of messages the queue can hold (must be > 0)
// @return Pointer to the newly created queue, or NULL on failure
sc_generic_queue_t *sc_generic_queue_init(size_t capacity) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (capacity == 0 || capacity > SC_GENERIC_QUEUE_MAX_CAPACITY) {
    queue_errno = (capacity == 0) ? SC_GENERIC_QUEUE_ERR_INVALID : SC_GENERIC_QUEUE_ERR_OVERFLOW;
    log_error("Invalid capacity: %zu (max: %zu)", capacity, SC_GENERIC_QUEUE_MAX_CAPACITY);
    return NULL;
  }

  sc_generic_queue_t *q = calloc(1, sizeof(sc_generic_queue_t));
  if (!q) {
    queue_errno = SC_GENERIC_QUEUE_ERR_MEMORY;
    log_error("%s", "Failed to allocate memory for queue");
    return NULL;
  }
  // Check for overflow before allocating buffer
  size_t buffer_size;
  if (__builtin_mul_overflow(capacity, sizeof(void *), &buffer_size)) {
    queue_errno = SC_GENERIC_QUEUE_ERR_OVERFLOW;
    log_error("Integer overflow calculating buffer size for capacity %zu", capacity);
    free(q);
    return NULL;
  }

  q->buffer = calloc(capacity, sizeof(void *));
  if (!q->buffer) {
    queue_errno = SC_GENERIC_QUEUE_ERR_MEMORY;
    log_error("%s", "Failed to allocate memory for queue buffer");
    free(q);
    return NULL;
  }

  q->capacity = capacity;
  q->size     = 0;
  q->head     = 0;
  q->tail     = 0;

  // Initialize synchronization primitives
  if (pthread_rwlock_init(&q->rwlock, NULL) != 0) {
    queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize queue rwlock");
    free(q->buffer);
    free(q);
    return NULL;
  }

  if (pthread_mutex_init(&q->cond_mutex, NULL) != 0) {
    queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize condition mutex");
    pthread_rwlock_destroy(&q->rwlock);
    free(q->buffer);
    free(q);
    return NULL;
  }

  if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
    queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize cond_not_empty");
    pthread_mutex_destroy(&q->cond_mutex);
    pthread_rwlock_destroy(&q->rwlock);
    free(q->buffer);
    free(q);
    return NULL;
  }

  if (pthread_cond_init(&q->cond_not_full, NULL) != 0) {
    queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize cond_not_full");
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_mutex_destroy(&q->cond_mutex);
    pthread_rwlock_destroy(&q->rwlock);
    free(q->buffer);
    free(q);
    return NULL;
  }

  return q;
}

// Destroys a queue and frees its resources
// Note: Does not free items still in the queue - caller is responsible
// @param q Pointer to the queue to destroy
// @return QUEUE_SUCCESS on success, or an error code on failure
sc_generic_queue_ret_val_t sc_generic_queue_nuke(sc_generic_queue_t *q) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return SC_GENERIC_QUEUE_ERR_NULL;
  }
  // Note: This does not free the items themselves,
  // as ownership is transferred out of the queue on pop.
  // The caller is responsible for freeing items.
  int err = pthread_rwlock_destroy(&q->rwlock);
  if (err != 0) {
    queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
    log_error("Failed to destroy rwlock: %d", err);
  }

  pthread_mutex_destroy(&q->cond_mutex);

  free(q->buffer);
  pthread_cond_destroy(&q->cond_not_empty);
  pthread_cond_destroy(&q->cond_not_full);
  free(q);

  return queue_errno;
}

// Destroys a queue and applies a cleanup function to all remaining items
// @param q Pointer to the queue to destroy (must not be NULL)
// @param cleanup_fn Optional callback to process each remaining item (can be NULL)
// @param user_data Optional user data passed to the cleanup function
void sc_generic_queue_nuke_with_cleanup(sc_generic_queue_t *q,
                                        sc_generic_queue_cleanup_fn cleanup_fn, void *user_data) {
  if (q == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return;
  }
  // Lock the queue to prevent any new operations
  pthread_rwlock_wrlock(&q->rwlock);

  // Drain all remaining items
  while (q->size > 0) {
    void *item = q->buffer[q->head];
    q->head    = (q->head + 1) % q->capacity;
    q->size--;

    // Apply cleanup callback if provided
    if (cleanup_fn != NULL) {
      cleanup_fn(item, user_data);
    }
  }

  // Unlock before destroying synchronization primitives
  pthread_rwlock_unlock(&q->rwlock);

  // Clean up queue structure
  free(q->buffer);
  pthread_rwlock_destroy(&q->rwlock);
  pthread_mutex_destroy(&q->cond_mutex);
  pthread_cond_destroy(&q->cond_not_empty);
  pthread_cond_destroy(&q->cond_not_full);
  free(q);
}

// ============================================================================
// Queue Operations - Blocking
// ============================================================================

// Adds an item to the queue, blocking if the queue is full
// Will timeout after SC_GENERIC_QUEUE_ADD_TIMEOUT seconds if the queue remains full
// @param q Pointer to the queue (must not be NULL)
// @param item Pointer to the item to add (must not be NULL)
// @return QUEUE_SUCCESS on success, or an error code on failure
sc_generic_queue_ret_val_t sc_generic_queue_add(sc_generic_queue_t *q, void *item) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL || item == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return SC_GENERIC_QUEUE_ERR_NULL;
  }

  pthread_rwlock_wrlock(&q->rwlock);

  while (q->size == q->capacity) {
    // Queue is full, must unlock rwlock before waiting on condition
    pthread_rwlock_unlock(&q->rwlock);

    // Use condition mutex for waiting
    pthread_mutex_lock(&q->cond_mutex);

    struct timespec timeout;
    get_absolute_timeout(&timeout, SC_GENERIC_QUEUE_ADD_TIMEOUT);

    int result = pthread_cond_timedwait(&q->cond_not_full, &q->cond_mutex, &timeout);
    pthread_mutex_unlock(&q->cond_mutex);

    if (result == ETIMEDOUT) {
      log_error("sc_generic_queue_add timed out after %d seconds", SC_GENERIC_QUEUE_ADD_TIMEOUT);
      queue_errno = SC_GENERIC_QUEUE_ERR_TIMEOUT;
      return SC_GENERIC_QUEUE_ERR_TIMEOUT;
    }
    if (result != 0) {
      log_error("sc_generic_queue_add pthread_cond_timedwait failed: %d", result);
      queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
      return SC_GENERIC_QUEUE_ERR_THREAD;
    }
    // Re-acquire write lock and check condition again
    pthread_rwlock_wrlock(&q->rwlock);
  }

  q->buffer[q->tail] = item;
  q->tail            = (q->tail + 1) % q->capacity;
  q->size++;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting consumer that there's a new item
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->cond_mutex);

  return SC_GENERIC_QUEUE_SUCCESS;
}

// Removes and returns an item from the queue, blocking if empty
// Will timeout after SC_GENERIC_QUEUE_POP_TIMEOUT seconds if the queue remains empty
// @param q Pointer to the queue (must not be NULL)
// @param item Pointer to store the removed item (must not be NULL)
// @return QUEUE_SUCCESS on success, or an error code on failure
sc_generic_queue_ret_val_t sc_generic_queue_pop(sc_generic_queue_t *q, void **item) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL || item == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return SC_GENERIC_QUEUE_ERR_NULL;
  }

  *item = NULL;

  pthread_rwlock_wrlock(&q->rwlock);

  while (q->size == 0) {
    // Queue is empty, must unlock rwlock before waiting on condition
    pthread_rwlock_unlock(&q->rwlock);

    // Use condition mutex for waiting
    pthread_mutex_lock(&q->cond_mutex);

    struct timespec timeout;
    get_absolute_timeout(&timeout, SC_GENERIC_QUEUE_POP_TIMEOUT);

    int result = pthread_cond_timedwait(&q->cond_not_empty, &q->cond_mutex, &timeout);
    pthread_mutex_unlock(&q->cond_mutex);

    if (result == ETIMEDOUT) {
      log_error("sc_generic_queue_pop timed out after %d seconds", SC_GENERIC_QUEUE_POP_TIMEOUT);
      queue_errno = SC_GENERIC_QUEUE_ERR_TIMEOUT;
      return SC_GENERIC_QUEUE_ERR_TIMEOUT;
    }
    if (result != 0) {
      log_error("sc_generic_queue_pop pthread_cond_timedwait failed: %d", result);
      queue_errno = SC_GENERIC_QUEUE_ERR_THREAD;
      return SC_GENERIC_QUEUE_ERR_THREAD;
    }
    // Re-acquire write lock and check condition again
    pthread_rwlock_wrlock(&q->rwlock);
  }

  *item   = q->buffer[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->size--;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting producer that there's new space
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->cond_mutex);

  return SC_GENERIC_QUEUE_SUCCESS;
}

// ============================================================================
// Queue Operations - Non-blocking
// ============================================================================

// Attempts to add an item to the queue without blocking
// @param q Pointer to the queue (must not be NULL)
// @param item Pointer to the item to add (must not be NULL)
// @return QUEUE_SUCCESS on success, QUEUE_ERR_FULL if full, or error code
sc_generic_queue_ret_val_t sc_generic_queue_try_add(sc_generic_queue_t *q, void *item) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL || item == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return SC_GENERIC_QUEUE_ERR_NULL;
  }

  pthread_rwlock_wrlock(&q->rwlock);

  if (q->size == q->capacity) {
    // Queue is full, return error immediately
    pthread_rwlock_unlock(&q->rwlock);
    queue_errno = SC_GENERIC_QUEUE_ERR_FULL;
    return SC_GENERIC_QUEUE_ERR_FULL;
  }

  q->buffer[q->tail] = item;
  q->tail            = (q->tail + 1) % q->capacity;
  q->size++;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting consumer that there's a new item.
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->cond_mutex);

  return SC_GENERIC_QUEUE_SUCCESS;
}

// Attempts to remove and return an item from the queue without blocking
// @param q Pointer to the queue (must not be NULL)
// @param item Pointer to store the removed item (must not be NULL)
// @return QUEUE_SUCCESS on success, QUEUE_ERR_EMPTY if empty, or error code
sc_generic_queue_ret_val_t sc_generic_queue_try_pop(sc_generic_queue_t *q, void **item) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL || item == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return SC_GENERIC_QUEUE_ERR_NULL;
  }

  *item = NULL;

  pthread_rwlock_wrlock(&q->rwlock);

  if (q->size == 0) {
    // Queue is empty, return error immediately
    pthread_rwlock_unlock(&q->rwlock);
    queue_errno = SC_GENERIC_QUEUE_ERR_EMPTY;
    return SC_GENERIC_QUEUE_ERR_EMPTY;
  }

  *item   = q->buffer[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->size--;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting producer that there's new space.
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->cond_mutex);

  return SC_GENERIC_QUEUE_SUCCESS;
}

// ============================================================================
// Queue Status Functions
// ============================================================================

// Checks if the queue is empty (thread-safe)
// @param q Pointer to the queue
// @return true if the queue is empty, false otherwise (including on error)
bool sc_generic_queue_is_empty(const sc_generic_queue_t *q) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return false;
  }

  pthread_rwlock_rdlock_const(&q->rwlock);
  bool is_empty = (q->size == 0);
  pthread_rwlock_unlock_const(&q->rwlock);
  return is_empty;
}

// Checks if the queue is full (thread-safe)
// @param q Pointer to the queue
// @return true if the queue is full, false otherwise (including on error)
bool sc_generic_queue_is_full(const sc_generic_queue_t *q) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return false;
  }

  pthread_rwlock_rdlock_const(&q->rwlock);
  bool is_full = (q->size == q->capacity);
  pthread_rwlock_unlock_const(&q->rwlock);
  return is_full;
}

// Gets the current number of items in the queue (thread-safe)
// @param q Pointer to the queue
// @return Number of items currently in the queue, 0 on error
size_t sc_generic_queue_get_size(const sc_generic_queue_t *q) {
  queue_errno = SC_GENERIC_QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = SC_GENERIC_QUEUE_ERR_NULL;
    return 0;
  }

  pthread_rwlock_rdlock_const(&q->rwlock);
  size_t size = q->size;
  pthread_rwlock_unlock_const(&q->rwlock);
  return size;
}
