#include "vendor/unity.c"

#include "../src/state.h"
#include "../src/state.c"

void test_state_foo(void) {
  TEST_ASSERT_EQUAL(0, 0);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  UnityBegin("tst/state_tests.c");
  RUN_TEST(test_state_foo, 20);
  return (UnityEnd());
}
