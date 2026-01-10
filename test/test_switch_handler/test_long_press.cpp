#ifdef UNIT_TEST

#include <unity.h>
#include "switch_handler.h"

// Unity setup/teardown
void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

// Test helper
static void setRawStates(bool states[4], bool s0, bool s1, bool s2, bool s3) {
    states[0] = s0;
    states[1] = s1;
    states[2] = s2;
    states[3] = s3;
}

// Test: Long press should trigger after threshold time
void test_long_press_triggers_after_threshold(void) {
    SwitchHandler handler(30, 400, 1000);  // 1000ms long press
    bool rawStates[4];
    unsigned long time = 0;

    // Press switches 0 and 1 simultaneously
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);  // Past debounce

    // Should not trigger immediately
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    // Hold for just under threshold
    time = 1049;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    // Hold past threshold
    time = 1100;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));
}

// Test: Long press should not trigger twice for same press
void test_long_press_triggers_only_once(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];
    unsigned long time = 0;

    // Press and hold switches 0 and 1
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    // Trigger long press
    time = 1100;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));

    // Try again - should not trigger
    time = 1200;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    time = 2000;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));
}

// Test: Long press resets after releasing and pressing again
void test_long_press_resets_after_release(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];
    unsigned long time = 0;

    // First press and hold
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    time = 1100;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));

    // Release switches
    time = 1200;
    setRawStates(rawStates, true, true, true, true);
    handler.update(rawStates, time);
    time = 1250;
    handler.update(rawStates, time);

    // Press again
    time = 1300;
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 1350;
    handler.update(rawStates, time);

    // Should not trigger yet (need another 1000ms)
    time = 1400;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    // Should trigger after new threshold
    time = 2400;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));
}

// Test: Custom long press duration
void test_custom_long_press_duration(void) {
    SwitchHandler handler(30, 400, 1000);  // Default 1000ms
    bool rawStates[4];
    unsigned long time = 0;

    // Press switches
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    // Use custom duration of 2000ms
    time = 1500;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time, 2000));

    time = 2100;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time, 2000));
}

// Test: Long press requires both switches pressed
void test_long_press_requires_both_switches(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];
    unsigned long time = 0;

    // Press only switch 0
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    time = 1100;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    // Now press both
    setRawStates(rawStates, false, false, true, true);
    time = 1150;
    handler.update(rawStates, time);
    time = 1200;
    handler.update(rawStates, time);

    // Should trigger after holding both
    time = 2300;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));
}

// Test: Long press works with non-simultaneous press (uses later press time)
void test_long_press_uses_later_press_time(void) {
    SwitchHandler handler(30, 400, 1000);
    bool rawStates[4];
    unsigned long time = 0;

    // Press switch 0 first
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    // Press switch 1 later (300ms after switch 0)
    time = 350;
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 400;
    handler.update(rawStates, time);

    // 1000ms after first press (only 650ms after second press)
    time = 1100;
    TEST_ASSERT_FALSE(handler.isLongPress(0, 1, time));

    // 1000ms after second press
    time = 1450;
    TEST_ASSERT_TRUE(handler.isLongPress(0, 1, time));
}

// Test: Recent press detection for simultaneous press patterns
void test_recent_press_detection(void) {
    SwitchHandler handler(30, 400, 1000);  // 400ms simultaneous window
    bool rawStates[4];
    unsigned long time = 0;

    // Press switch 0
    setRawStates(rawStates, false, true, true, true);
    handler.update(rawStates, time);
    time = 50;
    handler.update(rawStates, time);

    // Should be recent
    TEST_ASSERT_TRUE(handler.isRecentPress(0, time));

    // Press switch 1 within window
    time = 200;
    setRawStates(rawStates, false, false, true, true);
    handler.update(rawStates, time);
    time = 250;
    handler.update(rawStates, time);

    // Both should be recent
    TEST_ASSERT_TRUE(handler.isRecentPress(0, time));
    TEST_ASSERT_TRUE(handler.isRecentPress(1, time));

    // Past the window for switch 0
    time = 500;
    TEST_ASSERT_FALSE(handler.isRecentPress(0, time));
    TEST_ASSERT_TRUE(handler.isRecentPress(1, time));

    // Clear recent presses
    handler.clearRecentPresses();
    TEST_ASSERT_FALSE(handler.isRecentPress(0, time));
    TEST_ASSERT_FALSE(handler.isRecentPress(1, time));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_long_press_triggers_after_threshold);
    RUN_TEST(test_long_press_triggers_only_once);
    RUN_TEST(test_long_press_resets_after_release);
    RUN_TEST(test_custom_long_press_duration);
    RUN_TEST(test_long_press_requires_both_switches);
    RUN_TEST(test_long_press_uses_later_press_time);
    RUN_TEST(test_recent_press_detection);

    return UNITY_END();
}

#endif // UNIT_TEST
