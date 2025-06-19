#include <stdlib.h>
#include <assert.h>

#include "message.h"
#include "queue.h"
#include "log.h"

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

void queue_push(queue_t* q, message_t* msg) {
    assert(q != NULL);
    pthread_mutex_lock(&q->mutex);

    while (q->size == q->capacity) {
        // Queue is full, wait for a spot to open up.
        pthread_cond_wait(&q->cond_not_full, &q->mutex);
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
        // Queue is empty, wait for an item to be pushed.
        pthread_cond_wait(&q->cond_not_empty, &q->mutex);
    }

    message_t* msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    // Signal a waiting producer that there's new space.
    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->mutex);

    return msg;
}
