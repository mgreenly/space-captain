#include "vendor/unity.c"

void setUp(void) {
    // Empty setup
}

void tearDown(void) {
    // Empty teardown
}

void test_dummy_always_passes(void) {
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dummy_always_passes);
    return UNITY_END();
}