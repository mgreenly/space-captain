#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "generic_queue.h"
#include "message.h" // Include message.h for the Message type
#include <stdbool.h>
#include <stddef.h>

// Return codes for message queue operations (matches sc_generic_queue_ret_val_t)
typedef enum {
  SC_MESSAGE_QUEUE_ERR_OVERFLOW = -8, // Integer overflow in capacity calculation
  SC_MESSAGE_QUEUE_ERR_INVALID  = -7, // Invalid parameter (e.g., capacity = 0)
  SC_MESSAGE_QUEUE_ERR_EMPTY    = -6, // Queue is empty (for try_pop)
  SC_MESSAGE_QUEUE_ERR_FULL     = -5, // Queue is full (for try_add)
  SC_MESSAGE_QUEUE_ERR_MEMORY   = -4, // Memory allocation failure
  SC_MESSAGE_QUEUE_ERR_NULL     = -3, // Null pointer parameter
  SC_MESSAGE_QUEUE_ERR_THREAD   = -2, // Thread operation failed
  SC_MESSAGE_QUEUE_ERR_TIMEOUT  = -1, // Operation timed out
  SC_MESSAGE_QUEUE_SUCCESS      = 0   // Operation completed successfully
} sc_message_queue_ret_val_t;

// Type alias for message queue (using generic queue internally)
typedef sc_generic_queue_t sc_message_queue_t;

// ============================================================================
// Queue Lifecycle Functions
// ============================================================================

// Initialize a new message queue with the specified capacity
// Returns: Pointer to the initialized queue, or NULL on error
sc_message_queue_t *sc_message_queue_init(size_t capacity);

// Destroy a message queue and free all resources
// Returns: SC_GENERIC_QUEUE_SUCCESS on success, error code otherwise
sc_generic_queue_ret_val_t sc_message_queue_nuke(sc_message_queue_t *queue);

// ============================================================================
// Queue Operations
// ============================================================================

// Add a message to the queue (blocking with timeout)
// Returns: SC_MESSAGE_QUEUE_SUCCESS on success, error code otherwise
sc_message_queue_ret_val_t sc_message_queue_add(sc_message_queue_t *queue, message_t *msg);

// Try to add a message to the queue (non-blocking)
// Returns: SC_MESSAGE_QUEUE_SUCCESS on success, SC_MESSAGE_QUEUE_ERR_FULL if full
sc_message_queue_ret_val_t sc_message_queue_try_add(sc_message_queue_t *queue, message_t *msg);

// Remove a message from the queue (blocking with timeout)
// Returns: SC_MESSAGE_QUEUE_SUCCESS on success, error code otherwise
// The message pointer is set in the msg output parameter
sc_message_queue_ret_val_t sc_message_queue_pop(sc_message_queue_t *queue, message_t **msg);

// Try to remove a message from the queue (non-blocking)
// Returns: SC_MESSAGE_QUEUE_SUCCESS on success, SC_MESSAGE_QUEUE_ERR_EMPTY if empty
// The message pointer is set in the msg output parameter
sc_message_queue_ret_val_t sc_message_queue_try_pop(sc_message_queue_t *queue, message_t **msg);

// ============================================================================
// Queue Status Functions
// ============================================================================

// Check if the queue is empty
// Returns: true if empty, false otherwise
bool sc_message_queue_is_empty(const sc_message_queue_t *queue);

// Check if the queue is full
// Returns: true if full, false otherwise
bool sc_message_queue_is_full(const sc_message_queue_t *queue);

// Get the current number of messages in the queue
// Returns: Number of messages currently in the queue
size_t sc_message_queue_size(const sc_message_queue_t *queue);

#endif // MESSAGE_QUEUE_H
