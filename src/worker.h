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
} sc_worker_context_t;

// Worker pool structure
typedef struct {
  sc_worker_context_t *workers;
  int32_t pool_size;
  queue_t *msg_queue;
  volatile bool shutdown_flag;
} sc_worker_pool_t;

// Worker pool functions
sc_worker_pool_t *sc_worker_pool_init(int32_t pool_size, queue_t *msg_queue);
void sc_worker_pool_nuke(sc_worker_pool_t *pool);
void sc_worker_pool_start(sc_worker_pool_t *pool);
void sc_worker_pool_stop(sc_worker_pool_t *pool);

// Worker thread function
void *sc_worker_thread(void *arg);

// Message handlers
void sc_worker_handle_echo_message(int32_t client_fd, message_t *msg);
void sc_worker_handle_reverse_message(int32_t client_fd, message_t *msg);
void sc_worker_handle_time_message(int32_t client_fd, message_t *msg);

#endif // WORKER_H