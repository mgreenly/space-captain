#include "message_queue.h"

// Creates a new thread-safe message queue with the specified capacity.
// @param capacity Maximum number of messages the queue can hold (must be > 0).
// @return Pointer to the newly created queue, or NULL on failure.
sc_message_queue_t *sc_message_queue_init(size_t capacity) {
  return sc_generic_queue_init(capacity);
}

// Destroys a message queue and frees its resources.
// Note: This does not free the message_t items still in the queue.
// The caller is responsible for managing the lifecycle of messages.
// @param queue Pointer to the queue to destroy.
// @return SC_MESSAGE_QUEUE_SUCCESS on success, or an error code on failure.
sc_generic_queue_ret_val_t sc_message_queue_nuke(sc_message_queue_t *queue) {
  return sc_generic_queue_nuke(queue);
}

// Adds a message to the queue, blocking if the queue is full.
// Will time out if the queue remains full for a configured duration.
// @param queue Pointer to the queue (must not be NULL).
// @param msg Pointer to the message to add (must not be NULL).
// @return SC_MESSAGE_QUEUE_SUCCESS on success, or an error code on failure.
sc_message_queue_ret_val_t sc_message_queue_add(sc_message_queue_t *queue, message_t *msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_add(queue, (void *) msg);
}

// Attempts to add a message to the queue without blocking.
// @param queue Pointer to the queue (must not be NULL).
// @param msg Pointer to the message to add (must not be NULL).
// @return SC_MESSAGE_QUEUE_SUCCESS on success, SC_MESSAGE_QUEUE_ERR_FULL if full,
//         or another error code on failure.
sc_message_queue_ret_val_t sc_message_queue_try_add(sc_message_queue_t *queue, message_t *msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_try_add(queue, (void *) msg);
}

// Removes and returns a message from the queue, blocking if empty.
// Will time out if the queue remains empty for a configured duration.
// @param queue Pointer to the queue (must not be NULL).
// @param msg Pointer to store the removed message (must not be NULL).
// @return SC_MESSAGE_QUEUE_SUCCESS on success, or an error code on failure.
sc_message_queue_ret_val_t sc_message_queue_pop(sc_message_queue_t *queue, message_t **msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_pop(queue, (void **) msg);
}

// Attempts to remove and return a message from the queue without blocking.
// @param queue Pointer to the queue (must not be NULL).
// @param msg Pointer to store the removed message (must not be NULL).
// @return SC_MESSAGE_QUEUE_SUCCESS on success, SC_MESSAGE_QUEUE_ERR_EMPTY if empty,
//         or another error code on failure.
sc_message_queue_ret_val_t sc_message_queue_try_pop(sc_message_queue_t *queue, message_t **msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_try_pop(queue, (void **) msg);
}

// Checks if the queue is empty (thread-safe).
// @param queue Pointer to the queue.
// @return true if the queue is empty, false otherwise.
bool sc_message_queue_is_empty(const sc_message_queue_t *queue) {
  return sc_generic_queue_is_empty(queue);
}

// Checks if the queue is full (thread-safe).
// @param queue Pointer to the queue.
// @return true if the queue is full, false otherwise.
bool sc_message_queue_is_full(const sc_message_queue_t *queue) {
  return sc_generic_queue_is_full(queue);
}

// Gets the current number of items in the queue (thread-safe).
// @param queue Pointer to the queue.
// @return Number of items currently in the queue, or 0 on error.
size_t sc_message_queue_size(const sc_message_queue_t *queue) {
  return sc_generic_queue_get_size(queue);
}
