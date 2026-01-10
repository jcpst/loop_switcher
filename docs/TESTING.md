# Unit Testing Guide

This project includes unit tests that run on your local machine without requiring connected hardware.

## Overview

The test suite uses:
- **PlatformIO's native platform** - runs tests on your development machine
- **Unity test framework** - lightweight C/C++ unit testing
- **Arduino mocks** - simulated Arduino functions for testing

## Running Tests

### Run all tests
```bash
pio test -e native
```

### Run specific test suite
```bash
pio test -e native -f test_switch_debounce
pio test -e native -f test_long_press
```

### Verbose output
```bash
pio test -e native -v
```

## Test Structure

```
test/
├── mocks/
│   ├── Arduino.h          # Mock Arduino functions
│   └── Arduino.cpp        # Mock implementation
└── test_switch_handler/
    ├── test_switch_debounce.cpp   # Debouncing tests
    └── test_long_press.cpp        # Long press & pattern tests
```

## What's Tested

### Switch Debouncing (`test_switch_debounce.cpp`)
- Initial state detection
- Press detection after debounce delay
- Bounce filtering
- Multiple independent switches
- Switch release detection
- State reset

### Long Press Detection (`test_long_press.cpp`)
- Long press threshold timing
- Single-trigger per press
- Reset after release
- Custom duration support
- Two-switch requirement
- Sequential press handling
- Recent press detection (for simultaneous patterns)

## Testable Modules

### SwitchHandler (`src/switch_handler.h/cpp`)
A hardware-independent module for switch debouncing and pattern detection:
- **`update(rawStates[], time)`** - Process raw switch readings
- **`isPressed(index)`** - Check if switch is currently pressed
- **`isLongPress(sw1, sw2, time)`** - Detect two-switch long press
- **`isRecentPress(index, time)`** - Check for recent press (simultaneous detection)
- **`clearRecentPresses()`** - Clear recent press timestamps
- **`reset()`** - Reset all state

This module is ready to use in Steps 3+ of the rewrite plan (see `docs/REWRITE_PLAN.md`).

## Arduino Mocks

The test environment includes mocks for common Arduino functions:
- `pinMode()`, `digitalWrite()`, `digitalRead()`
- `millis()`, `delay()`

Mock control functions (for tests only):
- `ArduinoMock::setMillis(ms)` - Set mock time
- `ArduinoMock::setPinState(pin, state)` - Simulate pin input
- `ArduinoMock::getPinState(pin)` - Read mock pin state
- `ArduinoMock::reset()` - Reset all mock state

## Adding New Tests

1. Create a new test file in `test/test_<module_name>/`
2. Include Unity and your module:
   ```cpp
   #ifdef UNIT_TEST
   #include <unity.h>
   #include "../../src/your_module.h"
   ```

3. Write test functions:
   ```cpp
   void test_something(void) {
       // Arrange
       // Act
       // Assert
       TEST_ASSERT_TRUE(condition);
   }
   ```

4. Add tests to main:
   ```cpp
   int main(int argc, char **argv) {
       UNITY_BEGIN();
       RUN_TEST(test_something);
       return UNITY_END();
   }
   #endif
   ```

## Integration with Development Workflow

Tests are designed to complement hardware validation:
1. **Write tests first** - Define expected behavior
2. **Implement logic** - Create hardware-independent modules
3. **Run unit tests** - Verify logic without hardware
4. **Hardware test** - Validate on actual device (per REWRITE_PLAN.md)

## CI Integration

To run tests in CI (GitHub Actions, etc.):

```yaml
- name: Run Tests
  run: pio test -e native
```

See `.github/workflows/` for complete examples.

## Troubleshooting

### "undefined reference to Arduino functions"
Make sure your test file has:
```cpp
#ifdef UNIT_TEST
// ... test code ...
#endif
```

### Tests not found
Ensure test files are in `test/test_<name>/` directories and named `test_*.cpp`.

### Mock time not advancing
Remember to call `ArduinoMock::setMillis()` or use the `delay()` mock in tests.

## Future Test Coverage

As the firmware progresses through the rewrite steps (see `docs/REWRITE_PLAN.md`), additional test modules can be added:

- **Step 5**: MIDI message generation
- **Step 6**: Bank mode state machine
- **Step 7**: EEPROM preset logic
- **Step 8**: Individual modules (display, relays, etc.)

Each module should be designed with testability in mind - separating business logic from hardware I/O.
