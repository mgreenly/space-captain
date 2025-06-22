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

void* blocking_producer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;

  // Create and add a message
  message_t *msg = malloc(sizeof(message_t));
  msg->header.type = MSG_ECHO;
  msg->header.length = 10;
  msg->body = strdup("Blocked msg");

  // This should block since queue is full
  queue_add(data->queue, msg);

  // Set test_value to indicate we successfully added after blocking
  data->test_value = 1;

  return NULL;
}

void* delayed_consumer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;

  // Wait before consuming to ensure producer blocks
  usleep(data->delay_ms * 1000);

  // Pop one message to make space
  message_t *msg = queue_pop(data->queue);
  if (msg) {
    free(msg->body);
    free(msg);
  }

  return NULL;
}

void test_queue_add_blocks_on_full_queue(void) {
  queue_t *queue = queue_create(2);  // Small capacity
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue to capacity with dynamically allocated messages
  message_t *msg1 = malloc(sizeof(message_t));
  msg1->header.type = MSG_ECHO;
  msg1->header.length = 5;
  msg1->body = strdup("msg1");

  message_t *msg2 = malloc(sizeof(message_t));
  msg2->header.type = MSG_ECHO;
  msg2->header.length = 5;
  msg2->body = strdup("msg2");

  queue_add(queue, msg1);
  queue_add(queue, msg2);

  // Queue is now full

  pthread_t producer, consumer;
  thread_data_t producer_data = {queue, 0, 0};  // test_value will be set to 1 when add completes
  thread_data_t consumer_data = {queue, 100, 0};  // wait 100ms before consuming

  // Start producer that will block trying to add to full queue
  pthread_create(&producer, NULL, blocking_producer_thread, &producer_data);

  // Give producer time to reach the blocking point
  usleep(50000);  // 50ms

  // Verify producer hasn't completed yet (still blocked)
  TEST_ASSERT_EQUAL(0, producer_data.test_value);

  // Start consumer that will free up space after delay
  pthread_create(&consumer, NULL, delayed_consumer_thread, &consumer_data);

  // Wait for both threads to complete
  pthread_join(producer, NULL);
  pthread_join(consumer, NULL);

  // Verify producer was able to add message after consumer freed space
  TEST_ASSERT_EQUAL(1, producer_data.test_value);

  // Clean up remaining messages
  // The delayed_consumer_thread already freed one message (either msg1 or msg2)
  // The blocking_producer_thread added one message
  // So there should be exactly 2 messages left in the queue
  for (int i = 0; i < 2; i++) {
    message_t *remaining = queue_pop(queue);
    if (remaining) {
      free(remaining->body);
      free(remaining);
    }
  }

  queue_destroy(queue);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  UnityBegin("tst/queue_tests.c");
  RUN_TEST(test_queue_add_and_pop_message);
  RUN_TEST(test_queue_pop_blocks_until_push);
  RUN_TEST(test_queue_add_blocks_on_full_queue);
  return (UnityEnd());
}
