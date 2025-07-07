#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "generic_queue.h"
#include "message.h" // Include message.h for the Message type

// Type-safe wrapper functions for message queue operations

static inline void sc_message_queue_push(sc_generic_queue_t *q, message_t *msg) {
  sc_generic_queue_add(q, (void *) msg);
}

static inline message_t *sc_message_queue_pop(sc_generic_queue_t *q) {
  void *item = NULL;
  sc_generic_queue_pop(q, &item);
  return (message_t *) item;
}

static inline sc_generic_queue_ret_val_t sc_message_queue_try_push(sc_generic_queue_t *q,
                                                                   message_t *msg) {
  return sc_generic_queue_try_add(q, (void *) msg);
}

static inline sc_generic_queue_ret_val_t sc_message_queue_try_pop(sc_generic_queue_t *q,
                                                                  message_t **msg) {
  return sc_generic_queue_try_pop(q, (void **) msg);
}

#endif // MESSAGE_QUEUE_H