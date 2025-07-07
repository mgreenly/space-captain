#include "message_queue.h"

// Initialize a new message queue with the specified capacity
sc_message_queue_t *sc_message_queue_init(size_t capacity) {
  return sc_generic_queue_init(capacity);
}

// Destroy a message queue and free all resources
sc_generic_queue_ret_val_t sc_message_queue_nuke(sc_message_queue_t *queue) {
  return sc_generic_queue_nuke(queue);
}

// Add a message to the queue (blocking)
sc_message_queue_ret_val_t sc_message_queue_add(sc_message_queue_t *queue, message_t *msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_add(queue, (void *) msg);
}

// Try to add a message to the queue (non-blocking)
sc_message_queue_ret_val_t sc_message_queue_try_add(sc_message_queue_t *queue, message_t *msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_try_add(queue, (void *) msg);
}

// Remove a message from the queue (blocking)
sc_message_queue_ret_val_t sc_message_queue_pop(sc_message_queue_t *queue, message_t **msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_pop(queue, (void **) msg);
}

// Try to remove a message from the queue (non-blocking)
sc_message_queue_ret_val_t sc_message_queue_try_pop(sc_message_queue_t *queue, message_t **msg) {
  return (sc_message_queue_ret_val_t) sc_generic_queue_try_pop(queue, (void **) msg);
}

// Check if the queue is empty
bool sc_message_queue_is_empty(sc_message_queue_t *queue) {
  return sc_generic_queue_is_empty(queue);
}

// Check if the queue is full
bool sc_message_queue_is_full(sc_message_queue_t *queue) {
  return sc_generic_queue_is_full(queue);
}

// Get the current size of the queue
size_t sc_message_queue_size(sc_message_queue_t *queue) {
  return sc_generic_queue_get_size(queue);
}