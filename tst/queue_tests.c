#include "vendor/unity.c"

#include "../src/queue.h"
#include "../src/queue.c"

void test_queue_foo(void) {
  TEST_ASSERT_EQUAL(0, 0);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  puts("==============================================");
  UnityBegin("tst/queue_tests.c");
  RUN_TEST(test_queue_foo);
  return (UnityEnd());
}
