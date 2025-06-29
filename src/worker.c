#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <assert.h>
#include "worker.h"
#include "log.h"
#include "config.h"

// Create worker pool
worker_pool_t *worker_pool_create(int32_t pool_size, queue_t *msg_queue) {
  assert(pool_size > 0);
  assert(msg_queue != NULL);

  worker_pool_t *pool = malloc(sizeof(worker_pool_t));
  if (!pool) {
    log_error("%s", "Failed to allocate worker pool");
    return NULL;
  }

  pool->workers = calloc(pool_size, sizeof(worker_context_t));
  if (!pool->workers) {
    log_error("%s", "Failed to allocate worker contexts");
    free(pool);
    return NULL;
  }

  pool->pool_size = pool_size;
  pool->msg_queue = msg_queue;
  pool->shutdown_flag = false;

  // Initialize worker contexts
  for (int32_t i = 0; i < pool_size; i++) {
    pool->workers[i].id = i;
    pool->workers[i].msg_queue = msg_queue;
    pool->workers[i].shutdown_flag = &pool->shutdown_flag;
  }

  log_info("Created worker pool with %d workers", pool_size);
  return pool;
}

// Destroy worker pool
void worker_pool_destroy(worker_pool_t *pool) {
  if (!pool)
    return;

  free(pool->workers);
  free(pool);
  log_info("%s", "Worker pool destroyed");
}

// Start all workers
void worker_pool_start(worker_pool_t *pool) {
  assert(pool != NULL);

  for (int32_t i = 0; i < pool->pool_size; i++) {
    if (pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]) != 0) {
      log_error("Failed to create worker thread %d", i);
    } else {
      log_info("Started worker thread %d", i);
    }
  }
}

// Stop all workers
void worker_pool_stop(worker_pool_t *pool) {
  assert(pool != NULL);

  pool->shutdown_flag = true;
  log_info("%s", "Signaling worker pool shutdown");

  // Wait for all workers to finish
  for (int32_t i = 0; i < pool->pool_size; i++) {
    pthread_join(pool->workers[i].thread, NULL);
    log_info("Worker thread %d stopped", i);
  }
}

// Worker thread function
void *worker_thread(void *arg) {
  worker_context_t *ctx = (worker_context_t *) arg;
  log_info("Worker %d started", ctx->id);

  while (!*ctx->shutdown_flag) {
    message_t *msg = NULL;
    int32_t result = queue_try_pop(ctx->msg_queue, &msg);

    if (result == QUEUE_SUCCESS && msg != NULL) {
      log_debug("Worker %d processing message type %d", ctx->id, msg->header.type);

      // Extract client fd from message (stored in first 4 bytes of body)
      int32_t client_fd = *(int32_t *) msg->body;

      // Adjust body pointer to actual message content
      message_t actual_msg = *msg;
      actual_msg.body = msg->body + CLIENT_FD_SIZE;

      switch (msg->header.type) {
      case MSG_ECHO:
        handle_echo_message(client_fd, &actual_msg);
        break;
      case MSG_REVERSE:
        handle_reverse_message(client_fd, &actual_msg);
        break;
      case MSG_TIME:
        handle_time_message(client_fd, &actual_msg);
        break;
      default:
        log_error("Worker %d: Unknown message type %d", ctx->id, msg->header.type);
      }

      // Free the original message
      free(msg->body);
      free(msg);
    } else if (result == QUEUE_ERR_EMPTY) {
      // Queue is empty, sleep briefly to avoid busy waiting
      usleep(WORKER_SLEEP_MS * 1000); // Convert ms to us
    } else if (result != QUEUE_SUCCESS) {
      log_error("Worker %d: Queue pop error %d", ctx->id, result);
    }
  }

  log_info("Worker %d shutting down", ctx->id);
  return NULL;
}

// Send response helper
static void send_response(int32_t client_fd, message_type_t type, const char *response) {
  message_header_t header = {.type = type, .length = strlen(response) + 1};

  // Send header
  if (send(client_fd, &header, sizeof(header), 0) != sizeof(header)) {
    log_error("%s", "Failed to send response header");
    return;
  }

  // Send body
  if (send(client_fd, response, header.length, 0) != (ssize_t) header.length) {
    log_error("%s", "Failed to send response body");
    return;
  }

  const char *type_str = message_type_to_string(type);

  log_info("Sent response [Type: %s] to fd=%d: %s", type_str, client_fd, response);
  log_debug("Response details - Type: %d (%s), Length: %u bytes, Client: fd=%d", type, type_str,
            header.length, client_fd);
}

// Handle echo message - sends back the same message
void handle_echo_message(int32_t client_fd, message_t *msg) {
  log_debug("Worker handling ECHO request from fd=%d: %s", client_fd, msg->body);
  send_response(client_fd, MSG_ECHO, msg->body);
}

// Handle reverse message - sends back the reversed string
void handle_reverse_message(int32_t client_fd, message_t *msg) {
  log_debug("Worker handling REVERSE request from fd=%d: %s", client_fd, msg->body);

  int32_t len = strlen(msg->body);
  char *reversed = malloc(len + 1);
  if (!reversed) {
    log_error("%s", "Failed to allocate reverse buffer");
    return;
  }

  // Reverse the string
  for (int32_t i = 0; i < len; i++) {
    reversed[i] = msg->body[len - 1 - i];
  }
  reversed[len] = '\0';

  log_debug("Reversed string: %s -> %s", msg->body, reversed);
  send_response(client_fd, MSG_REVERSE, reversed);
  free(reversed);
}

// Handle time message - sends back current UTC time in ISO8601 format
void handle_time_message(int32_t client_fd, message_t *msg) {
  (void) msg; // Unused parameter
  log_debug("Worker handling TIME request from fd=%d", client_fd);

  time_t now;
  time(&now);
  struct tm *tm_info = gmtime(&now);

  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_info);

  log_debug("Generated time response: %s", time_str);
  send_response(client_fd, MSG_TIME, time_str);
}