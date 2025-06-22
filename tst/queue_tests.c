#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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

// Timeout test constants
#define TIMEOUT_TEST_SECONDS 2    // Shorter timeout for faster testing
#define TIMEOUT_MARGIN_MS 500     // Allow 500ms margin for timing variations

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

// Helper to get current time in milliseconds
static long long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Thread data for timeout testing
typedef struct {
    queue_t *queue;
    long long start_time;
    long long end_time;
    int operation_result;  // 0 = success, -1 = timeout/error
    message_t *message;    // For pop operations
} timeout_thread_data_t;

// Thread function for testing queue_pop timeout
void* timeout_pop_thread(void* arg) {
    timeout_thread_data_t *data = (timeout_thread_data_t*)arg;

    data->start_time = get_time_ms();
    data->operation_result = queue_pop(data->queue, &data->message);
    data->end_time = get_time_ms();
    return NULL;
}

// Thread function for testing queue_add timeout
void* timeout_add_thread(void* arg) {
    timeout_thread_data_t *data = (timeout_thread_data_t*)arg;

    data->start_time = get_time_ms();
    int result = queue_add(data->queue, data->message);
    data->end_time = get_time_ms();

    data->operation_result = result;
    return NULL;
}


void test_queue_add_and_pop_message(void) {
  queue_t *queue = queue_create(10);
  TEST_ASSERT_NOT_NULL(queue);

  // Always use dynamic allocation for messages
  message_t *msg = create_test_message(MSG_ECHO, TEST_MSG_SIMPLE);

  int result = queue_add(queue, msg);
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);

  message_t *popped_msg = NULL;
  int pop_result = queue_pop(queue, &popped_msg);
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, pop_result);
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
  // Result could be stored in thread_data if needed for verification

  return NULL;
}

void* consumer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;

  message_t *msg = NULL;
  int result = queue_pop(data->queue, &msg);

  data->test_value = (result == QUEUE_SUCCESS && msg != NULL) ? 1 : 0;

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
  int result = queue_add(data->queue, msg);

  // Set test_value based on success
  data->test_value = (result == QUEUE_SUCCESS) ? 1 : 0;

  return NULL;
}

void* delayed_consumer_thread(void* arg) {
  thread_data_t *data = (thread_data_t*)arg;

  // Wait before consuming to ensure producer blocks
  usleep(data->delay_ms * 1000);

  // Pop one message to make space
  message_t *msg = NULL;
  queue_pop(data->queue, &msg);
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

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));

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
    message_t *remaining = NULL;
    queue_pop(queue, &remaining);
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

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));

  // Queue is now full, try_add should return QUEUE_ERR_FULL
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  int result = queue_try_add(queue, msg3);
  TEST_ASSERT_EQUAL(QUEUE_ERR_FULL, result);

  // Clean up msg3 since it wasn't added
  free(msg3->body);
  free(msg3);

  // Clean up queue messages
  for (int i = 0; i < 2; i++) {
    message_t *msg = NULL;
    queue_pop(queue, &msg);
    if (msg) {
      free(msg->body);
      free(msg);
    }
  }

  queue_destroy(queue);
}

void test_queue_try_add_succeeds_with_space(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);

  message_t *msg = create_test_message(MSG_ECHO, "test message");

  // Try to add to empty queue, should succeed
  int result = queue_try_add(queue, msg);
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);

  // Verify message was added
  message_t *popped = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_pop(queue, &popped));
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
  message_t *msg = NULL;
  int result = queue_try_pop(queue, &msg);
  TEST_ASSERT_EQUAL(QUEUE_ERR_EMPTY, result);
  TEST_ASSERT_NULL(msg);

  queue_destroy(queue);
}

void test_queue_try_pop_returns_message(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);

  message_t *msg = create_test_message(MSG_ECHO, "pop test");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg));

  // Try to pop, should get the message
  message_t *popped = NULL;
  int result = queue_try_pop(queue, &popped);
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);
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

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg3));

  // Queue is full now
  message_t *msg4 = create_test_message(MSG_ECHO, "msg4");
  TEST_ASSERT_EQUAL(QUEUE_ERR_FULL, queue_try_add(queue, msg4));
  free(msg4->body);
  free(msg4);

  // Pop using both methods
  message_t *p1 = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_pop(queue, &p1));
  TEST_ASSERT_NOT_NULL(p1);
  TEST_ASSERT_EQUAL_STRING("msg1", p1->body);
  free(p1->body);
  free(p1);

  message_t *p2 = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_pop(queue, &p2));
  TEST_ASSERT_NOT_NULL(p2);
  TEST_ASSERT_EQUAL_STRING("msg2", p2->body);
  free(p2->body);
  free(p2);

  message_t *p3 = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_pop(queue, &p3));
  TEST_ASSERT_NOT_NULL(p3);
  TEST_ASSERT_EQUAL_STRING("msg3", p3->body);
  free(p3->body);
  free(p3);

  // Queue should be empty now
  message_t *empty = NULL;
  TEST_ASSERT_EQUAL(QUEUE_ERR_EMPTY, queue_try_pop(queue, &empty));
  TEST_ASSERT_NULL(empty);

  queue_destroy(queue);
}

void test_queue_is_empty_on_new_queue(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);

  // New queue should be empty
  TEST_ASSERT_TRUE(queue_is_empty(queue));

  queue_destroy(queue);
}

void test_queue_is_empty_after_add_and_pop(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Initially empty
  TEST_ASSERT_TRUE(queue_is_empty(queue));

  // Add a message, should not be empty
  message_t *msg = create_test_message(MSG_ECHO, "test");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg));
  TEST_ASSERT_FALSE(queue_is_empty(queue));

  // Pop the message, should be empty again
  message_t *popped = NULL;
  queue_pop(queue, &popped);
  TEST_ASSERT_TRUE(queue_is_empty(queue));

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
  TEST_ASSERT_TRUE(queue_is_empty(queue));

  // Start producer after small delay
  pthread_create(&producer, NULL, producer_thread, &producer_data);

  pthread_join(consumer, NULL);
  pthread_join(producer, NULL);

  // Queue should be empty after consumer got the message
  TEST_ASSERT_TRUE(queue_is_empty(queue));

  queue_destroy(queue);
}

void test_queue_is_full_on_new_queue(void) {
  queue_t *queue = queue_create(5);
  TEST_ASSERT_NOT_NULL(queue);

  // New queue should not be full
  TEST_ASSERT_FALSE(queue_is_full(queue));

  queue_destroy(queue);
}

void test_queue_is_full_after_filling_queue(void) {
  queue_t *queue = queue_create(2);  // Small capacity for easy testing
  TEST_ASSERT_NOT_NULL(queue);

  // Initially not full
  TEST_ASSERT_FALSE(queue_is_full(queue));

  // Add first message, should not be full yet
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
  TEST_ASSERT_FALSE(queue_is_full(queue));

  // Add second message, should now be full
  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
  TEST_ASSERT_TRUE(queue_is_full(queue));

  // Pop one message, should not be full anymore
  message_t *popped = NULL;
  queue_pop(queue, &popped);
  TEST_ASSERT_FALSE(queue_is_full(queue));

  // Clean up
  free(popped->body);
  free(popped);

  // Clean up remaining message
  message_t *remaining = NULL;
  queue_pop(queue, &remaining);
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

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg1));
  TEST_ASSERT_FALSE(queue_is_full(queue));

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg2));
  TEST_ASSERT_FALSE(queue_is_full(queue));

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg3));
  TEST_ASSERT_TRUE(queue_is_full(queue));

  // Try to add another message, should fail since queue is full
  message_t *msg4 = create_test_message(MSG_ECHO, "msg4");
  TEST_ASSERT_EQUAL(QUEUE_ERR_FULL, queue_try_add(queue, msg4));
  TEST_ASSERT_TRUE(queue_is_full(queue));

  // Clean up unused message
  free(msg4->body);
  free(msg4);

  // Clean up queue
  for (int i = 0; i < 3; i++) {
    message_t *msg = NULL;
    queue_try_pop(queue, &msg);
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
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));

  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));

  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg3));
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));

  // Pop messages and verify size decreases
  message_t *popped1 = NULL;
  queue_pop(queue, &popped1);
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  free(popped1->body);
  free(popped1);

  message_t *popped2 = NULL;
  queue_pop(queue, &popped2);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  free(popped2->body);
  free(popped2);

  message_t *popped3 = NULL;
  queue_pop(queue, &popped3);
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

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));

  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg3));
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));

  // Verify size equals capacity when full
  TEST_ASSERT_TRUE(queue_is_full(queue));
  TEST_ASSERT_EQUAL(3, queue_get_size(queue));

  // Clean up
  for (int i = 0; i < 3; i++) {
    message_t *msg = NULL;
    queue_pop(queue, &msg);
    if (msg) {
      free(msg->body);
      free(msg);
    }
  }

  queue_destroy(queue);
}

void test_queue_get_size_with_try_operations(void) {
  queue_t *queue = queue_create(2);
  TEST_ASSERT_NOT_NULL(queue);

  TEST_ASSERT_EQUAL(0, queue_get_size(queue));

  // Use try_add operations
  message_t *msg1 = create_test_message(MSG_ECHO, "msg1");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));

  message_t *msg2 = create_test_message(MSG_ECHO, "msg2");
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));

  // Queue is full, try_add should fail but size should remain same
  message_t *msg3 = create_test_message(MSG_ECHO, "msg3");
  TEST_ASSERT_EQUAL(QUEUE_ERR_FULL, queue_try_add(queue, msg3));
  TEST_ASSERT_EQUAL(2, queue_get_size(queue));
  free(msg3->body);
  free(msg3);

  // Use try_pop operations
  message_t *popped1 = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_pop(queue, &popped1));
  TEST_ASSERT_NOT_NULL(popped1);
  TEST_ASSERT_EQUAL(1, queue_get_size(queue));
  free(popped1->body);
  free(popped1);

  message_t *popped2 = NULL;
  TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_try_pop(queue, &popped2));
  TEST_ASSERT_NOT_NULL(popped2);
  TEST_ASSERT_EQUAL(0, queue_get_size(queue));
  free(popped2->body);
  free(popped2);

  // Queue is empty, try_pop should return error but size should remain 0
  message_t *empty = NULL;
  TEST_ASSERT_EQUAL(QUEUE_ERR_EMPTY, queue_try_pop(queue, &empty));
  TEST_ASSERT_NULL(empty);
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
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg));

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

    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg3));

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

    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));

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

    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg3));

    // Pop one message normally
    message_t *popped = NULL;
    queue_pop(queue, &popped);
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

void test_queue_pop_timeout_on_empty_queue(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);

    pthread_t thread;
    timeout_thread_data_t thread_data = {0};
    thread_data.queue = queue;

    // Start thread that will try to pop from empty queue
    pthread_create(&thread, NULL, timeout_pop_thread, &thread_data);

    // Wait for thread to complete (should timeout)
    pthread_join(thread, NULL);

    // Verify timeout occurred
    TEST_ASSERT_EQUAL(-1, thread_data.operation_result);
    TEST_ASSERT_NULL(thread_data.message);

    // Verify timing (should be around 2 seconds timeout)
    long long elapsed_ms = thread_data.end_time - thread_data.start_time;
    long long expected_timeout_ms = 2 * 1000;

    // Allow some margin for timing variations
    TEST_ASSERT_GREATER_OR_EQUAL(expected_timeout_ms - TIMEOUT_MARGIN_MS, elapsed_ms);
    TEST_ASSERT_LESS_OR_EQUAL(expected_timeout_ms + TIMEOUT_MARGIN_MS, elapsed_ms);

    queue_destroy(queue);
}

void test_queue_add_timeout_on_full_queue(void) {
    queue_t *queue = queue_create(2);  // Small capacity
    TEST_ASSERT_NOT_NULL(queue);

    // Fill the queue to capacity
    message_t *msg1 = create_test_message(MSG_ECHO, "fill1");
    message_t *msg2 = create_test_message(MSG_ECHO, "fill2");
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));

    // Try to add another message - should timeout
    pthread_t thread;
    timeout_thread_data_t thread_data = {0};
    thread_data.queue = queue;
    thread_data.message = create_test_message(MSG_ECHO, "timeout_msg");

    pthread_create(&thread, NULL, timeout_add_thread, &thread_data);
    pthread_join(thread, NULL);

    // Verify timeout occurred by checking operation_result
    TEST_ASSERT_EQUAL(QUEUE_ERR_TIMEOUT, thread_data.operation_result);

    // Verify timing (should timeout after 2 seconds)
    long long elapsed_ms = thread_data.end_time - thread_data.start_time;
    long long expected_timeout_ms = 2 * 1000;

    TEST_ASSERT_GREATER_OR_EQUAL(expected_timeout_ms - TIMEOUT_MARGIN_MS, elapsed_ms);
    TEST_ASSERT_LESS_OR_EQUAL(expected_timeout_ms + TIMEOUT_MARGIN_MS, elapsed_ms);

    // Clean up - the timeout message may or may not have been added
    // depending on exact timing, so clean up safely
    message_t *remaining = NULL;
    while (queue_try_pop(queue, &remaining) == QUEUE_SUCCESS) {
        free(remaining->body);
        free(remaining);
        remaining = NULL;
    }

    // Clean up timeout message if it wasn't added
    if (thread_data.message) {
        free(thread_data.message->body);
        free(thread_data.message);
    }

    queue_destroy(queue);
}

void test_queue_pop_succeeds_before_timeout(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);

    pthread_t pop_thread, add_thread;
    timeout_thread_data_t pop_data = {0};
    thread_data_t add_data = {queue, 1000, 42}; // Add message after 1 second

    pop_data.queue = queue;

    // Start pop thread first (will wait)
    pthread_create(&pop_thread, NULL, timeout_pop_thread, &pop_data);

    // Start add thread with delay (should unblock pop before timeout)
    pthread_create(&add_thread, NULL, producer_thread, &add_data);

    pthread_join(pop_thread, NULL);
    pthread_join(add_thread, NULL);

    // Verify pop succeeded (didn't timeout)
    TEST_ASSERT_EQUAL(0, pop_data.operation_result);
    TEST_ASSERT_NOT_NULL(pop_data.message);

    // Verify timing was less than timeout
    long long elapsed_ms = pop_data.end_time - pop_data.start_time;
    long long timeout_ms = 2 * 1000;
    TEST_ASSERT_LESS_THAN(timeout_ms, elapsed_ms);

    // Clean up message
    free(pop_data.message->body);
    free(pop_data.message);

    queue_destroy(queue);
}

void test_queue_add_succeeds_before_timeout(void) {
    queue_t *queue = queue_create(2);
    TEST_ASSERT_NOT_NULL(queue);

    // Fill queue
    message_t *msg1 = create_test_message(MSG_ECHO, "fill1");
    message_t *msg2 = create_test_message(MSG_ECHO, "fill2");
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));

    pthread_t add_thread, pop_thread;
    timeout_thread_data_t add_data = {0};
    thread_data_t pop_data = {queue, 1000, 0}; // Pop after 1 second

    add_data.queue = queue;
    add_data.message = create_test_message(MSG_ECHO, "add_test");

    // Start add thread (will block on full queue)
    pthread_create(&add_thread, NULL, timeout_add_thread, &add_data);

    // Start delayed consumer to make space
    pthread_create(&pop_thread, NULL, delayed_consumer_thread, &pop_data);

    pthread_join(add_thread, NULL);
    pthread_join(pop_thread, NULL);

    // Verify add completed before timeout
    long long elapsed_ms = add_data.end_time - add_data.start_time;
    long long timeout_ms = 2 * 1000;
    TEST_ASSERT_LESS_THAN(timeout_ms, elapsed_ms);

    // Clean up remaining messages
    message_t *remaining = NULL;
    while (queue_try_pop(queue, &remaining) == QUEUE_SUCCESS) {
        free(remaining->body);
        free(remaining);
        remaining = NULL;
    }

    queue_destroy(queue);
}

void test_queue_timeout_thread_safety(void) {
    queue_t *queue = queue_create(3);
    TEST_ASSERT_NOT_NULL(queue);

    // Test multiple threads timing out simultaneously
    pthread_t pop_threads[3];
    timeout_thread_data_t pop_data[3];

    for (int i = 0; i < 3; i++) {
        pop_data[i].queue = queue;
        pop_data[i].message = NULL;
        pop_data[i].operation_result = 0;
        pthread_create(&pop_threads[i], NULL, timeout_pop_thread, &pop_data[i]);
    }

    // Wait for all threads to timeout
    for (int i = 0; i < 3; i++) {
        pthread_join(pop_threads[i], NULL);
    }

    // Verify all threads timed out properly
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL(-1, pop_data[i].operation_result);
        TEST_ASSERT_NULL(pop_data[i].message);
    }

    queue_destroy(queue);
}

void test_queue_timeout_error_conditions(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);

    // Test that timeout doesn't interfere with normal operations
    message_t *msg = create_test_message(MSG_ECHO, "normal_op");

    // Add should succeed immediately on empty queue
    long long start = get_time_ms();
    int result = queue_add(queue, msg);
    long long elapsed = get_time_ms() - start;
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);

    // Should complete very quickly, not wait for timeout
    TEST_ASSERT_LESS_THAN(1000, elapsed); // Less than 1 second

    // Pop should succeed immediately on non-empty queue
    start = get_time_ms();
    message_t *popped = NULL;
    queue_pop(queue, &popped);
    elapsed = get_time_ms() - start;

    TEST_ASSERT_NOT_NULL(popped);
    TEST_ASSERT_LESS_THAN(1000, elapsed); // Less than 1 second

    free(popped->body);
    free(popped);
    queue_destroy(queue);
}

void test_queue_add_returns_error_on_null_queue(void) {
    message_t *msg = create_test_message(MSG_ECHO, "test");
    int result = queue_add(NULL, msg);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    free(msg->body);
    free(msg);
}

void test_queue_add_returns_error_on_null_message(void) {
    queue_t *queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    int result = queue_add(queue, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    
    queue_destroy(queue);
}

void test_queue_add_timeout_returns_error_code(void) {
    queue_t *queue = queue_create(2);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Fill the queue
    message_t *msg1 = create_test_message(MSG_ECHO, "fill1");
    message_t *msg2 = create_test_message(MSG_ECHO, "fill2");
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg1));
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg2));
    
    // Try to add another - should timeout
    message_t *timeout_msg = create_test_message(MSG_ECHO, "timeout");
    long long start = get_time_ms();
    int result = queue_add(queue, timeout_msg);
    long long elapsed = get_time_ms() - start;
    
    TEST_ASSERT_EQUAL(QUEUE_ERR_TIMEOUT, result);
    TEST_ASSERT_GREATER_OR_EQUAL(2000 - TIMEOUT_MARGIN_MS, elapsed);
    
    // Clean up
    free(timeout_msg->body);
    free(timeout_msg);
    message_t *remaining = NULL;
    while (queue_try_pop(queue, &remaining) == QUEUE_SUCCESS) {
        free(remaining->body);
        free(remaining);
        remaining = NULL;
    }
    queue_destroy(queue);
}

// Test errno functions
void test_queue_get_error_initial_state(void) {
    // Verify error starts at QUEUE_SUCCESS
    queue_clear_error();
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
}

void test_queue_get_error_after_null_parameter(void) {
    queue_clear_error();
    int result = queue_add(NULL, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
}

void test_queue_clear_error_resets_state(void) {
    // Set an error condition
    queue_add(NULL, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    // Clear error
    queue_clear_error();
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
}

void test_queue_strerror_returns_correct_messages(void) {
    TEST_ASSERT_EQUAL_STRING("Success", queue_strerror(QUEUE_SUCCESS));
    TEST_ASSERT_EQUAL_STRING("Null pointer parameter", queue_strerror(QUEUE_ERR_NULL));
    TEST_ASSERT_EQUAL_STRING("Operation timed out", queue_strerror(QUEUE_ERR_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("Thread operation failed", queue_strerror(QUEUE_ERR_THREAD));
    TEST_ASSERT_EQUAL_STRING("Memory allocation failed", queue_strerror(QUEUE_ERR_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Queue is full", queue_strerror(QUEUE_ERR_FULL));
    TEST_ASSERT_EQUAL_STRING("Queue is empty", queue_strerror(QUEUE_ERR_EMPTY));
    TEST_ASSERT_EQUAL_STRING("Invalid parameter", queue_strerror(QUEUE_ERR_INVALID));
    TEST_ASSERT_EQUAL_STRING("Integer overflow in capacity calculation", queue_strerror(QUEUE_ERR_OVERFLOW));
    TEST_ASSERT_EQUAL_STRING("Unknown error", queue_strerror(999));
}

// Test thread-local error handling
void* thread_local_error_test(void* arg) {
    (void)arg; // Unused parameter
    
    // Each thread should have its own error state
    queue_clear_error();
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
    
    // Trigger an error in this thread
    queue_add(NULL, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    return NULL;
}

void test_queue_errno_is_thread_local(void) {
    queue_t* queue = queue_create(5);
    
    // Main thread error state
    queue_clear_error();
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
    
    // Create thread that sets its own error
    pthread_t thread;
    pthread_create(&thread, NULL, thread_local_error_test, queue);
    pthread_join(thread, NULL);
    
    // Main thread error should still be SUCCESS
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
    
    queue_destroy(queue);
}

// New API tests
void test_queue_create_with_zero_capacity(void) {
    queue_clear_error();
    queue_t* queue = queue_create(0);
    TEST_ASSERT_NULL(queue);
    TEST_ASSERT_EQUAL(QUEUE_ERR_INVALID, queue_get_error());
}

void test_queue_destroy_returns_success(void) {
    queue_t* queue = queue_create(5);
    TEST_ASSERT_NOT_NULL(queue);
    
    queue_clear_error();
    int result = queue_destroy(queue);
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
}

void test_queue_destroy_with_null_returns_error(void) {
    queue_clear_error();
    int result = queue_destroy(NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
}

void test_queue_pop_with_output_parameter(void) {
    queue_t* queue = queue_create(5);
    message_t* msg_in = create_test_message(MSG_ECHO, "test");
    
    queue_add(queue, msg_in);
    
    message_t* msg_out = NULL;
    queue_clear_error();
    int result = queue_pop(queue, &msg_out);
    
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(msg_out);
    TEST_ASSERT_EQUAL_STRING("test", msg_out->body);
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
    
    free(msg_out->body);
    free(msg_out);
    queue_destroy(queue);
}

void test_queue_pop_with_null_parameters(void) {
    queue_t* queue = queue_create(5);
    message_t* msg = NULL;
    
    // Test NULL queue
    queue_clear_error();
    int result = queue_pop(NULL, &msg);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    // Test NULL output parameter
    queue_clear_error();
    result = queue_pop(queue, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    queue_destroy(queue);
}

void test_queue_pop_timeout_with_new_api(void) {
    queue_t* queue = queue_create(5);
    message_t* msg = NULL;
    
    queue_clear_error();
    long long start = get_time_ms();
    int result = queue_pop(queue, &msg);
    long long elapsed = get_time_ms() - start;
    
    TEST_ASSERT_EQUAL(QUEUE_ERR_TIMEOUT, result);
    TEST_ASSERT_NULL(msg);
    TEST_ASSERT_EQUAL(QUEUE_ERR_TIMEOUT, queue_get_error());
    TEST_ASSERT_GREATER_OR_EQUAL(2000 - TIMEOUT_MARGIN_MS, elapsed);
    
    queue_destroy(queue);
}

void test_queue_try_pop_with_output_parameter(void) {
    queue_t* queue = queue_create(5);
    message_t* msg_in = create_test_message(MSG_ECHO, "test");
    queue_add(queue, msg_in);
    
    message_t* msg_out = NULL;
    queue_clear_error();
    int result = queue_try_pop(queue, &msg_out);
    
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(msg_out);
    TEST_ASSERT_EQUAL_STRING("test", msg_out->body);
    TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
    
    free(msg_out->body);
    free(msg_out);
    queue_destroy(queue);
}

void test_queue_try_add_null_parameters(void) {
    queue_t* queue = queue_create(5);
    
    // Test NULL queue
    queue_clear_error();
    int result = queue_try_add(NULL, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    // Test NULL message
    queue_clear_error();
    result = queue_try_add(queue, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    queue_destroy(queue);
}

void test_queue_try_pop_null_parameters(void) {
    queue_t* queue = queue_create(5);
    message_t* msg = NULL;
    
    // Test NULL queue
    queue_clear_error();
    int result = queue_try_pop(NULL, &msg);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    // Test NULL output parameter
    queue_clear_error();
    result = queue_try_pop(queue, NULL);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
    
    queue_destroy(queue);
}

void test_queue_is_empty_with_null_queue(void) {
    queue_clear_error();
    bool result = queue_is_empty(NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
}

void test_queue_is_full_with_null_queue(void) {
    queue_clear_error();
    bool result = queue_is_full(NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
}

void test_queue_get_size_with_null_queue(void) {
    queue_clear_error();
    size_t size = queue_get_size(NULL);
    
    TEST_ASSERT_EQUAL(0, size);
    TEST_ASSERT_EQUAL(QUEUE_ERR_NULL, queue_get_error());
}

// Test overflow detection in queue_create
void test_queue_create_with_overflow_capacity(void) {
    queue_clear_error();
    
    // Try to create queue with capacity that would overflow
    size_t huge_capacity = SIZE_MAX / sizeof(message_t*) + 1;
    queue_t* queue = queue_create(huge_capacity);
    
    TEST_ASSERT_NULL(queue);
    TEST_ASSERT_EQUAL(QUEUE_ERR_OVERFLOW, queue_get_error());
}

void test_queue_create_with_max_capacity(void) {
    queue_clear_error();
    
    // Try to create queue with exactly max allowed capacity
    size_t max_capacity = SIZE_MAX / sizeof(message_t*) / 2;
    queue_t* queue = queue_create(max_capacity);
    
    // This should fail - either due to safety limit or memory allocation
    TEST_ASSERT_NULL(queue);
    int error = queue_get_error();
    // Could be QUEUE_ERR_OVERFLOW if caught by limit check, or QUEUE_ERR_MEMORY if allocation fails
    TEST_ASSERT_TRUE(error == QUEUE_ERR_OVERFLOW || error == QUEUE_ERR_MEMORY);
}

void test_queue_create_with_safe_large_capacity(void) {
    queue_clear_error();
    
    // Create queue with large but safe capacity
    size_t large_capacity = 1000000; // 1 million
    queue_t* queue = queue_create(large_capacity);
    
    // This should succeed if we have enough memory
    if (queue != NULL) {
        TEST_ASSERT_NOT_NULL(queue);
        TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_get_error());
        
        // Verify basic operations work
        message_t* msg = create_test_message(MSG_ECHO, "test");
        TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_add(queue, msg));
        
        message_t* popped = NULL;
        TEST_ASSERT_EQUAL(QUEUE_SUCCESS, queue_pop(queue, &popped));
        TEST_ASSERT_NOT_NULL(popped);
        
        free(popped->body);
        free(popped);
        queue_destroy(queue);
    } else {
        // If allocation failed, it should be due to memory limits, not overflow
        TEST_ASSERT_EQUAL(QUEUE_ERR_MEMORY, queue_get_error());
    }
}

void test_queue_create_memory_allocation_failure(void) {
    queue_clear_error();
    
    // Try to create queue with very large capacity that might fail allocation
    // but not due to overflow
    size_t huge_capacity = SIZE_MAX / sizeof(message_t*) / 4;
    queue_t* queue = queue_create(huge_capacity);
    
    if (queue == NULL) {
        // Should fail with either overflow (due to safety limit) or memory error
        int error = queue_get_error();
        TEST_ASSERT_TRUE(error == QUEUE_ERR_OVERFLOW || error == QUEUE_ERR_MEMORY);
    } else {
        // If it succeeded, clean up
        queue_destroy(queue);
    }
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
  RUN_TEST(test_queue_pop_timeout_on_empty_queue);
  RUN_TEST(test_queue_add_timeout_on_full_queue);
  RUN_TEST(test_queue_pop_succeeds_before_timeout);
  RUN_TEST(test_queue_add_succeeds_before_timeout);
  RUN_TEST(test_queue_timeout_thread_safety);
  RUN_TEST(test_queue_timeout_error_conditions);
  RUN_TEST(test_queue_add_returns_error_on_null_queue);
  RUN_TEST(test_queue_add_returns_error_on_null_message);
  RUN_TEST(test_queue_add_timeout_returns_error_code);
  
  // New errno-style error handling tests
  RUN_TEST(test_queue_get_error_initial_state);
  RUN_TEST(test_queue_get_error_after_null_parameter);
  RUN_TEST(test_queue_clear_error_resets_state);
  RUN_TEST(test_queue_strerror_returns_correct_messages);
  RUN_TEST(test_queue_errno_is_thread_local);
  
  // New API tests
  RUN_TEST(test_queue_create_with_zero_capacity);
  RUN_TEST(test_queue_destroy_returns_success);
  RUN_TEST(test_queue_destroy_with_null_returns_error);
  RUN_TEST(test_queue_pop_with_output_parameter);
  RUN_TEST(test_queue_pop_with_null_parameters);
  RUN_TEST(test_queue_pop_timeout_with_new_api);
  RUN_TEST(test_queue_try_pop_with_output_parameter);
  RUN_TEST(test_queue_try_add_null_parameters);
  RUN_TEST(test_queue_try_pop_null_parameters);
  RUN_TEST(test_queue_is_empty_with_null_queue);
  RUN_TEST(test_queue_is_full_with_null_queue);
  RUN_TEST(test_queue_get_size_with_null_queue);
  
  // Overflow detection tests
  RUN_TEST(test_queue_create_with_overflow_capacity);
  RUN_TEST(test_queue_create_with_max_capacity);
  RUN_TEST(test_queue_create_with_safe_large_capacity);
  RUN_TEST(test_queue_create_memory_allocation_failure);
  
  return (UnityEnd());
}
