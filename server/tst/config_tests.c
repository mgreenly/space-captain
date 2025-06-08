#include "vendor/unity.c"

#include "../src/config.h"
#include "../src/config.c"

void test_config_foo(void) {
  TEST_ASSERT_EQUAL(0, 0);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  UnityBegin("tst/config_tests.c");
  RUN_TEST(test_config_foo, 20);
  return (UnityEnd());
}
