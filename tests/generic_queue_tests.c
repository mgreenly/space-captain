#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "vendor/unity.h"

#include "../src/generic_queue.h"

// Unity test framework functions
void setUp(void);
void tearDown(void);

// Thread functions
void *producer_thread(void *arg);
void *consumer_thread(void *arg);
void *blocking_producer_thread(void *arg);
void *delayed_consumer_thread(void *arg);
void *timeout_pop_thread(void *arg);
void *timeout_add_thread(void *arg);
void *thread_local_error_test(void *arg);

// Test functions
void test_queue_add_and_pop_item(void);
void test_queue_pop_blocks_until_push(void);
void test_queue_add_blocks_on_full_queue(void);
void test_queue_try_add_returns_error_on_full(void);
void test_queue_try_add_succeeds_with_space(void);
void test_queue_try_pop_returns_null_on_empty(void);
void test_queue_try_pop_returns_message(void);
void test_queue_try_operations_mixed(void);
void test_queue_is_empty_on_new_queue(void);
void test_queue_is_empty_after_add_and_pop(void);
void test_queue_is_empty_thread_safety(void);
void test_queue_is_full_on_new_queue(void);
void test_queue_is_full_after_filling_queue(void);
void test_queue_is_full_with_try_operations(void);
void test_queue_get_size_on_new_queue(void);
void test_queue_get_size_with_add_and_pop(void);
void test_queue_get_size_at_capacity(void);
void test_queue_get_size_with_try_operations(void);
void test_queue_nuke_with_cleanup_empty_queue(void);
void test_queue_nuke_with_cleanup_single_item(void);
void test_queue_nuke_with_cleanup_multiple_items(void);
void test_queue_nuke_with_cleanup_null_callback(void);
void test_queue_nuke_with_cleanup_partial_queue(void);
void test_queue_nuke_with_cleanup_thread_safety(void);
void test_queue_pop_timeout_on_empty_queue(void);
void test_queue_add_timeout_on_full_queue(void);
void test_queue_pop_succeeds_before_timeout(void);
void test_queue_add_succeeds_before_timeout(void);
void test_queue_timeout_thread_safety(void);
void test_queue_timeout_error_conditions(void);
void test_queue_add_returns_error_on_null_queue(void);
void test_queue_add_returns_error_on_null_item(void);
void test_queue_add_timeout_returns_error_code(void);
void test_queue_get_error_initial_state(void);
void test_queue_get_error_after_null_parameter(void);
void test_queue_clear_error_resets_state(void);
void test_queue_strerror_returns_correct_messages(void);
void test_queue_errno_is_thread_local(void);
void test_queue_init_with_zero_capacity(void);
void test_queue_nuke_returns_success(void);
void test_queue_nuke_with_null_returns_error(void);
void test_queue_pop_with_output_parameter(void);
void test_queue_pop_with_null_parameters(void);
void test_queue_pop_timeout_with_new_api(void);
void test_queue_try_pop_with_output_parameter(void);
void test_queue_try_add_null_parameters(void);
void test_queue_try_pop_null_parameters(void);
void test_queue_is_empty_with_null_queue(void);
void test_queue_is_full_with_null_queue(void);
void test_queue_get_size_with_null_queue(void);
void test_queue_init_with_overflow_capacity(void);
void test_queue_init_with_max_capacity(void);
void test_queue_init_with_safe_large_capacity(void);
void test_queue_init_memory_allocation_failure(void);

// Test data structure for generic queue testing
typedef struct {
  int id;
  char *data;
} TestData;

// Timeout test constants
#define TIMEOUT_TEST_SECONDS 2   // Shorter timeout for faster testing
#define TIMEOUT_MARGIN_MS    500 // Allow 500ms margin for timing variations

// Helper function to create test data
static TestData *create_test_data(int id, const char *data_str) {
  TestData *td = malloc(sizeof(TestData));
  TEST_ASSERT_NOT_NULL_MESSAGE(td, "Failed to allocate TestData");

  td->id   = id;
  td->data = strdup(data_str);
  TEST_ASSERT_NOT_NULL_MESSAGE(td->data, "Failed to allocate data string");

  return td;
}

// Helper function to free test data
static void free_test_data(TestData *td) {
  if (td) {
    free(td->data);
    free(td);
  }
}

// Cleanup callback tracker for testing
typedef struct {
  int cleanup_call_count;
  TestData **cleaned_items;
  size_t items_capacity;
} cleanup_tracker_t;

// Test cleanup callback that tracks calls
static void test_cleanup_callback(void *item, void *user_data) {
  cleanup_tracker_t *tracker = (cleanup_tracker_t *) user_data;
  TestData *td               = (TestData *) item;

  // Store the item pointer for verification
  if ((size_t) tracker->cleanup_call_count < tracker->items_capacity) {
    tracker->cleaned_items[tracker->cleanup_call_count] = td;
  }

  tracker->cleanup_call_count++;

  // Free the test data as normal cleanup would do
  free_test_data(td);
}

// Helper to get current time in milliseconds
static long long get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (long long) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Thread data for timeout testing
typedef struct {
  sc_generic_queue_t *queue;
  long long start_time;
  long long end_time;
  sc_generic_queue_ret_val_t operation_result; // SC_GENERIC_QUEUE_SUCCESS or error code
  TestData *test_data;                         // For pop operations
} timeout_thread_data_t;

// Thread function for testing queue_pop timeout
void *timeout_pop_thread(void *arg) {
  timeout_thread_data_t *data = (timeout_thread_data_t *) arg;

  data->start_time       = get_time_ms();
  data->operation_result = sc_generic_queue_pop(data->queue, (void **) &data->test_data);
  data->end_time         = get_time_ms();
  return NULL;
}

// Thread function for testing queue_add timeout
void *timeout_add_thread(void *arg) {
  timeout_thread_data_t *data = (timeout_thread_data_t *) arg;

  data->start_time                  = get_time_ms();
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(data->queue, data->test_data);
  data->end_time                    = get_time_ms();

  data->operation_result = result;
  return NULL;
}

void test_queue_add_and_pop_item(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(10);
  TEST_ASSERT_NOT_NULL(queue);

  // Always use dynamic allocation for test data
  TestData *td = create_test_data(42, "This is the test data");

  sc_generic_queue_ret_val_t result = sc_generic_queue_add(queue, td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);

  TestData *popped_td                   = NULL;
  sc_generic_queue_ret_val_t pop_result = sc_generic_queue_pop(queue, (void **) &popped_td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, pop_result);
  TEST_ASSERT_NOT_NULL(popped_td);
  TEST_ASSERT_EQUAL(42, popped_td->id);
  TEST_ASSERT_EQUAL_STRING("This is the test data", popped_td->data);

  // Clean up dynamically allocated test data
  free_test_data(popped_td);

  sc_generic_queue_nuke(queue);
}

typedef struct {
  sc_generic_queue_t *queue;
  int delay_ms;
  int test_value;
} thread_data_t;

void *producer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  usleep((unsigned int) (data->delay_ms * 1000));

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Test data %d", data->test_value);

  TestData *td = create_test_data(data->test_value, buffer);

  sc_generic_queue_add(data->queue, td);
  // Result could be stored in thread_data if needed for verification

  return NULL;
}

void *consumer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  TestData *td                      = NULL;
  sc_generic_queue_ret_val_t result = sc_generic_queue_pop(data->queue, (void **) &td);

  data->test_value = (result == SC_GENERIC_QUEUE_SUCCESS && td != NULL) ? 1 : 0;

  if (td) {
    free_test_data(td);
  }

  return NULL;
}

void test_queue_pop_blocks_until_push(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
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

  sc_generic_queue_nuke(queue);
}

void *blocking_producer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  // Create and add test data
  TestData *td = create_test_data(99, "Blocked data");

  // This should block since queue is full
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(data->queue, td);

  // Set test_value based on success
  data->test_value = (result == SC_GENERIC_QUEUE_SUCCESS) ? 1 : 0;

  return NULL;
}

void *delayed_consumer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  // Wait before consuming to ensure producer blocks
  usleep((unsigned int) (data->delay_ms * 1000));

  // Pop one item to make space
  TestData *td = NULL;
  sc_generic_queue_pop(data->queue, (void **) &td);
  if (td) {
    free_test_data(td);
  }

  return NULL;
}

void test_queue_add_blocks_on_full_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2); // Small capacity
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue to capacity with dynamically allocated test data
  TestData *td1 = create_test_data(1, "data1");
  TestData *td2 = create_test_data(2, "data2");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  // Queue is now full

  pthread_t producer, consumer;
  thread_data_t producer_data = {queue, 0, 0};   // test_value will be set to 1 when add completes
  thread_data_t consumer_data = {queue, 100, 0}; // wait 100ms before consuming

  // Start producer that will block trying to add to full queue
  pthread_create(&producer, NULL, blocking_producer_thread, &producer_data);

  // Give producer time to reach the blocking point
  usleep(50000); // 50ms

  // Verify producer hasn't completed yet (still blocked)
  TEST_ASSERT_EQUAL(0, producer_data.test_value);

  // Start consumer that will free up space after delay
  pthread_create(&consumer, NULL, delayed_consumer_thread, &consumer_data);

  // Wait for both threads to complete
  pthread_join(producer, NULL);
  pthread_join(consumer, NULL);

  // Verify producer was able to add message after consumer freed space
  TEST_ASSERT_EQUAL(1, producer_data.test_value);

  // Clean up remaining items
  // The delayed_consumer_thread already freed one item (either td1 or td2)
  // The blocking_producer_thread added one item
  // So there should be exactly 2 items left in the queue
  for (int i = 0; i < 2; i++) {
    TestData *remaining = NULL;
    sc_generic_queue_pop(queue, (void **) &remaining);
    if (remaining) {
      free_test_data(remaining);
    }
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_try_add_returns_error_on_full(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2); // Small capacity
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue to capacity
  TestData *td1 = create_test_data(1, "data1");
  TestData *td2 = create_test_data(2, "data2");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  // Queue is now full, try_add should return SC_GENERIC_QUEUE_ERR_FULL
  TestData *td3                     = create_test_data(3, "data3");
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_add(queue, td3);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_FULL, result);

  // Clean up td3 since it wasn't added
  free_test_data(td3);

  // Clean up queue items
  for (int i = 0; i < 2; i++) {
    TestData *td = NULL;
    sc_generic_queue_pop(queue, (void **) &td);
    if (td) {
      free_test_data(td);
    }
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_try_add_succeeds_with_space(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  TestData *td = create_test_data(42, "test data");

  // Try to add to empty queue, should succeed
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_add(queue, td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);

  // Verify item was added
  TestData *popped = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_pop(queue, (void **) &popped));
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_EQUAL_STRING("test data", popped->data);

  free_test_data(popped);

  sc_generic_queue_nuke(queue);
}

void test_queue_try_pop_returns_null_on_empty(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Try to pop from empty queue
  TestData *msg                     = NULL;
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_pop(queue, (void **) &msg);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_EMPTY, result);
  TEST_ASSERT_NULL(msg);

  sc_generic_queue_nuke(queue);
}

void test_queue_try_pop_returns_message(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  TestData *msg = create_test_data(42, "pop test");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg));

  // Try to pop, should get the message
  TestData *popped                  = NULL;
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_pop(queue, (void **) &popped);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_EQUAL_STRING("pop test", popped->data);

  free_test_data(popped);

  sc_generic_queue_nuke(queue);
}

void test_queue_try_operations_mixed(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(3);
  TEST_ASSERT_NOT_NULL(queue);

  // Add messages using both methods
  TestData *msg1 = create_test_data(1, "msg1");
  TestData *msg2 = create_test_data(2, "msg2");
  TestData *msg3 = create_test_data(3, "msg3");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg3));

  // Queue is full now
  TestData *msg4 = create_test_data(4, "msg4");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_FULL, sc_generic_queue_try_add(queue, msg4));
  free_test_data(msg4);

  // Pop using both methods
  TestData *p1 = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_pop(queue, (void **) &p1));
  TEST_ASSERT_NOT_NULL(p1);
  TEST_ASSERT_EQUAL_STRING("msg1", p1->data);
  free_test_data(p1);

  TestData *p2 = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_pop(queue, (void **) &p2));
  TEST_ASSERT_NOT_NULL(p2);
  TEST_ASSERT_EQUAL_STRING("msg2", p2->data);
  free_test_data(p2);

  TestData *p3 = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_pop(queue, (void **) &p3));
  TEST_ASSERT_NOT_NULL(p3);
  TEST_ASSERT_EQUAL_STRING("msg3", p3->data);
  free_test_data(p3);

  // Queue should be empty now
  TestData *empty = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_EMPTY, sc_generic_queue_try_pop(queue, (void **) &empty));
  TEST_ASSERT_NULL(empty);

  sc_generic_queue_nuke(queue);
}

void test_queue_is_empty_on_new_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // New queue should be empty
  TEST_ASSERT_TRUE(sc_generic_queue_is_empty(queue));

  sc_generic_queue_nuke(queue);
}

void test_queue_is_empty_after_add_and_pop(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Initially empty
  TEST_ASSERT_TRUE(sc_generic_queue_is_empty(queue));

  // Add a message, should not be empty
  TestData *msg = create_test_data(42, "test");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg));
  TEST_ASSERT_FALSE(sc_generic_queue_is_empty(queue));

  // Pop the message, should be empty again
  TestData *popped = NULL;
  sc_generic_queue_pop(queue, (void **) &popped);
  TEST_ASSERT_TRUE(sc_generic_queue_is_empty(queue));

  // Clean up
  free_test_data(popped);
  sc_generic_queue_nuke(queue);
}

void test_queue_is_empty_thread_safety(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(10);
  TEST_ASSERT_NOT_NULL(queue);

  // Test that queue_is_empty works correctly with concurrent operations
  pthread_t producer, consumer;
  thread_data_t producer_data = {queue, 50, 42};
  thread_data_t consumer_data = {queue, 0, 0};

  // Start consumer first (will block on empty queue)
  pthread_create(&consumer, NULL, consumer_thread, &consumer_data);

  // Queue should still be empty initially
  TEST_ASSERT_TRUE(sc_generic_queue_is_empty(queue));

  // Start producer after small delay
  pthread_create(&producer, NULL, producer_thread, &producer_data);

  pthread_join(consumer, NULL);
  pthread_join(producer, NULL);

  // Queue should be empty after consumer got the message
  TEST_ASSERT_TRUE(sc_generic_queue_is_empty(queue));

  sc_generic_queue_nuke(queue);
}

void test_queue_is_full_on_new_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // New queue should not be full
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  sc_generic_queue_nuke(queue);
}

void test_queue_is_full_after_filling_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2); // Small capacity for easy testing
  TEST_ASSERT_NOT_NULL(queue);

  // Initially not full
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  // Add first message, should not be full yet
  TestData *msg1 = create_test_data(42, "msg1");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg1));
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  // Add second message, should now be full
  TestData *msg2 = create_test_data(42, "msg2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg2));
  TEST_ASSERT_TRUE(sc_generic_queue_is_full(queue));

  // Pop one message, should not be full anymore
  TestData *popped = NULL;
  sc_generic_queue_pop(queue, (void **) &popped);
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  // Clean up
  free_test_data(popped);

  // Clean up remaining message
  TestData *remaining = NULL;
  sc_generic_queue_pop(queue, (void **) &remaining);
  free_test_data(remaining);

  sc_generic_queue_nuke(queue);
}

void test_queue_is_full_with_try_operations(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(3);
  TEST_ASSERT_NOT_NULL(queue);

  // Fill queue using try_add operations
  TestData *msg1 = create_test_data(1, "msg1");
  TestData *msg2 = create_test_data(2, "msg2");
  TestData *msg3 = create_test_data(3, "msg3");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg1));
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg2));
  TEST_ASSERT_FALSE(sc_generic_queue_is_full(queue));

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg3));
  TEST_ASSERT_TRUE(sc_generic_queue_is_full(queue));

  // Try to add another message, should fail since queue is full
  TestData *msg4 = create_test_data(4, "msg4");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_FULL, sc_generic_queue_try_add(queue, msg4));
  TEST_ASSERT_TRUE(sc_generic_queue_is_full(queue));

  // Clean up unused message
  free_test_data(msg4);

  // Clean up queue
  for (int i = 0; i < 3; i++) {
    TestData *msg = NULL;
    sc_generic_queue_try_pop(queue, (void **) &msg);
    if (msg) {
      free_test_data(msg);
    }
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_get_size_on_new_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(10);
  TEST_ASSERT_NOT_NULL(queue);

  // New queue should have size 0
  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));

  sc_generic_queue_nuke(queue);
}

void test_queue_get_size_with_add_and_pop(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Initially size 0
  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));

  // Add messages and verify size increases
  TestData *msg1 = create_test_data(42, "msg1");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, sc_generic_queue_get_size(queue));

  TestData *msg2 = create_test_data(42, "msg2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, sc_generic_queue_get_size(queue));

  TestData *msg3 = create_test_data(42, "msg3");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg3));
  TEST_ASSERT_EQUAL(3, sc_generic_queue_get_size(queue));

  // Pop messages and verify size decreases
  TestData *popped1 = NULL;
  sc_generic_queue_pop(queue, (void **) &popped1);
  TEST_ASSERT_EQUAL(2, sc_generic_queue_get_size(queue));
  free_test_data(popped1);

  TestData *popped2 = NULL;
  sc_generic_queue_pop(queue, (void **) &popped2);
  TEST_ASSERT_EQUAL(1, sc_generic_queue_get_size(queue));
  free_test_data(popped2);

  TestData *popped3 = NULL;
  sc_generic_queue_pop(queue, (void **) &popped3);
  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));
  free_test_data(popped3);

  sc_generic_queue_nuke(queue);
}

void test_queue_get_size_at_capacity(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(3);
  TEST_ASSERT_NOT_NULL(queue);

  // Fill queue to capacity and verify size
  TestData *msg1 = create_test_data(1, "msg1");
  TestData *msg2 = create_test_data(2, "msg2");
  TestData *msg3 = create_test_data(3, "msg3");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, sc_generic_queue_get_size(queue));

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, sc_generic_queue_get_size(queue));

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, msg3));
  TEST_ASSERT_EQUAL(3, sc_generic_queue_get_size(queue));

  // Verify size equals capacity when full
  TEST_ASSERT_TRUE(sc_generic_queue_is_full(queue));
  TEST_ASSERT_EQUAL(3, sc_generic_queue_get_size(queue));

  // Clean up
  for (int i = 0; i < 3; i++) {
    TestData *msg = NULL;
    sc_generic_queue_pop(queue, (void **) &msg);
    if (msg) {
      free_test_data(msg);
    }
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_get_size_with_try_operations(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2);
  TEST_ASSERT_NOT_NULL(queue);

  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));

  // Use try_add operations
  TestData *msg1 = create_test_data(42, "msg1");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg1));
  TEST_ASSERT_EQUAL(1, sc_generic_queue_get_size(queue));

  TestData *msg2 = create_test_data(42, "msg2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_add(queue, msg2));
  TEST_ASSERT_EQUAL(2, sc_generic_queue_get_size(queue));

  // Queue is full, try_add should fail but size should remain same
  TestData *msg3 = create_test_data(42, "msg3");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_FULL, sc_generic_queue_try_add(queue, msg3));
  TEST_ASSERT_EQUAL(2, sc_generic_queue_get_size(queue));
  free_test_data(msg3);

  // Use try_pop operations
  TestData *popped1 = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_pop(queue, (void **) &popped1));
  TEST_ASSERT_NOT_NULL(popped1);
  TEST_ASSERT_EQUAL(1, sc_generic_queue_get_size(queue));
  free_test_data(popped1);

  TestData *popped2 = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_try_pop(queue, (void **) &popped2));
  TEST_ASSERT_NOT_NULL(popped2);
  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));
  free_test_data(popped2);

  // Queue is empty, try_pop should return error but size should remain 0
  TestData *empty = NULL;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_EMPTY, sc_generic_queue_try_pop(queue, (void **) &empty));
  TEST_ASSERT_NULL(empty);
  TEST_ASSERT_EQUAL(0, sc_generic_queue_get_size(queue));

  sc_generic_queue_nuke(queue);
}

void test_queue_nuke_with_cleanup_empty_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  cleanup_tracker_t tracker = {0};
  TestData *cleaned[10];
  tracker.cleaned_items  = cleaned;
  tracker.items_capacity = 10;

  // Destroy empty queue with cleanup callback
  sc_generic_queue_nuke_with_cleanup(queue, test_cleanup_callback, &tracker);

  // Should not call cleanup for empty queue
  TEST_ASSERT_EQUAL(0, tracker.cleanup_call_count);
}

void test_queue_nuke_with_cleanup_single_item(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  TestData *td = create_test_data(42, "cleanup test");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td));

  cleanup_tracker_t tracker = {0};
  TestData *cleaned[10];
  tracker.cleaned_items  = cleaned;
  tracker.items_capacity = 10;

  sc_generic_queue_nuke_with_cleanup(queue, test_cleanup_callback, &tracker);

  // Should call cleanup once
  TEST_ASSERT_EQUAL(1, tracker.cleanup_call_count);
  TEST_ASSERT_EQUAL(td, tracker.cleaned_items[0]);
}

void test_queue_nuke_with_cleanup_multiple_items(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Add multiple items
  TestData *td1 = create_test_data(1, "data1");
  TestData *td2 = create_test_data(2, "data2");
  TestData *td3 = create_test_data(3, "data3");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td3));

  cleanup_tracker_t tracker = {0};
  TestData *cleaned[10];
  tracker.cleaned_items  = cleaned;
  tracker.items_capacity = 10;

  sc_generic_queue_nuke_with_cleanup(queue, test_cleanup_callback, &tracker);

  // Should call cleanup for all 3 items
  TEST_ASSERT_EQUAL(3, tracker.cleanup_call_count);

  // Verify all items were cleaned (order may vary due to queue structure)
  int found_td1 = 0, found_td2 = 0, found_td3 = 0;
  for (int i = 0; i < 3; i++) {
    if (tracker.cleaned_items[i] == td1)
      found_td1 = 1;
    if (tracker.cleaned_items[i] == td2)
      found_td2 = 1;
    if (tracker.cleaned_items[i] == td3)
      found_td3 = 1;
  }
  TEST_ASSERT_EQUAL(1, found_td1);
  TEST_ASSERT_EQUAL(1, found_td2);
  TEST_ASSERT_EQUAL(1, found_td3);
}

void test_queue_nuke_with_cleanup_null_callback(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(3);
  TEST_ASSERT_NOT_NULL(queue);

  // Add items that won't be cleaned up
  TestData *td1 = create_test_data(1, "leaked1");
  TestData *td2 = create_test_data(2, "leaked2");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  // Destroy with NULL cleanup callback - items will be drained but not freed
  // This simulates the case where caller wants to handle cleanup elsewhere
  sc_generic_queue_nuke_with_cleanup(queue, NULL, NULL);

  // Note: In real usage this would leak memory, but for testing we just
  // verify the function handles NULL callback gracefully
  // In practice, the caller would be responsible for item cleanup

  // Clean up to avoid AddressSanitizer leak reports
  free_test_data(td1);
  free_test_data(td2);
}

void test_queue_nuke_with_cleanup_partial_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Add items, pop some, leave others
  TestData *td1 = create_test_data(1, "pop_me");
  TestData *td2 = create_test_data(2, "cleanup_me");
  TestData *td3 = create_test_data(3, "cleanup_me_too");

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td3));

  // Pop one item normally
  TestData *popped = NULL;
  sc_generic_queue_pop(queue, (void **) &popped);
  TEST_ASSERT_EQUAL(td1, popped);
  free_test_data(popped);

  // Destroy with remaining items
  cleanup_tracker_t tracker = {0};
  TestData *cleaned[10];
  tracker.cleaned_items  = cleaned;
  tracker.items_capacity = 10;

  sc_generic_queue_nuke_with_cleanup(queue, test_cleanup_callback, &tracker);

  // Should cleanup the 2 remaining items
  TEST_ASSERT_EQUAL(2, tracker.cleanup_call_count);
}

void test_queue_nuke_with_cleanup_thread_safety(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(10);
  TEST_ASSERT_NOT_NULL(queue);

  // Add items from multiple threads first
  pthread_t producers[3];
  thread_data_t producer_data[3];

  for (int i = 0; i < 3; i++) {
    producer_data[i].queue      = queue;
    producer_data[i].delay_ms   = 10;
    producer_data[i].test_value = i;
    pthread_create(&producers[i], NULL, producer_thread, &producer_data[i]);
  }

  // Wait for all producers to finish
  for (int i = 0; i < 3; i++) {
    pthread_join(producers[i], NULL);
  }

  // Now destroy with cleanup - should handle all items safely
  cleanup_tracker_t tracker = {0};
  TestData *cleaned[10];
  tracker.cleaned_items  = cleaned;
  tracker.items_capacity = 10;

  sc_generic_queue_nuke_with_cleanup(queue, test_cleanup_callback, &tracker);

  // Should have cleaned up exactly 3 items
  TEST_ASSERT_EQUAL(3, tracker.cleanup_call_count);
}

void test_queue_pop_timeout_on_empty_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  pthread_t thread;
  timeout_thread_data_t thread_data = {0};
  thread_data.queue                 = queue;

  // Start thread that will try to pop from empty queue
  pthread_create(&thread, NULL, timeout_pop_thread, &thread_data);

  // Wait for thread to complete (should timeout)
  pthread_join(thread, NULL);

  // Verify timeout occurred
  TEST_ASSERT_EQUAL(-1, thread_data.operation_result);
  TEST_ASSERT_NULL(thread_data.test_data);

  // Verify timing (should be around 2 seconds timeout)
  long long elapsed_ms          = thread_data.end_time - thread_data.start_time;
  long long expected_timeout_ms = 2 * 1000;

  // Allow some margin for timing variations
  TEST_ASSERT_GREATER_OR_EQUAL(expected_timeout_ms - TIMEOUT_MARGIN_MS, elapsed_ms);
  TEST_ASSERT_LESS_OR_EQUAL(expected_timeout_ms + TIMEOUT_MARGIN_MS, elapsed_ms);

  sc_generic_queue_nuke(queue);
}

void test_queue_add_timeout_on_full_queue(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2); // Small capacity
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue to capacity
  TestData *td1 = create_test_data(1, "fill1");
  TestData *td2 = create_test_data(2, "fill2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  // Try to add another item - should timeout
  pthread_t thread;
  timeout_thread_data_t thread_data = {0};
  thread_data.queue                 = queue;
  thread_data.test_data             = create_test_data(3, "timeout_data");

  pthread_create(&thread, NULL, timeout_add_thread, &thread_data);
  pthread_join(thread, NULL);

  // Verify timeout occurred by checking operation_result
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_TIMEOUT, thread_data.operation_result);

  // Verify timing (should timeout after 2 seconds)
  long long elapsed_ms          = thread_data.end_time - thread_data.start_time;
  long long expected_timeout_ms = 2 * 1000;

  TEST_ASSERT_GREATER_OR_EQUAL(expected_timeout_ms - TIMEOUT_MARGIN_MS, elapsed_ms);
  TEST_ASSERT_LESS_OR_EQUAL(expected_timeout_ms + TIMEOUT_MARGIN_MS, elapsed_ms);

  // Clean up - the timeout item may or may not have been added
  // depending on exact timing, so clean up safely
  TestData *remaining = NULL;
  while (sc_generic_queue_try_pop(queue, (void **) &remaining) == SC_GENERIC_QUEUE_SUCCESS) {
    free_test_data(remaining);
    remaining = NULL;
  }

  // Clean up timeout item if it wasn't added
  if (thread_data.test_data) {
    free_test_data(thread_data.test_data);
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_pop_succeeds_before_timeout(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  pthread_t pop_thread, add_thread;
  timeout_thread_data_t pop_data = {0};
  thread_data_t add_data         = {queue, 1000, 42}; // Add message after 1 second

  pop_data.queue = queue;

  // Start pop thread first (will wait)
  pthread_create(&pop_thread, NULL, timeout_pop_thread, &pop_data);

  // Start add thread with delay (should unblock pop before timeout)
  pthread_create(&add_thread, NULL, producer_thread, &add_data);

  pthread_join(pop_thread, NULL);
  pthread_join(add_thread, NULL);

  // Verify pop succeeded (didn't timeout)
  TEST_ASSERT_EQUAL(0, pop_data.operation_result);
  TEST_ASSERT_NOT_NULL(pop_data.test_data);

  // Verify timing was less than timeout
  long long elapsed_ms = pop_data.end_time - pop_data.start_time;
  long long timeout_ms = 2 * 1000;
  TEST_ASSERT_LESS_THAN(timeout_ms, elapsed_ms);

  // Clean up test data
  free_test_data(pop_data.test_data);

  sc_generic_queue_nuke(queue);
}

void test_queue_add_succeeds_before_timeout(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2);
  TEST_ASSERT_NOT_NULL(queue);

  // Fill queue
  TestData *td1 = create_test_data(1, "fill1");
  TestData *td2 = create_test_data(2, "fill2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  pthread_t add_thread, pop_thread;
  timeout_thread_data_t add_data = {0};
  thread_data_t pop_data         = {queue, 1000, 0}; // Pop after 1 second

  add_data.queue     = queue;
  add_data.test_data = create_test_data(99, "add_test");

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

  // Clean up remaining items
  TestData *remaining = NULL;
  while (sc_generic_queue_try_pop(queue, (void **) &remaining) == SC_GENERIC_QUEUE_SUCCESS) {
    free_test_data(remaining);
    remaining = NULL;
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_timeout_thread_safety(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(3);
  TEST_ASSERT_NOT_NULL(queue);

  // Test multiple threads timing out simultaneously
  pthread_t pop_threads[3];
  timeout_thread_data_t pop_data[3];

  for (int i = 0; i < 3; i++) {
    pop_data[i].queue            = queue;
    pop_data[i].test_data        = NULL;
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
    TEST_ASSERT_NULL(pop_data[i].test_data);
  }

  sc_generic_queue_nuke(queue);
}

void test_queue_timeout_error_conditions(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  // Test that timeout doesn't interfere with normal operations
  TestData *td = create_test_data(42, "normal_op");

  // Add should succeed immediately on empty queue
  long long start                   = get_time_ms();
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(queue, td);
  long long elapsed                 = get_time_ms() - start;
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);

  // Should complete very quickly, not wait for timeout
  TEST_ASSERT_LESS_THAN(1000, elapsed); // Less than 1 second

  // Pop should succeed immediately on non-empty queue
  start            = get_time_ms();
  TestData *popped = NULL;
  sc_generic_queue_pop(queue, (void **) &popped);
  elapsed = get_time_ms() - start;

  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_LESS_THAN(1000, elapsed); // Less than 1 second

  free_test_data(popped);
  sc_generic_queue_nuke(queue);
}

void test_queue_add_returns_error_on_null_queue(void) {
  TestData *td                      = create_test_data(42, "test");
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(NULL, td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  free_test_data(td);
}

void test_queue_add_returns_error_on_null_item(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  sc_generic_queue_ret_val_t result = sc_generic_queue_add(queue, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);

  sc_generic_queue_nuke(queue);
}

void test_queue_add_timeout_returns_error_code(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(2);
  TEST_ASSERT_NOT_NULL(queue);

  // Fill the queue
  TestData *td1 = create_test_data(1, "fill1");
  TestData *td2 = create_test_data(2, "fill2");
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td1));
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td2));

  // Try to add another - should timeout
  TestData *timeout_td              = create_test_data(3, "timeout");
  long long start                   = get_time_ms();
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(queue, timeout_td);
  long long elapsed                 = get_time_ms() - start;

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_TIMEOUT, result);
  TEST_ASSERT_GREATER_OR_EQUAL(2000 - TIMEOUT_MARGIN_MS, elapsed);

  // Clean up
  free_test_data(timeout_td);
  TestData *remaining = NULL;
  while (sc_generic_queue_try_pop(queue, (void **) &remaining) == SC_GENERIC_QUEUE_SUCCESS) {
    free_test_data(remaining);
    remaining = NULL;
  }
  sc_generic_queue_nuke(queue);
}

// Test errno functions
void test_queue_get_error_initial_state(void) {
  // Verify error starts at SC_GENERIC_QUEUE_SUCCESS
  sc_generic_queue_clear_error();
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());
}

void test_queue_get_error_after_null_parameter(void) {
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_add(NULL, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());
}

void test_queue_clear_error_resets_state(void) {
  // Set an error condition
  sc_generic_queue_add(NULL, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  // Clear error
  sc_generic_queue_clear_error();
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());
}

void test_queue_strerror_returns_correct_messages(void) {
  TEST_ASSERT_EQUAL_STRING("Success", sc_generic_queue_strerror(SC_GENERIC_QUEUE_SUCCESS));
  TEST_ASSERT_EQUAL_STRING("Null pointer parameter",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_NULL));
  TEST_ASSERT_EQUAL_STRING("Operation timed out",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_TIMEOUT));
  TEST_ASSERT_EQUAL_STRING("Thread operation failed",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_THREAD));
  TEST_ASSERT_EQUAL_STRING("Memory allocation failed",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_MEMORY));
  TEST_ASSERT_EQUAL_STRING("Queue is full", sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_FULL));
  TEST_ASSERT_EQUAL_STRING("Queue is empty", sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_EMPTY));
  TEST_ASSERT_EQUAL_STRING("Invalid parameter",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_INVALID));
  TEST_ASSERT_EQUAL_STRING("Integer overflow in capacity calculation",
                           sc_generic_queue_strerror(SC_GENERIC_QUEUE_ERR_OVERFLOW));
  TEST_ASSERT_EQUAL_STRING("Unknown error", sc_generic_queue_strerror(999));
}

// Test thread-local error handling
void *thread_local_error_test(void *arg) {
  (void) arg; // Unused parameter

  // Each thread should have its own error state
  sc_generic_queue_clear_error();
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

  // Trigger an error in this thread
  sc_generic_queue_add(NULL, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  return NULL;
}

void test_queue_errno_is_thread_local(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);

  // Main thread error state
  sc_generic_queue_clear_error();
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

  // Create thread that sets its own error
  pthread_t thread;
  pthread_create(&thread, NULL, thread_local_error_test, queue);
  pthread_join(thread, NULL);

  // Main thread error should still be SUCCESS
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

  sc_generic_queue_nuke(queue);
}

// New API tests
void test_queue_init_with_zero_capacity(void) {
  sc_generic_queue_clear_error();
  sc_generic_queue_t *queue = sc_generic_queue_init(0);
  TEST_ASSERT_NULL(queue);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_INVALID, sc_generic_queue_get_error());
}

void test_queue_nuke_returns_success(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TEST_ASSERT_NOT_NULL(queue);

  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_nuke(queue);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());
}

void test_queue_nuke_with_null_returns_error(void) {
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_nuke(NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());
}

void test_queue_pop_with_output_parameter(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TestData *td_in           = create_test_data(42, "test");

  sc_generic_queue_add(queue, td_in);

  TestData *td_out = NULL;
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_pop(queue, (void **) &td_out);

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(td_out);
  TEST_ASSERT_EQUAL_STRING("test", td_out->data);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

  free_test_data(td_out);
  sc_generic_queue_nuke(queue);
}

void test_queue_pop_with_null_parameters(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TestData *td              = NULL;

  // Test NULL queue
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_pop(NULL, (void **) &td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  // Test NULL output parameter
  sc_generic_queue_clear_error();
  result = sc_generic_queue_pop(queue, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  sc_generic_queue_nuke(queue);
}

void test_queue_pop_timeout_with_new_api(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TestData *td              = NULL;

  sc_generic_queue_clear_error();
  long long start                   = get_time_ms();
  sc_generic_queue_ret_val_t result = sc_generic_queue_pop(queue, (void **) &td);
  long long elapsed                 = get_time_ms() - start;

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_TIMEOUT, result);
  TEST_ASSERT_NULL(td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_TIMEOUT, sc_generic_queue_get_error());
  TEST_ASSERT_GREATER_OR_EQUAL(2000 - TIMEOUT_MARGIN_MS, elapsed);

  sc_generic_queue_nuke(queue);
}

void test_queue_try_pop_with_output_parameter(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TestData *td_in           = create_test_data(42, "test");
  sc_generic_queue_add(queue, td_in);

  TestData *td_out = NULL;
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_pop(queue, (void **) &td_out);

  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(td_out);
  TEST_ASSERT_EQUAL_STRING("test", td_out->data);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

  free_test_data(td_out);
  sc_generic_queue_nuke(queue);
}

void test_queue_try_add_null_parameters(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);

  // Test NULL queue
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_add(NULL, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  // Test NULL item
  sc_generic_queue_clear_error();
  result = sc_generic_queue_try_add(queue, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  sc_generic_queue_nuke(queue);
}

void test_queue_try_pop_null_parameters(void) {
  sc_generic_queue_t *queue = sc_generic_queue_init(5);
  TestData *td              = NULL;

  // Test NULL queue
  sc_generic_queue_clear_error();
  sc_generic_queue_ret_val_t result = sc_generic_queue_try_pop(NULL, (void **) &td);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  // Test NULL output parameter
  sc_generic_queue_clear_error();
  result = sc_generic_queue_try_pop(queue, NULL);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());

  sc_generic_queue_nuke(queue);
}

void test_queue_is_empty_with_null_queue(void) {
  sc_generic_queue_clear_error();
  bool result = sc_generic_queue_is_empty(NULL);

  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());
}

void test_queue_is_full_with_null_queue(void) {
  sc_generic_queue_clear_error();
  bool result = sc_generic_queue_is_full(NULL);

  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());
}

void test_queue_get_size_with_null_queue(void) {
  sc_generic_queue_clear_error();
  size_t size = sc_generic_queue_get_size(NULL);

  TEST_ASSERT_EQUAL(0, size);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_NULL, sc_generic_queue_get_error());
}

// Test overflow detection in queue_create
void test_queue_init_with_overflow_capacity(void) {
  sc_generic_queue_clear_error();

  // Try to create queue with capacity that would overflow
  size_t huge_capacity      = SIZE_MAX / sizeof(void *) + 1;
  sc_generic_queue_t *queue = sc_generic_queue_init(huge_capacity);

  TEST_ASSERT_NULL(queue);
  TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_OVERFLOW, sc_generic_queue_get_error());
}

void test_queue_init_with_max_capacity(void) {
  sc_generic_queue_clear_error();

  // Try to create queue with exactly max allowed capacity
  size_t max_capacity       = SIZE_MAX / sizeof(void *) / 2;
  sc_generic_queue_t *queue = sc_generic_queue_init(max_capacity);

  // This should fail - either due to safety limit or memory allocation
  TEST_ASSERT_NULL(queue);
  int error = sc_generic_queue_get_error();
  // Could be SC_GENERIC_QUEUE_ERR_OVERFLOW if caught by limit check, or SC_GENERIC_QUEUE_ERR_MEMORY
  // if allocation fails
  TEST_ASSERT_TRUE(error == SC_GENERIC_QUEUE_ERR_OVERFLOW || error == SC_GENERIC_QUEUE_ERR_MEMORY);
}

void test_queue_init_with_safe_large_capacity(void) {
  sc_generic_queue_clear_error();

  // Create queue with large but safe capacity
  size_t large_capacity     = 1000000; // 1 million
  sc_generic_queue_t *queue = sc_generic_queue_init(large_capacity);

  // This should succeed if we have enough memory
  if (queue != NULL) {
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_get_error());

    // Verify basic operations work
    TestData *td = create_test_data(42, "test");
    TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_add(queue, td));

    TestData *popped = NULL;
    TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_SUCCESS, sc_generic_queue_pop(queue, (void **) &popped));
    TEST_ASSERT_NOT_NULL(popped);

    free_test_data(popped);
    sc_generic_queue_nuke(queue);
  } else {
    // If allocation failed, it should be due to memory limits, not overflow
    TEST_ASSERT_EQUAL(SC_GENERIC_QUEUE_ERR_MEMORY, sc_generic_queue_get_error());
  }
}

void test_queue_init_memory_allocation_failure(void) {
  sc_generic_queue_clear_error();

  // Try to create queue with very large capacity that might fail allocation
  // but not due to overflow
  size_t huge_capacity      = SIZE_MAX / sizeof(void *) / 4;
  sc_generic_queue_t *queue = sc_generic_queue_init(huge_capacity);

  if (queue == NULL) {
    // Should fail with either overflow (due to safety limit) or memory error
    int error = sc_generic_queue_get_error();
    TEST_ASSERT_TRUE(error == SC_GENERIC_QUEUE_ERR_OVERFLOW ||
                     error == SC_GENERIC_QUEUE_ERR_MEMORY);
  } else {
    // If it succeeded, clean up
    sc_generic_queue_nuke(queue);
  }
}

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
  UnityBegin("tests/generic_queue_tests.c");
  RUN_TEST(test_queue_add_and_pop_item);
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
  RUN_TEST(test_queue_nuke_with_cleanup_empty_queue);
  RUN_TEST(test_queue_nuke_with_cleanup_single_item);
  RUN_TEST(test_queue_nuke_with_cleanup_multiple_items);
  RUN_TEST(test_queue_nuke_with_cleanup_null_callback);
  RUN_TEST(test_queue_nuke_with_cleanup_partial_queue);
  RUN_TEST(test_queue_nuke_with_cleanup_thread_safety);
  RUN_TEST(test_queue_pop_timeout_on_empty_queue);
  RUN_TEST(test_queue_add_timeout_on_full_queue);
  RUN_TEST(test_queue_pop_succeeds_before_timeout);
  RUN_TEST(test_queue_add_succeeds_before_timeout);
  RUN_TEST(test_queue_timeout_thread_safety);
  RUN_TEST(test_queue_timeout_error_conditions);
  RUN_TEST(test_queue_add_returns_error_on_null_queue);
  RUN_TEST(test_queue_add_returns_error_on_null_item);
  RUN_TEST(test_queue_add_timeout_returns_error_code);

  // New errno-style error handling tests
  RUN_TEST(test_queue_get_error_initial_state);
  RUN_TEST(test_queue_get_error_after_null_parameter);
  RUN_TEST(test_queue_clear_error_resets_state);
  RUN_TEST(test_queue_strerror_returns_correct_messages);
  RUN_TEST(test_queue_errno_is_thread_local);

  // New API tests
  RUN_TEST(test_queue_init_with_zero_capacity);
  RUN_TEST(test_queue_nuke_returns_success);
  RUN_TEST(test_queue_nuke_with_null_returns_error);
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
  RUN_TEST(test_queue_init_with_overflow_capacity);
  RUN_TEST(test_queue_init_with_max_capacity);
  RUN_TEST(test_queue_init_with_safe_large_capacity);
  RUN_TEST(test_queue_init_memory_allocation_failure);

  return (UnityEnd());
}
