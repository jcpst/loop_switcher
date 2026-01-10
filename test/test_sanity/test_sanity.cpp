#ifdef UNIT_TEST

#include <unity.h>

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

// Basic sanity test
void test_sanity_check(void) {
    TEST_ASSERT_TRUE(true);
    TEST_ASSERT_FALSE(false);
    TEST_ASSERT_EQUAL(1, 1);
}

// Basic arithmetic test
void test_basic_math(void) {
    TEST_ASSERT_EQUAL(4, 2 + 2);
    TEST_ASSERT_EQUAL(10, 5 * 2);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_sanity_check);
    RUN_TEST(test_basic_math);

    return UNITY_END();
}

#endif // UNIT_TEST
