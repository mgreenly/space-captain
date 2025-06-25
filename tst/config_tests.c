#include "vendor/unity.c"

#include "../src/config.h"
#include "../src/config.c"

void test_config_foo(void) {
  config *cfg = NULL;

  config_result retval = config_load(&cfg);

  TEST_ASSERT_EQUAL(CONFIG_SUCCESS, retval);
  TEST_ASSERT_NOT_NULL(cfg);

  config_free(&cfg);
}

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
  UnityBegin("tst/config_tests.c");
  RUN_TEST(test_config_foo);
  return (UnityEnd());
}
