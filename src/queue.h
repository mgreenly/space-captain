#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>

#include "message.h"

// Queue operation return codes
#define QUEUE_SUCCESS      0
#define QUEUE_ERR_TIMEOUT  -1
#define QUEUE_ERR_THREAD   -2
#define QUEUE_ERR_NULL     -3
#define QUEUE_ERR_MEMORY   -4    // Memory allocation failure
#define QUEUE_ERR_FULL     -5    // Queue is full (for try_add)
#define QUEUE_ERR_EMPTY    -6    // Queue is empty (for try_pop)
#define QUEUE_ERR_INVALID  -7    // Invalid parameter (e.g., capacity = 0)
#define QUEUE_ERR_OVERFLOW -8    // Integer overflow in capacity calculation

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

typedef void (*queue_cleanup_fn)(message_t* msg, void* user_data);

queue_t* queue_create(size_t capacity);
int queue_destroy(queue_t* q);
void queue_destroy_with_cleanup(queue_t* q, queue_cleanup_fn cleanup_fn, void* user_data);
int queue_add(queue_t* q, message_t* msg);
int queue_pop(queue_t* q, message_t** msg);
int queue_try_add(queue_t* q, message_t* msg);
int queue_try_pop(queue_t* q, message_t** msg);
bool queue_is_empty(const queue_t* q);
bool queue_is_full(const queue_t* q);
size_t queue_get_size(const queue_t* q);

// Error handling functions
int queue_get_error(void);
void queue_clear_error(void);
const char* queue_strerror(int err);

#endif // QUEUE_H
