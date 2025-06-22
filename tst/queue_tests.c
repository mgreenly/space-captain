#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "vendor/unity.c"

#include "../src/message.h"
#include "../src/queue.h"
#include "../src/queue.c"

// Define all test strings at the top
#define TEST_MSG_SIMPLE "This is the test message"
#define TEST_MSG_FORMAT "Test message %d"
#define TEST_MSG_BLOCKED "Blocked msg"
#define TEST_MSG_FILL_1 "msg1"
#define TEST_MSG_FILL_2 "msg2"

// Helper function to create a dynamically allocated message
static message_t* create_test_message(message_type_t type, const char* body) {
  message_t *msg = malloc(sizeof(message_t));
  msg->header.type = type;
  msg->header.length = strlen(body);
  msg->body = strdup(body);
  return msg;
}

// Cleanup callback tracker for testing
typedef struct {
    int cleanup_call_count;
    message_t** cleaned_messages;
    size_t messages_capacity;
} cleanup_tracker_t;

// Test cleanup callback that tracks calls
static void test_cleanup_callback(message_t* msg, void* user_data) {
    cleanup_tracker_t* tracker = (cleanup_tracker_t*)user_data;
    
    // Store the message pointer for verification
    if ((size_t)tracker->cleanup_call_count < tracker->messages_capacity) {
        tracker->cleaned_messages[tracker->cleanup_call_count] = msg;
    }
    
    tracker->cleanup_call_count++;
    
    // Free the message as normal cleanup would do
    free(msg->body);
    free(msg);
}


void test_queue_add_and_pop_message(void) {
  queue_t *queue = queue_create(10);
  TEST_ASSERT_NOT_NULL(queue);

  // Always use dynamic allocation for messages
  message_t *msg = create_test_message(MSG_ECHO, TEST_MSG_SIMPLE);

  queue_add(queue, msg);

  message_t *popped_msg = queue_pop(queue);
  TEST_ASSERT_NOT_NULL(popped_msg);
  TEST_ASSERT_EQUAL(MSG_ECHO, popped_msg->header.type);
  TEST_ASSERT_EQUAL_STRING(TEST_MSG_SIMPLE, popped_msg->body);

  // Clean up dynamically allocated message
  free(popped_msg->body);
  free(popped_msg);

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
  snprintf(buffer, sizeof(buffer), TEST_MSG_FORMAT, data->test_value);

  message_t *msg = create_test_message(MSG_ECHO, buffer);

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
  message_t *msg = create_test_message(MSG_ECHO, TEST_MSG_BLOCKED);

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
  message_t *msg1 = create_test_message(MSG_ECHO, TEST_MSG_FILL_1);
  message_t *msg2 = create_test_message(MSG_ECHO, TEST_MSG_FILL_2);

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

void test_queue_try_add_returns_error_on_full(void) {
  queue_t *queue = queue_create(2);  // Small capacity
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue to capacity
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  
  queue_add(queue, msg1);
  queue_add(queue, msg2);
  
  // Queue is now full, try_add should return -1
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  int result = queue_try_add(queue, msg3);
  TEST_ASSERT_EQUAL(-1, result);
  
  // Clean up msg3 since it wasn't added
  free(msg3->body);
  free(msg3);
  
  // Clean up queue messages
  for (int i = 0; i < 2; i++) {
    message_t *msg = queue_pop(queue);
    free(msg->body);
    free(msg);
  }
  
  queue_destroy(queue);
}

void test_queue_try_add_succeeds_with_space(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  message_t *msg = create_test_message(MSG_ECHO, "test message");
  
  // Try to add to empty queue, should succeed
  int result = queue_try_add(queue, msg);
  TEST_ASSERT_EQUAL(0, result);
  
  // Verify message was added
  message_t *popped = queue_pop(queue);
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_EQUAL_STRING("test message", popped->body);
  
  free(popped->body);
  free(popped);
  
  queue_destroy(queue);
}

void test_queue_try_pop_returns_null_on_empty(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Try to pop from empty queue
  message_t *msg = queue_try_pop(queue);
  TEST_ASSERT_NULL(msg);
  
  queue_destroy(queue);
}

void test_queue_try_pop_returns_message(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  message_t *msg = create_test_message(MSG_ECHO, "pop test");
  queue_add(queue, msg);
  
  // Try to pop, should get the message
  message_t *popped = queue_try_pop(queue);
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_EQUAL_STRING("pop test", popped->body);
  
  free(popped->body);
  free(popped);
  
  queue_destroy(queue);
}

void test_queue_try_operations_mixed(void) {
  queue_t *queue = queue_create(3);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Add messages using both methods
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg1));
  queue_add(queue, msg2);
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg3));
  
  // Queue is full now
  message_t *msg4 = create_test_message(MSG_ECHO, "msg4");
  TEST_ASSERT_EQUAL(-1, queue_try_add(queue, msg4));
  free(msg4->body);
  free(msg4);
  
  // Pop using both methods
  message_t *p1 = queue_try_pop(queue);
  TEST_ASSERT_NOT_NULL(p1);
  TEST_ASSERT_EQUAL_STRING("msg1", p1->body);
  free(p1->body);
  free(p1);
  
  message_t *p2 = queue_pop(queue);
  TEST_ASSERT_NOT_NULL(p2);
  TEST_ASSERT_EQUAL_STRING("msg2", p2->body);
  free(p2->body);
  free(p2);
  
  message_t *p3 = queue_try_pop(queue);
  TEST_ASSERT_NOT_NULL(p3);
  TEST_ASSERT_EQUAL_STRING("msg3", p3->body);
  free(p3->body);
  free(p3);
  
  // Queue should be empty now
  TEST_ASSERT_NULL(queue_try_pop(queue));
  
  queue_destroy(queue);
}

void test_queue_is_empty_on_new_queue(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  // New queue should be empty
  TEST_ASSERT_EQUAL(1, queue_is_empty(queue));
  
  queue_destroy(queue);
}

void test_queue_is_empty_after_add_and_pop(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Initially empty
  TEST_ASSERT_EQUAL(1, queue_is_empty(queue));
  
  // Add a message, should not be empty
  message_t *msg = create_test_message(MSG_ECHO, "test");
  queue_add(queue, msg);
  TEST_ASSERT_EQUAL(0, queue_is_empty(queue));
  
  // Pop the message, should be empty again
  message_t *popped = queue_pop(queue);
  TEST_ASSERT_EQUAL(1, queue_is_empty(queue));
  
  // Clean up
  free(popped->body);
  free(popped);
  queue_destroy(queue);
}

void test_queue_is_empty_thread_safety(void) {
  queue_t *queue = queue_create(10);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Test that queue_is_empty works correctly with concurrent operations
  pthread_t producer, consumer;
  thread_data_t producer_data = {queue, 50, 42};
  thread_data_t consumer_data = {queue, 0, 0};
  
  // Start consumer first (will block on empty queue)
  pthread_create(&consumer, NULL, consumer_thread, &consumer_data);
  
  // Queue should still be empty initially
  TEST_ASSERT_EQUAL(1, queue_is_empty(queue));
  
  // Start producer after small delay
  pthread_create(&producer, NULL, producer_thread, &producer_data);
  
  pthread_join(consumer, NULL);
  pthread_join(producer, NULL);
  
  // Queue should be empty after consumer got the message
  TEST_ASSERT_EQUAL(1, queue_is_empty(queue));
  
  queue_destroy(queue);
}

void test_queue_is_full_on_new_queue(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  // New queue should not be full
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  queue_destroy(queue);
}

void test_queue_is_full_after_filling_queue(void) {
  queue_t *queue = queue_create(2);  // Small capacity for easy testing
  TEST_ASSERT_NOT_NULL(queue);
  
  // Initially not full
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  // Add first message, should not be full yet
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  queue_add(queue, msg1);
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  // Add second message, should now be full
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  queue_add(queue, msg2);
  TEST_ASSERT_EQUAL(1, queue_is_full(queue));
  
  // Pop one message, should not be full anymore
  message_t *popped = queue_pop(queue);
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  // Clean up
  free(popped->body);
  free(popped);
  
  // Clean up remaining message
  message_t *remaining = queue_pop(queue);
  free(remaining->body);
  free(remaining);
  
  queue_destroy(queue);
}

void test_queue_is_full_with_try_operations(void) {
  queue_t *queue = queue_create(3);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Fill queue using try_add operations
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg2));
  TEST_ASSERT_EQUAL(0, queue_is_full(queue));
  
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg3));
  TEST_ASSERT_EQUAL(1, queue_is_full(queue));
  
  // Try to add another message, should fail since queue is full
  message_t *msg4 = create_test_message(MSG_ECHO, "msg4");
  TEST_ASSERT_EQUAL(-1, queue_try_add(queue, msg4));
  TEST_ASSERT_EQUAL(1, queue_is_full(queue));
  
  // Clean up unused message
  free(msg4->body);
  free(msg4);
  
  // Clean up queue
  for (int i = 0; i < 3; i++) {
    message_t *msg = queue_try_pop(queue);
    if (msg) {
      free(msg->body);
      free(msg);
    }
  }
  
  queue_destroy(queue);
}

void test_queue_get_size_on_new_queue(void) {
  queue_t *queue = queue_create(10);
  TEST_ASSERT_NOT_NULL(queue);
  
  // New queue should have size 0
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  
  queue_destroy(queue);
}

void test_queue_get_size_with_add_and_pop(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Initially size 0
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  
  // Add messages and verify size increases
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  queue_add(queue, msg1);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  queue_add(queue, msg2);
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  queue_add(queue, msg3);
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));
  
  // Pop messages and verify size decreases
  message_t *popped1 = queue_pop(queue);
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  free(popped1->body);
  free(popped1);
  
  message_t *popped2 = queue_pop(queue);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  free(popped2->body);
  free(popped2);
  
  message_t *popped3 = queue_pop(queue);
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  free(popped3->body);
  free(popped3);
  
  queue_destroy(queue);
}

void test_queue_get_size_at_capacity(void) {
  queue_t *queue = queue_create(3);
  TEST_ASSERT_NOT_NULL(queue);
  
  // Fill queue to capacity and verify size
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  
  queue_add(queue, msg1);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  
  queue_add(queue, msg2);
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  
  queue_add(queue, msg3);
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));
  
  // Verify size equals capacity when full
  TEST_ASSERT_EQUAL(1, queue_is_full(queue));
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));
  
  // Clean up
  for (int i = 0; i < 3; i++) {
    message_t *msg = queue_pop(queue);
    free(msg->body);
    free(msg);
  }
  
  queue_destroy(queue);
}

void test_queue_get_size_with_try_operations(void) {
  queue_t *queue = queue_create(2);
  TEST_ASSERT_NOT_NULL(queue);
  
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  
  // Use try_add operations
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  TEST_ASSERT_EQUAL(0, queue_try_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  
  // Queue is full, try_add should fail but size should remain same
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  TEST_ASSERT_EQUAL(-1, queue_try_add(queue, msg3));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  free(msg3->body);
  free(msg3);
  
  // Use try_pop operations
  message_t *popped1 = queue_try_pop(queue);
  TEST_ASSERT_NOT_NULL(popped1);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  free(popped1->body);
  free(popped1);
  
  message_t *popped2 = queue_try_pop(queue);
  TEST_ASSERT_NOT_NULL(popped2);
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  free(popped2->body);
  free(popped2);
  
  // Queue is empty, try_pop should return NULL but size should remain 0
  TEST_ASSERT_NULL(queue_try_pop(queue));
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  
  queue_destroy(queue);
}

void test_queue_destroy_with_cleanup_empty_queue(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    cleanup_tracker_t tracker = {0};
    message_t* cleaned[10];
    tracker.cleaned_messages = cleaned;
    tracker.messages_capacity = 10;
    
    // Destroy empty queue with cleanup callback
    queue_destroy_with_cleanup(queue, test_cleanup_callback, &tracker);
    
    // Should not call cleanup for empty queue
    TEST_ASSERT_EQUAL(0, tracker.cleanup_call_count);
}

void test_queue_destroy_with_cleanup_single_message(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    message_t *msg = create_test_message(MSG_ECHO, "cleanup test");
    queue_add(queue, msg);
    
    cleanup_tracker_t tracker = {0};
    message_t* cleaned[10];
    tracker.cleaned_messages = cleaned;
    tracker.messages_capacity = 10;
    
    queue_destroy_with_cleanup(queue, test_cleanup_callback, &tracker);
    
    // Should call cleanup once
    TEST_ASSERT_EQUAL(1, tracker.cleanup_call_count);
    TEST_ASSERT_EQUAL(msg, tracker.cleaned_messages[0]);
}

void test_queue_destroy_with_cleanup_multiple_messages(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Add multiple messages
    message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
    message_t *msg2 = create_test_message(MSG_REVERSE, "msg2");  
    message_t *msg3 = create_test_message(MSG_TIME, "msg3");
    
    queue_add(queue, msg1);
    queue_add(queue, msg2);
    queue_add(queue, msg3);
    
    cleanup_tracker_t tracker = {0};
    message_t* cleaned[10];
    tracker.cleaned_messages = cleaned;
    tracker.messages_capacity = 10;
    
    queue_destroy_with_cleanup(queue, test_cleanup_callback, &tracker);
    
    // Should call cleanup for all 3 messages
    TEST_ASSERT_EQUAL(3, tracker.cleanup_call_count);
    
    // Verify all messages were cleaned (order may vary due to queue structure)
    int found_msg1 = 0, found_msg2 = 0, found_msg3 = 0;
    for (int i = 0; i < 3; i++) {
        if (tracker.cleaned_messages[i] == msg1) found_msg1 = 1;
        if (tracker.cleaned_messages[i] == msg2) found_msg2 = 1;
        if (tracker.cleaned_messages[i] == msg3) found_msg3 = 1;
    }
    TEST_ASSERT_EQUAL(1, found_msg1);
    TEST_ASSERT_EQUAL(1, found_msg2);
    TEST_ASSERT_EQUAL(1, found_msg3);
}

void test_queue_destroy_with_cleanup_null_callback(void) {
    queue_t *queue = queue_create(3);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Add messages that won't be cleaned up
    message_t *msg1 = create_test_message(MSG_ECHO, "leaked1");
    message_t *msg2 = create_test_message(MSG_ECHO, "leaked2");
    
    queue_add(queue, msg1);
    queue_add(queue, msg2);
    
    // Destroy with NULL cleanup callback - messages will be drained but not freed
    // This simulates the case where caller wants to handle cleanup elsewhere
    queue_destroy_with_cleanup(queue, NULL, NULL);
    
    // Note: In real usage this would leak memory, but for testing we just
    // verify the function handles NULL callback gracefully
    // In practice, the caller would be responsible for message cleanup
}

void test_queue_destroy_with_cleanup_partial_queue(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Add messages, pop some, leave others
    message_t *msg1 = create_test_message(MSG_ECHO, "pop_me");
    message_t *msg2 = create_test_message(MSG_ECHO, "cleanup_me");
    message_t *msg3 = create_test_message(MSG_ECHO, "cleanup_me_too");
    
    queue_add(queue, msg1);
    queue_add(queue, msg2);
    queue_add(queue, msg3);
    
    // Pop one message normally
    message_t *popped = queue_pop(queue);
    TEST_ASSERT_EQUAL(msg1, popped);
    free(popped->body);
    free(popped);
    
    // Destroy with remaining messages
    cleanup_tracker_t tracker = {0};
    message_t* cleaned[10];
    tracker.cleaned_messages = cleaned;
    tracker.messages_capacity = 10;
    
    queue_destroy_with_cleanup(queue, test_cleanup_callback, &tracker);
    
    // Should cleanup the 2 remaining messages
    TEST_ASSERT_EQUAL(2, tracker.cleanup_call_count);
}

void test_queue_destroy_with_cleanup_thread_safety(void) {
    queue_t *queue = queue_create(10);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Add messages from multiple threads first
    pthread_t producers[3];
    thread_data_t producer_data[3];
    
    for (int i = 0; i < 3; i++) {
        producer_data[i].queue = queue;
        producer_data[i].delay_ms = 10;
        producer_data[i].test_value = i;
        pthread_create(&producers[i], NULL, producer_thread, &producer_data[i]);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < 3; i++) {
        pthread_join(producers[i], NULL);
    }
    
    // Now destroy with cleanup - should handle all messages safely
    cleanup_tracker_t tracker = {0};
    message_t* cleaned[10];
    tracker.cleaned_messages = cleaned;
    tracker.messages_capacity = 10;
    
    queue_destroy_with_cleanup(queue, test_cleanup_callback, &tracker);
    
    // Should have cleaned up exactly 3 messages
    TEST_ASSERT_EQUAL(3, tracker.cleanup_call_count);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  UnityBegin("tst/queue_tests.c");
  RUN_TEST(test_queue_add_and_pop_message);
  RUN_TEST(test_queue_pop_blocks_until_push);
  RUN_TEST(test_queue_add_blocks_on_full_queue);
  RUN_TEST(test_queue_try_add_returns_error_on_full);
  RUN_TEST(test_queue_try_add_succeeds_with_space);
  RUN_TEST(test_queue_try_pop_returns_null_on_empty);
  RUN_TEST(test_queue_try_pop_returns_message);
  RUN_TEST(test_queue_try_operations_mixed);
  RUN_TEST(test_queue_is_empty_on_new_queue);
  RUN_TEST(test_queue_is_empty_after_add_and_pop);
  RUN_TEST(test_queue_is_empty_thread_safety);
  RUN_TEST(test_queue_is_full_on_new_queue);
  RUN_TEST(test_queue_is_full_after_filling_queue);
  RUN_TEST(test_queue_is_full_with_try_operations);
  RUN_TEST(test_queue_get_size_on_new_queue);
  RUN_TEST(test_queue_get_size_with_add_and_pop);
  RUN_TEST(test_queue_get_size_at_capacity);
  RUN_TEST(test_queue_get_size_with_try_operations);
  RUN_TEST(test_queue_destroy_with_cleanup_empty_queue);
  RUN_TEST(test_queue_destroy_with_cleanup_single_message);
  RUN_TEST(test_queue_destroy_with_cleanup_multiple_messages);
  RUN_TEST(test_queue_destroy_with_cleanup_null_callback);
  RUN_TEST(test_queue_destroy_with_cleanup_partial_queue);
  RUN_TEST(test_queue_destroy_with_cleanup_thread_safety);
  return (UnityEnd());
}
