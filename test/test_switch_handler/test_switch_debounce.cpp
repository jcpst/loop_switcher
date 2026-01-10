#ifdef UNIT_TEST

#include <unity.h>
#include "switch_handler.h"

// Test helper to create raw states array
void setRawStates(bool states[4], bool s0, bool s1, bool s2, bool s3) {
    states[0] = s0;
    states[1] = s1;
    states[2] = s2;
    states[3] = s3;
}

// Test: Initial state should be HIGH (not pressed) for all switches
void test_initial_state_all_unpressed(void) {
    SwitchHandler handler;

    TEST_ASSERT_FALSE(handler.isPressed(0));
    TEST_ASSERT_FALSE(handler.isPressed(1));
    TEST_ASSERT_FALSE(handler.isPressed(2));
    TEST_ASSERT_FALSE(handler.isPressed(3));
}

// Test: Switch press should be detected after debounce time
void test_switch_press_after_debounce(void) {
    SwitchHandler handler(30, 400, 1000);  // 30ms debounce
    bool rawStates[4];

    // Initially all switches HIGH (not pressed)
    setRawStates(rawStates, true, true, true, true);
    handler.update(rawStates, 0);

    TEST_ASSERT_FALSE(handler.isPressed(0));

    // Press switch 0 (goes LOW)
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, 10);  // 10ms later

    // Should not be registered yet (within debounce time)
    TEST_ASSERT_FALSE(handler.isPressed(0));

    // Keep it pressed, advance past debounce time
    handler.update(rawStates, 50);  // 50ms total (> 30ms debounce)

    // Now it should be registered
    TEST_ASSERT_TRUE(handler.isPressed(0));
    TEST_ASSERT_FALSE(handler.isPressed(1));
}

// Test: Bouncing signal should not cause false triggering
void test_debounce_prevents_false_triggers(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];
    unsigned long time = 0;

    // Initial state
    setRawStates(rawStates, true, true, true, true);
    handler.update(rawStates, time);

    // Start pressing (bouncing between HIGH and LOW)
    time = 5;
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, time);

    time = 10;
    setRawStates(rawStates, true, true, true, true);  // Bounce back
    handler.update(rawStates, time);

    time = 15;
    setRawStates(rawStates, false, true, true, true);  // Bounce again
    handler.update(rawStates, time);

    time = 20;
    setRawStates(rawStates, true, true, true, true);  // Bounce back
    handler.update(rawStates, time);

    time = 25;
    setRawStates(rawStates, false, true, true, true);  // Finally stable
    handler.update(rawStates, time);

    // Should not be pressed yet
    TEST_ASSERT_FALSE(handler.isPressed(0));

    // Wait past debounce time with stable state
    time = 60;
    handler.update(rawStates, time);

    // Now it should be registered
    TEST_ASSERT_TRUE(handler.isPressed(0));
}

// Test: Multiple switches can be pressed independently
void test_multiple_switches_independent(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];

    // Initial state
    setRawStates(rawStates, true, true, true, true);
    handler.update(rawStates, 0);

    // Press switches 0 and 2
    setRawStates(rawStates, false, true, false, true);
    handler.update(rawStates, 0);
    handler.update(rawStates, 50);  // Past debounce

    TEST_ASSERT_TRUE(handler.isPressed(0));
    TEST_ASSERT_FALSE(handler.isPressed(1));
    TEST_ASSERT_TRUE(handler.isPressed(2));
    TEST_ASSERT_FALSE(handler.isPressed(3));
}

// Test: Switch release should be detected
void test_switch_release(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];

    // Press switch 0
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, 0);
    handler.update(rawStates, 50);  // Past debounce

    TEST_ASSERT_TRUE(handler.isPressed(0));

    // Release switch 0
    setRawStates(rawStates, true, true, true, true);
    handler.update(rawStates, 100);
    handler.update(rawStates, 150);  // Past debounce

    TEST_ASSERT_FALSE(handler.isPressed(0));
}

// Test: Reset should clear all state
void test_reset_clears_state(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];

    // Press all switches
    setRawStates(rawStates, false, false, false, false);
    handler.update(rawStates, 0);
    handler.update(rawStates, 50);

    TEST_ASSERT_TRUE(handler.isPressed(0));
    TEST_ASSERT_TRUE(handler.isPressed(1));

    // Reset
    handler.reset();

    // All should be unpressed
    TEST_ASSERT_FALSE(handler.isPressed(0));
    TEST_ASSERT_FALSE(handler.isPressed(1));
    TEST_ASSERT_FALSE(handler.isPressed(2));
    TEST_ASSERT_FALSE(handler.isPressed(3));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_all_unpressed);
    RUN_TEST(test_switch_press_after_debounce);
    RUN_TEST(test_debounce_prevents_false_triggers);
    RUN_TEST(test_multiple_switches_independent);
    RUN_TEST(test_switch_release);
    RUN_TEST(test_reset_clears_state);

    return UNITY_END();
}

#endif // UNIT_TEST
