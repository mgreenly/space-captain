#ifndef GENERIC_QUEUE_H
#define GENERIC_QUEUE_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// No specific type dependencies - this is a generic queue

// ============================================================================
// Constants and Error Codes
// ============================================================================

// Queue operation return codes
typedef enum {
  SC_GENERIC_QUEUE_ERR_OVERFLOW = -8, // Integer overflow in capacity calculation
  SC_GENERIC_QUEUE_ERR_INVALID  = -7, // Invalid parameter (e.g., capacity = 0)
  SC_GENERIC_QUEUE_ERR_EMPTY    = -6, // Queue is empty (for try_pop)
  SC_GENERIC_QUEUE_ERR_FULL     = -5, // Queue is full (for try_add)
  SC_GENERIC_QUEUE_ERR_MEMORY   = -4, // Memory allocation failure
  SC_GENERIC_QUEUE_ERR_NULL     = -3, // Null pointer parameter
  SC_GENERIC_QUEUE_ERR_THREAD   = -2, // Thread operation failed
  SC_GENERIC_QUEUE_ERR_TIMEOUT  = -1, // Operation timed out
  SC_GENERIC_QUEUE_SUCCESS      = 0   // Operation completed successfully
} sc_generic_queue_ret_val_t;

// Timeout values in seconds for blocking operations
#define SC_GENERIC_QUEUE_POP_TIMEOUT 2 // Timeout for pop operations
#define SC_GENERIC_QUEUE_ADD_TIMEOUT 2 // Timeout for add operations

// Maximum safe capacity to prevent excessive allocations
#define SC_GENERIC_QUEUE_MAX_CAPACITY (SIZE_MAX / sizeof(void *) / 2)

// ============================================================================
// Type Definitions
// ============================================================================

// Thread-safe generic queue structure
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

// Cleanup callback function type for sc_generic_queue_nuke_with_cleanup
typedef void (*sc_generic_queue_cleanup_fn)(void *item, void *user_data);

// ============================================================================
// Queue Lifecycle Functions
// ============================================================================

sc_generic_queue_t *sc_generic_queue_init(size_t capacity);
sc_generic_queue_ret_val_t sc_generic_queue_nuke(sc_generic_queue_t *q);
void sc_generic_queue_nuke_with_cleanup(sc_generic_queue_t *q,
                                        sc_generic_queue_cleanup_fn cleanup_fn, void *user_data);

// ============================================================================
// Queue Operations
// ============================================================================

// Blocking operations (with timeout)
sc_generic_queue_ret_val_t sc_generic_queue_add(sc_generic_queue_t *q, void *item);
sc_generic_queue_ret_val_t sc_generic_queue_pop(sc_generic_queue_t *q, void **item);

// Non-blocking operations
sc_generic_queue_ret_val_t sc_generic_queue_try_add(sc_generic_queue_t *q, void *item);
sc_generic_queue_ret_val_t sc_generic_queue_try_pop(sc_generic_queue_t *q, void **item);

// ============================================================================
// Queue Status Functions
// ============================================================================

bool sc_generic_queue_is_empty(const sc_generic_queue_t *q);
bool sc_generic_queue_is_full(const sc_generic_queue_t *q);
size_t sc_generic_queue_get_size(const sc_generic_queue_t *q);

// ============================================================================
// Error Handling Functions
// ============================================================================

sc_generic_queue_ret_val_t sc_generic_queue_get_error(void);
void sc_generic_queue_clear_error(void);
const char *sc_generic_queue_strerror(sc_generic_queue_ret_val_t err);

#endif // GENERIC_QUEUE_H
