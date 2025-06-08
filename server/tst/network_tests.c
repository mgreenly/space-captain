#include "vendor/unity.c"

#include "../src/network.h"
#include "../src/network.c"

void test_network_foo(void) {
  TEST_ASSERT_EQUAL(0, 0);
}

void setUp(void) { }

void tearDown(void) { }

int main(void)
{
  puts("==============================================");
  UnityBegin("tst/network_tests.c");
  RUN_TEST(test_network_foo);
  return (UnityEnd());
}
