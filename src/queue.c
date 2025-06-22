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

// Helper function to calculate absolute timeout from relative timeout
static void get_absolute_timeout(struct timespec* abs_timeout, int timeout_seconds) {
    clock_gettime(CLOCK_REALTIME, abs_timeout);
    abs_timeout->tv_sec += timeout_seconds;
}

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

int queue_is_empty(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int is_empty = (q->size == 0);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return is_empty;
}

int queue_is_full(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int is_full = (q->size == q->capacity);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return is_full;
}

size_t queue_get_size(const queue_t* q) {
    assert(q != NULL);
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    size_t size = q->size;
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    return size;
}