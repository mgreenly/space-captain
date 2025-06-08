#include "vendor/unity.c"

#include "config_tests.c"
#include "state_tests.c"
#include "network_tests.c"
#include "game_tests.c"

void setUp(void) {
}

void tearDown(void) {
}


int main(void)
{
  UnityBegin("tst/config_tests.c");
  RUN_TEST(test_config_foo, 20);
  RUN_TEST(test_state_foo, 20);
  RUN_TEST(test_network_foo, 20);
  RUN_TEST(test_game_foo, 20);
  return (UnityEnd());
}
