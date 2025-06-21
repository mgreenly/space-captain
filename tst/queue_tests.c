#include <string.h>

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

  queue_push(queue, &msg);

  message_t *popped_msg = queue_pop(queue);
  TEST_ASSERT_NOT_NULL(popped_msg);
  TEST_ASSERT_EQUAL(MSG_ECHO, popped_msg->header.type);
  TEST_ASSERT_EQUAL_STRING(test_message, popped_msg->body);

  queue_destroy(queue);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  puts("==============================================");
  UnityBegin("tst/queue_tests.c");
  RUN_TEST(test_queue_add_and_pop_message);
  return (UnityEnd());
}
