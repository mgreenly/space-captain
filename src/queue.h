#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stddef.h>

#include "message.h"

typedef struct {
    message_t** buffer;
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
} queue_t;

queue_t* queue_create(size_t capacity);
void queue_destroy(queue_t* q);
void queue_push(queue_t* q, message_t* msg);
message_t* queue_pop(queue_t* q);

#endif // QUEUE_H
