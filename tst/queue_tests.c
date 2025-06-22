#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "vendor/unity.c"

#include "../src/message.h"
#include "../src/queue.h"
#include "../src/queue.c"

void test_queue_add_and_pop_message(void) {
  queue_t *queue = queue_create(10);
  TEST_ASSERT_NOT_NULL(queue);

  char *test_message = "This is the test message";

  message_t msg = {
    .header = {
      .type = MSG_ECHO,
      .length = strlen(test_message)
    },
    .body = test_message
  };

  queue_add(queue, &msg);

  message_t *popped_msg = queue_pop(queue);
  TEST_ASSERT_NOT_NULL(popped_msg);
  TEST_ASSERT_EQUAL(MSG_ECHO, popped_msg->header.type);
  TEST_ASSERT_EQUAL_STRING(test_message, popped_msg->body);

  queue_destroy(queue);
}

typedef struct {
  queue_t *queue;
  int delay_ms;
  int test_value;
} thread_data_t;

void* producer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;
  
  usleep(data->delay_ms * 1000);
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Test message %d", data->test_value);
  
  message_t *msg = malloc(sizeof(message_t));
  msg->header.type = MSG_ECHO;
  msg->header.length = strlen(buffer);
  msg->body = strdup(buffer);
  
  queue_add(data->queue, msg);
  
  return NULL;
}

void* consumer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;
  
  message_t *msg = queue_pop(data->queue);
  
  data->test_value = (msg != NULL) ? 1 : 0;
  
  if (msg) {
    free(msg->body);
    free(msg);
  }
  
  return NULL;
}

void test_queue_pop_blocks_until_push(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  pthread_t producer, consumer;
  thread_data_t producer_data = {queue, 100, 42};
  thread_data_t consumer_data = {queue, 0, 0};
  
  pthread_create(&consumer, NULL, consumer_thread, &consumer_data);
  
  usleep(3000);
  
  pthread_create(&producer, NULL, producer_thread, &producer_data);
  
  pthread_join(consumer, NULL);
  pthread_join(producer, NULL);
  
  TEST_ASSERT_EQUAL(1, consumer_data.test_value);
  
  queue_destroy(queue);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  UnityBegin("tst/queue_tests.c");
  RUN_TEST(test_queue_add_and_pop_message);
  RUN_TEST(test_queue_pop_blocks_until_push);
  return (UnityEnd());
}
