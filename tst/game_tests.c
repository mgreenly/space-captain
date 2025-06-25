#include "vendor/unity.c"

#include "../src/state.h"
#include "../src/state.c"

#include "../src/game.h"
#include "../src/game.c"

void test_game_foo(void)
{
  TEST_ASSERT_EQUAL(0, 0);
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
  UnityBegin("tst/game_tests.c");
  RUN_TEST(test_game_foo);
  return (UnityEnd());
}
