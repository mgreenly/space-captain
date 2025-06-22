#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include "message.h"
#include "queue.h"
#include "log.h"

// Timeout values in seconds for blocking operations
#define QUEUE_POP_TIMEOUT 2   // 2 seconds timeout for pop operations (shorter for testing)
#define QUEUE_ADD_TIMEOUT 2   // 2 seconds timeout for add operations (shorter for testing)

/**
 * Calculates an absolute timeout from a relative timeout in seconds.
 * @param abs_timeout Pointer to timespec structure to store the absolute timeout
 * @param timeout_seconds Number of seconds from now for the timeout
 */
static void get_absolute_timeout(struct timespec* abs_timeout, int timeout_seconds) {
    clock_gettime(CLOCK_REALTIME, abs_timeout);
    abs_timeout->tv_sec += timeout_seconds;
}

/**
 * Creates a new thread-safe message queue with the specified capacity.
 * @param capacity Maximum number of messages the queue can hold (must be > 0)
 * @return Pointer to the newly created queue, or NULL on allocation failure
 */
queue_t* queue_create(size_t capacity) {
    assert(capacity > 0);
    queue_t* q = malloc(sizeof(queue_t));
    if (!q) {
        log_error("%s", "Failed to allocate memory for queue");
        return NULL;
    }

    q->buffer = malloc(sizeof(message_t*) * capacity);
    if (!q->buffer) {
        log_error("%s", "Failed to allocate memory for queue buffer");
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);

    return q;
}

/**
 * Destroys a queue and frees its resources.
 * Note: Does not free messages still in the queue - caller is responsible.
 * @param q Pointer to the queue to destroy (must not be NULL)
 */
void queue_destroy(queue_t* q) {
    assert(q != NULL);
    // Note: This does not free the messages themselves,
    // as ownership is transferred out of the queue on pop.
    // The caller is responsible for freeing messages.
    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_cond_destroy(&q->cond_not_full);
    free(q);
}

/**
 * Destroys a queue and applies a cleanup function to all remaining messages.
 * @param q Pointer to the queue to destroy (must not be NULL)
 * @param cleanup_fn Optional callback to process each remaining message (can be NULL)
 * @param user_data Optional user data passed to the cleanup function
 */
void queue_destroy_with_cleanup(queue_t* q, queue_cleanup_fn cleanup_fn, void* user_data) {
    assert(q != NULL);

    // Lock the queue to prevent any new operations
    pthread_mutex_lock(&q->mutex);

    // Drain all remaining messages
    while (q->size > 0) {
        message_t* msg = q->buffer[q->head];
        q->head = (q->head + 1) % q->capacity;
        q->size--;

        // Apply cleanup callback if provided
        if (cleanup_fn != NULL) {
            cleanup_fn(msg, user_data);
        }
    }

    // Unlock before destroying synchronization primitives
    pthread_mutex_unlock(&q->mutex);

    // Clean up queue structure
    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_cond_destroy(&q->cond_not_full);
    free(q);
}

/**
 * Adds a message to the queue, blocking if the queue is full.
 * Will timeout after QUEUE_ADD_TIMEOUT seconds if the queue remains full.
 * @param q Pointer to the queue (must not be NULL)
 * @param msg Pointer to the message to add
 */
void queue_add(queue_t* q, message_t* msg) {
    assert(q != NULL);
    pthread_mutex_lock(&q->mutex);

    while (q->size == q->capacity) {
        // Queue is full, wait for a spot to open up with timeout
        struct timespec timeout;
        get_absolute_timeout(&timeout, QUEUE_ADD_TIMEOUT);

        int result = pthread_cond_timedwait(&q->cond_not_full, &q->mutex, &timeout);
        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mutex);
            log_error("queue_add timed out after %d seconds", QUEUE_ADD_TIMEOUT);
            return; // Or could return error code if function signature changed
        }
        if (result != 0) {
            pthread_mutex_unlock(&q->mutex);
            log_error("queue_add pthread_cond_timedwait failed: %d", result);
            return;
        }
    }

    q->buffer[q->tail] = msg;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;

    // Signal a waiting consumer that there's a new item.
    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);
}

/**
 * Removes and returns a message from the queue, blocking if empty.
 * Will timeout after QUEUE_POP_TIMEOUT seconds if the queue remains empty.
 * @param q Pointer to the queue (must not be NULL)
 * @return Pointer to the removed message, or NULL on timeout/error
 */
message_t* queue_pop(queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock(&q->mutex);

    while (q->size == 0) {
        // Queue is empty, wait for an item to be pushed with timeout
        struct timespec timeout;
        get_absolute_timeout(&timeout, QUEUE_POP_TIMEOUT);

        int result = pthread_cond_timedwait(&q->cond_not_empty, &q->mutex, &timeout);
        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mutex);
            log_error("queue_pop timed out after %d seconds", QUEUE_POP_TIMEOUT);
            return NULL;
        }
        if (result != 0) {
            pthread_mutex_unlock(&q->mutex);
            log_error("queue_pop pthread_cond_timedwait failed: %d", result);
            return NULL;
        }
    }

    message_t* msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    // Signal a waiting producer that there's new space.
    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->mutex);

    return msg;
}

/**
 * Attempts to add a message to the queue without blocking.
 * @param q Pointer to the queue (must not be NULL)
 * @param msg Pointer to the message to add
 * @return 0 on success, -1 if the queue is full
 */
int queue_try_add(queue_t* q, message_t* msg) {
    assert(q != NULL);
    pthread_mutex_lock(&q->mutex);

    if (q->size == q->capacity) {
        // Queue is full, return error immediately
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    q->buffer[q->tail] = msg;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;

    // Signal a waiting consumer that there's a new item.
    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);

    return 0;
}

/**
 * Attempts to remove and return a message from the queue without blocking.
 * @param q Pointer to the queue (must not be NULL)
 * @return Pointer to the removed message, or NULL if the queue is empty
 */
message_t* queue_try_pop(queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock(&q->mutex);

    if (q->size == 0) {
        // Queue is empty, return NULL immediately
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    message_t* msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    // Signal a waiting producer that there's new space.
    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->mutex);

    return msg;
}

/**
 * Checks if the queue is empty (thread-safe).
 * @param q Pointer to the queue (must not be NULL)
 * @return 1 if the queue is empty, 0 otherwise
 */
int queue_is_empty(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int is_empty = (q->size == 0);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return is_empty;
}

/**
 * Checks if the queue is full (thread-safe).
 * @param q Pointer to the queue (must not be NULL)
 * @return 1 if the queue is full, 0 otherwise
 */
int queue_is_full(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int is_full = (q->size == q->capacity);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return is_full;
}

/**
 * Gets the current number of messages in the queue (thread-safe).
 * @param q Pointer to the queue (must not be NULL)
 * @return Number of messages currently in the queue
 */
size_t queue_get_size(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    size_t size = q->size;
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return size;
}
