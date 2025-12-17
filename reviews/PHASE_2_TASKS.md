# Phase 2 Sub-Tasks

This document breaks down the Phase 2 improvements from `reviews/001_REVIEW.md` into individual sub-issues for tracking under Issue #13.

**Parent Issue:** #13 - Phase 2  
**Estimated Total Time:** 2 hours  
**Status:** Ready for assignment

---

## Sub-Task 5: Define constants for magic numbers

**Priority:** High  
**Estimated Time:** 15 minutes  
**Files to modify:** `src/config.h`

### Description
Replace magic numbers throughout the codebase with named constants to improve code readability and maintainability.

### Required Constants
```cpp
const uint8_t NUM_LOOPS = 4;
const uint8_t NUM_BANKS = 32;
const uint8_t PRESETS_PER_BANK = 4;
const uint8_t TOTAL_PRESETS = 128;  // NUM_BANKS * PRESETS_PER_BANK
```

### Files Affected
- `src/config.h` - Add constant definitions
- `src/state_manager.cpp` - Replace hardcoded values
- `src/mode_controller.cpp` - Replace hardcoded values

### Acceptance Criteria
- [ ] All magic numbers replaced with named constants
- [ ] Code compiles without errors
- [ ] Constants defined in a logical location (`config.h`)
- [ ] No behavioral changes to the system

---

## Sub-Task 6: Add debug output flag

**Priority:** Medium  
**Estimated Time:** 20 minutes  
**Files to modify:** `src/config.h`, various source files

### Description
Implement a compile-time debug flag that enables debug output via Serial without affecting MIDI functionality.

### Implementation
```cpp
// In config.h:
#ifdef DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
```

### Considerations
- Debug output should not interfere with MIDI TX on Serial
- Should be compile-time flag (no runtime overhead when disabled)
- Consider using a separate software serial for debug if needed

### Acceptance Criteria
- [ ] DEBUG_MODE flag added to config.h
- [ ] Debug macros defined
- [ ] Key state transitions logged (mode changes, preset loads, etc.)
- [ ] Code compiles with and without DEBUG_MODE enabled
- [ ] MIDI output unaffected when debug is disabled

---

## Sub-Task 7: Implement display buffering

**Priority:** Medium  
**Estimated Time:** 30 minutes  
**Files to modify:** `src/display.cpp`, `src/display.h`

### Description
Add display buffering to reduce flicker and unnecessary SPI transactions to the MAX7219 display driver.

### Implementation Strategy
1. Add a buffer to track current display state
2. Compare new display content with buffer before sending
3. Only send SPI commands when display content actually changes
4. Maintain separate buffers for digits and decimal points

### Files Affected
- `src/display.h` - Add buffer state variables
- `src/display.cpp` - Implement buffering logic in update methods

### Acceptance Criteria
- [ ] Display buffer implemented
- [ ] Display only updates when content changes
- [ ] No visible flicker during normal operation
- [ ] All display modes work correctly (bank, PC, edit, saved)
- [ ] Memory usage remains within acceptable limits

---

## Sub-Task 8: Fix mode transition memory

**Priority:** Low  
**Estimated Time:** 25 minutes  
**Files to modify:** `src/state_manager.cpp`, `src/state_manager.h`

### Description
Remember the previous mode when exiting channel set mode, so the system returns to the correct mode instead of defaulting.

### Current Behavior
When exiting channel set mode, the system may not return to the previous mode (Manual or Bank).

### Desired Behavior
- Track previous mode before entering channel set mode
- Restore previous mode when exiting channel set mode
- Handle edge cases (power-up, reset, etc.)

### Implementation
Add a `previousMode` variable to StateManager that stores the mode before transitioning.

### Files Affected
- `src/state_manager.h` - Add previousMode member variable
- `src/state_manager.cpp` - Update mode transition logic

### Acceptance Criteria
- [ ] Previous mode correctly saved before entering channel set mode
- [ ] Previous mode correctly restored when exiting channel set mode
- [ ] Edge cases handled (power-up defaults to MANUAL_MODE)
- [ ] No impact on other mode transitions

---

## Sub-Task 9: Add loop timing control

**Priority:** Medium  
**Estimated Time:** 30 minutes  
**Files to modify:** `src/main.cpp`

### Description
Add fixed timing control to the main loop to reduce power consumption and improve efficiency.

### Current Behavior
Main loop runs as fast as possible (approximately 1-10 KHz depending on operations).

### Desired Behavior
- Main loop runs at fixed ~100Hz update rate
- Reduces unnecessary CPU cycles
- Improves power efficiency
- Maintains responsiveness for human input

### Implementation
```cpp
void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdate < 10) return;  // 100Hz update rate
    lastUpdate = currentTime;
    
    // ... rest of loop code
}
```

### Files Affected
- `src/main.cpp` - Modify main loop

### Acceptance Criteria
- [ ] Main loop runs at approximately 100Hz
- [ ] All functionality remains responsive
- [ ] Switch debouncing still works correctly
- [ ] MIDI timing unaffected
- [ ] Display updates remain smooth

---

## Implementation Order

Recommended order to minimize merge conflicts and dependencies:

1. **Sub-Task 5** - Define constants (foundation for others)
2. **Sub-Task 9** - Add loop timing control (simple, independent)
3. **Sub-Task 6** - Add debug output flag (helps with testing)
4. **Sub-Task 7** - Implement display buffering
5. **Sub-Task 8** - Fix mode transition memory

---

## Testing Checklist

After completing all Phase 2 tasks:

- [ ] Code compiles without errors or warnings
- [ ] All modes work correctly (Manual, Bank, Edit)
- [ ] Switch combinations work as documented
- [ ] MIDI output functions correctly
- [ ] Display shows correct information in all modes
- [ ] No regression in existing functionality
- [ ] Memory usage within acceptable limits (< 80% SRAM)
- [ ] Power consumption reduced (if measurable)

---

**Note:** These sub-tasks are derived from the Phase 2 section of `reviews/001_REVIEW.md`. Each should be created as a separate GitHub issue linked to parent issue #13.
