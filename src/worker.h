#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "queue.h"
#include "message.h"

// Worker context structure
typedef struct {
  int32_t id;
  pthread_t thread;
  queue_t *msg_queue;
  volatile bool *shutdown_flag;
} worker_context_t;

// Worker pool structure
typedef struct {
  worker_context_t *workers;
  int32_t pool_size;
  queue_t *msg_queue;
  volatile bool shutdown_flag;
} worker_pool_t;

// Worker pool functions
worker_pool_t *sc_worker_pool_create(int32_t pool_size, queue_t *msg_queue);
void sc_worker_pool_destroy(worker_pool_t *pool);
void sc_worker_pool_start(worker_pool_t *pool);
void sc_worker_pool_stop(worker_pool_t *pool);

// Worker thread function
void *sc_worker_thread(void *arg);

// Message handlers
void sc_worker_handle_echo_message(int32_t client_fd, message_t *msg);
void sc_worker_handle_reverse_message(int32_t client_fd, message_t *msg);
void sc_worker_handle_time_message(int32_t client_fd, message_t *msg);

#endif // WORKER_H