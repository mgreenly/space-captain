#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "message.h"

// ============================================================================
// Constants and Error Codes
// ============================================================================

// Queue operation return codes
typedef enum {
  QUEUE_SUCCESS      = 0,  // Operation completed successfully
  QUEUE_ERR_TIMEOUT  = -1, // Operation timed out
  QUEUE_ERR_THREAD   = -2, // Thread operation failed
  QUEUE_ERR_NULL     = -3, // Null pointer parameter
  QUEUE_ERR_MEMORY   = -4, // Memory allocation failure
  QUEUE_ERR_FULL     = -5, // Queue is full (for try_add)
  QUEUE_ERR_EMPTY    = -6, // Queue is empty (for try_pop)
  QUEUE_ERR_INVALID  = -7, // Invalid parameter (e.g., capacity = 0)
  QUEUE_ERR_OVERFLOW = -8  // Integer overflow in capacity calculation
} sc_queue_ret_val_t;

// Timeout values in seconds for blocking operations
#define QUEUE_POP_TIMEOUT 2 // Timeout for pop operations
#define QUEUE_ADD_TIMEOUT 2 // Timeout for add operations

// Maximum safe capacity to prevent excessive allocations
#define QUEUE_MAX_CAPACITY (SIZE_MAX / sizeof(message_t *) / 2)

// ============================================================================
// Type Definitions
// ============================================================================

// Thread-safe message queue structure
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

// Cleanup callback function type for sc_queue_nuke_with_cleanup
typedef void (*queue_cleanup_fn)(message_t *msg, void *user_data);

// ============================================================================
// Queue Lifecycle Functions
// ============================================================================

queue_t *sc_queue_init(size_t capacity);
sc_queue_ret_val_t sc_queue_nuke(queue_t *q);
void sc_queue_nuke_with_cleanup(queue_t *q, queue_cleanup_fn cleanup_fn, void *user_data);

// ============================================================================
// Queue Operations
// ============================================================================

// Blocking operations (with timeout)
sc_queue_ret_val_t sc_queue_add(queue_t *q, message_t *msg);
sc_queue_ret_val_t sc_queue_pop(queue_t *q, message_t **msg);

// Non-blocking operations
sc_queue_ret_val_t sc_queue_try_add(queue_t *q, message_t *msg);
sc_queue_ret_val_t sc_queue_try_pop(queue_t *q, message_t **msg);

// ============================================================================
// Queue Status Functions
// ============================================================================

bool sc_queue_is_empty(queue_t *q);
bool sc_queue_is_full(queue_t *q);
size_t sc_queue_get_size(queue_t *q);

// ============================================================================
// Error Handling Functions
// ============================================================================

sc_queue_ret_val_t sc_queue_get_error(void);
void sc_queue_clear_error(void);
const char *sc_queue_strerror(sc_queue_ret_val_t err);

#endif // QUEUE_H
