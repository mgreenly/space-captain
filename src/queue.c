#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "message.h"
#include "queue.h"
#include "log.h"

// Thread-local error variable for queue operations
static __thread int queue_errno = QUEUE_SUCCESS;

// Function to get last queue error
int queue_get_error(void) {
  return queue_errno;
}

// Function to clear queue error
void queue_clear_error(void) {
  queue_errno = QUEUE_SUCCESS;
}

// Function to get error string
const char *queue_strerror(int err) {
  switch (err) {
  case QUEUE_SUCCESS:
    return "Success";
  case QUEUE_ERR_TIMEOUT:
    return "Operation timed out";
  case QUEUE_ERR_THREAD:
    return "Thread operation failed";
  case QUEUE_ERR_NULL:
    return "Null pointer parameter";
  case QUEUE_ERR_MEMORY:
    return "Memory allocation failed";
  case QUEUE_ERR_FULL:
    return "Queue is full";
  case QUEUE_ERR_EMPTY:
    return "Queue is empty";
  case QUEUE_ERR_INVALID:
    return "Invalid parameter";
  case QUEUE_ERR_OVERFLOW:
    return "Integer overflow in capacity calculation";
  default:
    return "Unknown error";
  }
}

// Calculates an absolute timeout from a relative timeout in seconds.
// @param abs_timeout Pointer to timespec structure to store the absolute timeout
// @param timeout_seconds Number of seconds from now for the timeout
static void get_absolute_timeout(struct timespec *abs_timeout, int timeout_seconds) {
  clock_gettime(CLOCK_REALTIME, abs_timeout);
  abs_timeout->tv_sec += timeout_seconds;
}

// Creates a new thread-safe message queue with the specified capacity.
// @param capacity Maximum number of messages the queue can hold (must be > 0)
// @return Pointer to the newly created queue, or NULL on allocation failure
queue_t *queue_create(size_t capacity) {
  queue_errno = QUEUE_SUCCESS;

  if (capacity == 0 || capacity > QUEUE_MAX_CAPACITY) {
    queue_errno = (capacity == 0) ? QUEUE_ERR_INVALID : QUEUE_ERR_OVERFLOW;
    log_error("Invalid capacity: %zu (max: %zu)", capacity, QUEUE_MAX_CAPACITY);
    return NULL;
  }

  queue_t *q = calloc(1, sizeof(queue_t));
  if (!q) {
    queue_errno = QUEUE_ERR_MEMORY;
    log_error("%s", "Failed to allocate memory for queue");
    return NULL;
  }
  // Check for overflow before allocating buffer
  size_t buffer_size;
  if (__builtin_mul_overflow(capacity, sizeof(message_t *), &buffer_size)) {
    queue_errno = QUEUE_ERR_OVERFLOW;
    log_error("Integer overflow calculating buffer size for capacity %zu", capacity);
    free(q);
    return NULL;
  }

  q->buffer = calloc(capacity, sizeof(message_t *));
  if (!q->buffer) {
    queue_errno = QUEUE_ERR_MEMORY;
    log_error("%s", "Failed to allocate memory for queue buffer");
    free(q);
    return NULL;
  }

  q->capacity = capacity;
  q->size = 0;
  q->head = 0;
  q->tail = 0;

  // Initialize rwlock with error checking
  if (pthread_rwlock_init(&q->rwlock, NULL) != 0) {
    queue_errno = QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize queue rwlock");
    free(q->buffer);
    free(q);
    return NULL;
  }
  // Initialize condition mutex with error checking
  if (pthread_mutex_init(&q->cond_mutex, NULL) != 0) {
    queue_errno = QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize condition mutex");
    pthread_rwlock_destroy(&q->rwlock);
    free(q->buffer);
    free(q);
    return NULL;
  }
  // Initialize condition variables with error checking
  if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
    queue_errno = QUEUE_ERR_THREAD;
    log_error("%s", "Failed to initialize cond_not_empty");
    pthread_mutex_destroy(&q->cond_mutex);
    pthread_rwlock_destroy(&q->rwlock);
    free(q->buffer);
    free(q);
    return NULL;
  }

  if (pthread_cond_init(&q->cond_not_full, NULL) != 0) {
    queue_errno = QUEUE_ERR_THREAD;
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

// Destroys a queue and frees its resources.
// Note: Does not free messages still in the queue - caller is responsible.
// @param q Pointer to the queue to destroy
// @return QUEUE_SUCCESS on success, or an error code on failure
int queue_destroy(queue_t *q) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return QUEUE_ERR_NULL;
  }
  // Note: This does not free the messages themselves,
  // as ownership is transferred out of the queue on pop.
  // The caller is responsible for freeing messages.
  int err = pthread_rwlock_destroy(&q->rwlock);
  if (err != 0) {
    queue_errno = QUEUE_ERR_THREAD;
    log_error("Failed to destroy rwlock: %d", err);
  }

  pthread_mutex_destroy(&q->cond_mutex);

  free(q->buffer);
  pthread_cond_destroy(&q->cond_not_empty);
  pthread_cond_destroy(&q->cond_not_full);
  free(q);

  return queue_errno;
}

// Destroys a queue and applies a cleanup function to all remaining messages.
// @param q Pointer to the queue to destroy (must not be NULL)
// @param cleanup_fn Optional callback to process each remaining message (can be NULL)
// @param user_data Optional user data passed to the cleanup function
void queue_destroy_with_cleanup(queue_t *q, queue_cleanup_fn cleanup_fn, void *user_data) {
  if (q == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return;
  }
  // Lock the queue to prevent any new operations
  pthread_rwlock_wrlock(&q->rwlock);

  // Drain all remaining messages
  while (q->size > 0) {
    message_t *msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    // Apply cleanup callback if provided
    if (cleanup_fn != NULL) {
      cleanup_fn(msg, user_data);
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

// Adds a message to the queue, blocking if the queue is full.
// Will timeout after QUEUE_ADD_TIMEOUT seconds if the queue remains full.
// @param q Pointer to the queue (must not be NULL)
// @param msg Pointer to the message to add (must not be NULL)
// @return QUEUE_SUCCESS on success, or an error code on failure
int queue_add(queue_t *q, message_t *msg) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL || msg == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return QUEUE_ERR_NULL;
  }

  pthread_rwlock_wrlock(&q->rwlock);

  while (q->size == q->capacity) {
    // Queue is full, must unlock rwlock before waiting on condition
    pthread_rwlock_unlock(&q->rwlock);

    // Use condition mutex for waiting
    pthread_mutex_lock(&q->cond_mutex);

    struct timespec timeout;
    get_absolute_timeout(&timeout, QUEUE_ADD_TIMEOUT);

    int result = pthread_cond_timedwait(&q->cond_not_full, &q->cond_mutex, &timeout);
    pthread_mutex_unlock(&q->cond_mutex);

    if (result == ETIMEDOUT) {
      log_error("queue_add timed out after %d seconds", QUEUE_ADD_TIMEOUT);
      queue_errno = QUEUE_ERR_TIMEOUT;
      return QUEUE_ERR_TIMEOUT;
    }
    if (result != 0) {
      log_error("queue_add pthread_cond_timedwait failed: %d", result);
      queue_errno = QUEUE_ERR_THREAD;
      return QUEUE_ERR_THREAD;
    }
    // Re-acquire write lock and check condition again
    pthread_rwlock_wrlock(&q->rwlock);
  }

  q->buffer[q->tail] = msg;
  q->tail = (q->tail + 1) % q->capacity;
  q->size++;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting consumer that there's a new item
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->cond_mutex);

  return QUEUE_SUCCESS;
}

// Removes and returns a message from the queue, blocking if empty.
// Will timeout after QUEUE_POP_TIMEOUT seconds if the queue remains empty.
// @param q Pointer to the queue
// @param msg Pointer to store the removed message
// @return QUEUE_SUCCESS on success, or an error code on failure
int queue_pop(queue_t *q, message_t **msg) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL || msg == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return QUEUE_ERR_NULL;
  }

  *msg = NULL;

  pthread_rwlock_wrlock(&q->rwlock);

  while (q->size == 0) {
    // Queue is empty, must unlock rwlock before waiting on condition
    pthread_rwlock_unlock(&q->rwlock);

    // Use condition mutex for waiting
    pthread_mutex_lock(&q->cond_mutex);

    struct timespec timeout;
    get_absolute_timeout(&timeout, QUEUE_POP_TIMEOUT);

    int result = pthread_cond_timedwait(&q->cond_not_empty, &q->cond_mutex, &timeout);
    pthread_mutex_unlock(&q->cond_mutex);

    if (result == ETIMEDOUT) {
      log_error("queue_pop timed out after %d seconds", QUEUE_POP_TIMEOUT);
      queue_errno = QUEUE_ERR_TIMEOUT;
      return QUEUE_ERR_TIMEOUT;
    }
    if (result != 0) {
      log_error("queue_pop pthread_cond_timedwait failed: %d", result);
      queue_errno = QUEUE_ERR_THREAD;
      return QUEUE_ERR_THREAD;
    }
    // Re-acquire write lock and check condition again
    pthread_rwlock_wrlock(&q->rwlock);
  }

  *msg = q->buffer[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->size--;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting producer that there's new space
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->cond_mutex);

  return QUEUE_SUCCESS;
}

// Attempts to add a message to the queue without blocking.
// @param q Pointer to the queue
// @param msg Pointer to the message to add
// @return QUEUE_SUCCESS on success, or an error code on failure
int queue_try_add(queue_t *q, message_t *msg) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL || msg == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return QUEUE_ERR_NULL;
  }

  pthread_rwlock_wrlock(&q->rwlock);

  if (q->size == q->capacity) {
    // Queue is full, return error immediately
    pthread_rwlock_unlock(&q->rwlock);
    queue_errno = QUEUE_ERR_FULL;
    return QUEUE_ERR_FULL;
  }

  q->buffer[q->tail] = msg;
  q->tail = (q->tail + 1) % q->capacity;
  q->size++;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting consumer that there's a new item.
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->cond_mutex);

  return QUEUE_SUCCESS;
}

// Attempts to remove and return a message from the queue without blocking.
// @param q Pointer to the queue
// @param msg Pointer to store the removed message
// @return QUEUE_SUCCESS on success, or an error code on failure
int queue_try_pop(queue_t *q, message_t **msg) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL || msg == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return QUEUE_ERR_NULL;
  }

  *msg = NULL;

  pthread_rwlock_wrlock(&q->rwlock);

  if (q->size == 0) {
    // Queue is empty, return error immediately
    pthread_rwlock_unlock(&q->rwlock);
    queue_errno = QUEUE_ERR_EMPTY;
    return QUEUE_ERR_EMPTY;
  }

  *msg = q->buffer[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->size--;

  pthread_rwlock_unlock(&q->rwlock);

  // Signal a waiting producer that there's new space.
  pthread_mutex_lock(&q->cond_mutex);
  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->cond_mutex);

  return QUEUE_SUCCESS;
}

// Checks if the queue is empty (thread-safe).
// @param q Pointer to the queue
// @return true if the queue is empty, false otherwise (including on error)
bool queue_is_empty(queue_t *q) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return false;
  }

  pthread_rwlock_rdlock(&q->rwlock);
  bool is_empty = (q->size == 0);
  pthread_rwlock_unlock(&q->rwlock);
  return is_empty;
}

// Checks if the queue is full (thread-safe).
// @param q Pointer to the queue
// @return true if the queue is full, false otherwise (including on error)
bool queue_is_full(queue_t *q) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return false;
  }

  pthread_rwlock_rdlock(&q->rwlock);
  bool is_full = (q->size == q->capacity);
  pthread_rwlock_unlock(&q->rwlock);
  return is_full;
}

// Gets the current number of messages in the queue (thread-safe).
// @param q Pointer to the queue
// @return Number of messages currently in the queue, 0 on error
size_t queue_get_size(queue_t *q) {
  queue_errno = QUEUE_SUCCESS;

  if (q == NULL) {
    queue_errno = QUEUE_ERR_NULL;
    return 0;
  }

  pthread_rwlock_rdlock(&q->rwlock);
  size_t size = q->size;
  pthread_rwlock_unlock(&q->rwlock);
  return size;
}
