# Code Review: 4-Loop MIDI Switcher

**Review Date:** December 16, 2025  
**Repository:** jcpst/loop_switcher  
**Branch:** main

## Executive Summary

Arduino project for a guitar effects loop switcher with MIDI control. The code uses modular organization with separation of concerns and embedded systems patterns. Several areas require improvement in robustness, optimization, and best practices.

**Assessment:** Production ready with recommended improvements

---

## Architecture Overview

### Strengths
**Modular Design**: Separation of concerns with dedicated modules:
- `StateManager` - Central state management
- `ModeController` - Mode logic and state transitions
- `SwitchHandler` - Input handling with debouncing
- `Display`, `RelayController`, `LedController` - Hardware abstraction
- `MidiHandler` - MIDI communication

**File Organization**: Headers and implementations separated

**Hardware Abstraction**: Hardware components abstracted for testability

### Areas for Improvement
⚠️ **Tight Coupling**: Some circular dependencies exist (e.g., `ModeController` includes `midi_handler.h` directly, `StateManager` includes `display.h`)

⚠️ **Global State**: The `StateManager` acts as a global state object passed around - consider making methods more self-contained

---

## Detailed Code Review by Module

### 1. Configuration (`config.h`)

**Strengths:**
- Documented pin assignments
- Constants separated
- Uses `const` for compile-time constants

**Issues & Recommendations:**

🔴 **CRITICAL - Magic Numbers:**
```cpp
const uint8_t EEPROM_INIT_MAGIC = 0x42;
```
**Issue:** While documented, 0x42 could conflict with valid EEPROM data  
**Recommendation:** Use a more unique multi-byte signature or document the chosen value's rationale more clearly

🟡 **MODERATE - Hard-coded Array Sizes:**
```cpp
// Arrays are sized to 4 throughout the code
```
**Issue:** No constant like `NUM_LOOPS = 4` defined  
**Recommendation:** 
```cpp
const uint8_t NUM_LOOPS = 4;
const uint8_t NUM_SWITCHES = 4;
const uint8_t NUM_BANKS = 32;
const uint8_t PRESETS_PER_BANK = 4;
const uint8_t TOTAL_PRESETS = 128;
```

🟡 **MODERATE - LED Polarity Configuration:**
```cpp
const bool LED_ACTIVE_LOW = false;
```
**Issue:** Users must modify source code to change this  
**Recommendation:** Consider storing in EEPROM or using a jumper/switch to detect at runtime

---

### 2. State Manager (`state_manager.cpp/h`)

**Strengths:**
- Encapsulated state
- EEPROM initialization check
- Preset save/load implementation

**Issues & Recommendations:**

🔴 **CRITICAL - EEPROM Wear:**
```cpp
void StateManager::savePreset(uint8_t presetNumber) {
    // Writes directly to EEPROM every time
    EEPROM.write(EEPROM_PRESETS_START_ADDR + presetNumber - 1, packedState);
}
```
**Issue:** EEPROM has limited write cycles (~100,000). Saving in edit mode writes immediately  
**Recommendation:** 
```cpp
// Add wear leveling or dirty flag check:
void StateManager::savePreset(uint8_t presetNumber) {
    if (presetNumber < 1 || presetNumber > 128) return;
    
    uint8_t packedState = 0;
    for (int i = 0; i < 4; i++) {
        if (loopStates[i]) {
            packedState |= (1 << i);
        }
    }
    
    // Only write if changed
    uint8_t currentValue = EEPROM.read(EEPROM_PRESETS_START_ADDR + presetNumber - 1);
    if (currentValue != packedState) {
        EEPROM.write(EEPROM_PRESETS_START_ADDR + presetNumber - 1, packedState);
    }
}
```

🟡 **MODERATE - Bounds Checking:**
```cpp
if (storedChannel >= 1 && storedChannel <= 16) {
```
**Issue:** Validation exists, but could be more defensive  
**Recommendation:** Add similar validation for all EEPROM reads and log errors to Serial in debug builds

🟡 **MODERATE - Memory Safety:**
```cpp
bool* StateManager::getDisplayLoops() {
    return (currentMode == EDIT_MODE) ? editModeLoopStates : loopStates;
}
```
**Issue:** Returns raw pointer to internal state - can be modified externally  
**Recommendation:** Return `const bool*` or copy the array

---

### 3. Mode Controller (`mode_controller.cpp/h`)

**Strengths:**
- Organized switch pattern detection
- Clear state machine logic
- Handles simultaneous presses

**Issues & Recommendations:**

🔴 **CRITICAL - Double Press Logic:**
```cpp
if (state.activePreset == switchIndex && !state.globalPresetActive) {
    // Activate global preset
    state.globalPresetActive = true;
    sendMIDIProgramChange(128, state.midiChannel);
    // ...
}
```
**Issue:** No debouncing for double-press detection - relies on "recent press" window which may not detect intentional quick double presses  
**Recommendation:** Add explicit double-press detection with timing window:
```cpp
// Add to SwitchState:
unsigned long lastReleaseTime;

// In ModeController:
bool isDoubleTap(uint8_t switchIndex, uint16_t windowMs = 300) {
    unsigned long now = millis();
    bool result = (now - switches[switchIndex].lastReleaseTime) < windowMs;
    switches[switchIndex].lastReleaseTime = now;
    return result;
}
```

🟡 **MODERATE - Timeout Safety:**
```cpp
if ((now - state.channelModeStartTime) > CHANNEL_TIMEOUT_MS) {
    exitChannelSetMode();
}
```
**Issue:** Uses `unsigned long` subtraction which can overflow if `now` wraps around (every 49.7 days)  
**Recommendation:** While Arduino's millis() handles this correctly, add a comment explaining overflow safety or use safe comparison:
```cpp
// Safe for millis() overflow - subtraction wraps correctly
if ((now - state.channelModeStartTime) > CHANNEL_TIMEOUT_MS) {
```

🟡 **MODERATE - Mode Transitions:**
```cpp
void ModeController::exitChannelSetMode() {
    EEPROM.write(EEPROM_CHANNEL_ADDR, state.midiChannel);
    state.currentMode = BANK_MODE;  // Always returns to bank mode
    state.displayState = SHOWING_BANK;
}
```
**Issue:** Forces return to BANK_MODE even if user was in MANUAL_MODE  
**Recommendation:** Store previous mode before entering CHANNEL_SET_MODE

🟢 **MINOR - Code Duplication:**
Multiple similar bank change blocks:
```cpp
// Bank up
if (state.currentMode == BANK_MODE && sw3Pressed && sw4Pressed) {
    state.currentBank++;
    if (state.currentBank > 32) state.currentBank = 1;
    state.displayState = SHOWING_BANK;
    state.globalPresetActive = false;
    state.activePreset = -1;
    // ...
}
// Bank down - similar code
```
**Recommendation:** Extract to helper method:
```cpp
void adjustBank(int8_t direction) {
    state.currentBank += direction;
    if (state.currentBank > 32) state.currentBank = 1;
    if (state.currentBank < 1) state.currentBank = 32;
    state.displayState = SHOWING_BANK;
    state.globalPresetActive = false;
    state.activePreset = -1;
}
```

---

### 4. Switch Handler (`switches.cpp/h`)

**Strengths:**
- Proper debouncing implementation
- Clean simultaneous press detection
- Long press support with customizable duration

**Issues & Recommendations:**

🟡 **MODERATE - Debounce Algorithm:**
```cpp
if ((now - switches[i].lastDebounceTime) > debounceMs) {
    if (reading != switches[i].currentState) {
        switches[i].currentState = reading;
```
**Issue:** This is a simple time-based debounce. Rapid noise could reset the timer repeatedly  
**Recommendation:** Consider a more robust debounce like counting consecutive stable reads:
```cpp
// Add to SwitchState:
uint8_t stableCount;

// In readAndDebounce:
if (reading == switches[i].lastState) {
    switches[i].stableCount++;
    if (switches[i].stableCount >= STABLE_READS_REQUIRED) {
        switches[i].currentState = reading;
    }
} else {
    switches[i].stableCount = 0;
}
```

🟡 **MODERATE - Long Press Edge Case:**
```cpp
bool SwitchHandler::isLongPress(uint8_t sw1Index, uint8_t sw2Index, uint16_t customLongPressMs) {
    unsigned long now = millis();
    if (!switches[sw1Index].currentState && !switches[sw2Index].currentState) {
        if (!switches[sw1Index].longPressTriggered && !switches[sw2Index].longPressTriggered) {
            if ((now - switches[sw1Index].pressStartTime) > customLongPressMs &&
                (now - switches[sw2Index].pressStartTime) > customLongPressMs) {
```
**Issue:** Requires both switches pressed at exactly the same time. If one is pressed slightly before the other, timing check may fail  
**Recommendation:** Check that both have been held together for the duration:
```cpp
unsigned long laterPressTime = max(switches[sw1Index].pressStartTime, 
                                    switches[sw2Index].pressStartTime);
if ((now - laterPressTime) > customLongPressMs) {
```

🟢 **MINOR - Magic Number:**
```cpp
void clearRecentPresses() {
    for (int i = 0; i < 4; i++) {  // Hard-coded 4
```
**Recommendation:** Use constant `NUM_SWITCHES`

---

### 5. Display Controller (`display.cpp/h`)

**Strengths:**
- Uses LedControl library
- Abstracted display states
- Animated edit mode display

**Issues & Recommendations:**

🟡 **MODERATE - Display Buffer:**
```cpp
void Display::displayBankNumber(uint8_t num, bool globalPreset) {
    clear();  // Clears entire display every update
    setCharAt(7, 'b');
    setCharAt(6, 'A');
    // ...
}
```
**Issue:** Clearing and redrawing entire display on every call can cause flicker  
**Recommendation:** Track display state and only update changed segments:
```cpp
private:
    uint8_t lastDisplayValue;
    DisplayState lastState;
    
void update(...) {
    if (state == lastState && value == lastDisplayValue) return;
    // ... update display
    lastState = state;
    lastDisplayValue = value;
}
```

🟡 **MODERATE - Manual Mode Display:**
```cpp
void Display::displayManualStatus(bool loopStates[4]) {
    clear();
    setCharAt(6, loopStates[3] ? '4' : '_');
    setCharAt(4, loopStates[2] ? '3' : '_');
    setCharAt(2, loopStates[1] ? '2' : '_');
    setCharAt(0, loopStates[0] ? '1' : '_');
}
```
**Issue:** Spacing is not explained, and array indexing seems reversed (loopStates[3] for position 6)  
**Recommendation:** Add comments explaining layout and consider using decimal points instead of underscores:
```cpp
// Display: "4 3 2 1" with spaces, or use decimal points for active loops
setCharAt(6, '4', loopStates[3]);  // Use decimal point for active state
```

🟢 **MINOR - Error Handling:**
No bounds checking on `animFrame`, `num`, `ch` parameters  
**Recommendation:** Add parameter validation

---

### 6. MIDI Handler (`midi_handler.cpp/h`)

**Strengths:**
- Simple and correct MIDI implementation
- Proper baud rate (31250)
- Correct status byte calculation

**Issues & Recommendations:**

🟡 **MODERATE - No Error Handling:**
```cpp
void sendMIDIProgramChange(uint8_t program, uint8_t channel) {
    uint8_t statusByte = 0xC0 | ((channel - 1) & 0x0F);
    uint8_t programByte = (program - 1) & 0x7F;
    Serial.write(statusByte);
    Serial.write(programByte);
}
```
**Issue:** No validation of input parameters, no confirmation of send  
**Recommendation:**
```cpp
void sendMIDIProgramChange(uint8_t program, uint8_t channel) {
    // Validate inputs
    if (program < 1 || program > 128) return;
    if (channel < 1 || channel > 16) return;
    
    uint8_t statusByte = 0xC0 | ((channel - 1) & 0x0F);
    uint8_t programByte = (program - 1) & 0x7F;
    
    // Ensure serial is ready (optional, adds safety)
    while (!Serial.availableForWrite()) { /* wait */ }
    
    Serial.write(statusByte);
    Serial.write(programByte);
}
```

🟢 **MINOR - Feature Gap:**
No MIDI receive capability - this is fine for the current use case but could be a future enhancement

---

### 7. Relay Controller (`relays.cpp/h`)

**Strengths:**
- Simple and effective
- Clean API
- Proper initialization

**Issues & Recommendations:**

🟢 **MINOR - Relay Click Timing:**
```cpp
void RelayController::update(bool loopStates[4]) {
    for (int i = 0; i < 4; i++) {
        digitalWrite(relayPins[i], loopStates[i] ? HIGH : LOW);
    }
}
```
**Issue:** All relays switch simultaneously - could cause power supply sag or audio pops  
**Recommendation:** Consider staggered switching:
```cpp
void RelayController::update(bool loopStates[4]) {
    for (int i = 0; i < 4; i++) {
        digitalWrite(relayPins[i], loopStates[i] ? HIGH : LOW);
        delayMicroseconds(50);  // Small delay between relay switches
    }
}
```

🟢 **MINOR - Relay Protection:**
No mention of flyback diodes in documentation - should be documented in hardware section of README

---

### 8. LED Controller (`led_controller.cpp/h`)

**Strengths:**
- Clean shift register implementation
- Good documentation of bit assignments
- Proper polarity handling

**Issues & Recommendations:**

🟡 **MODERATE - Bit Manipulation:**
```cpp
for (uint8_t i = 0; i < 4; i++) {
    if (appliedLoopStates[i]) {
        outputByte |= (1 << i);
    }
}
```
**Issue:** Works but could be optimized  
**Recommendation:**
```cpp
// More efficient for embedded:
uint8_t outputByte = 0;
if (appliedLoopStates[0]) outputByte |= 0x01;
if (appliedLoopStates[1]) outputByte |= 0x02;
if (appliedLoopStates[2]) outputByte |= 0x04;
if (appliedLoopStates[3]) outputByte |= 0x08;
```

🟢 **MINOR - SPI Alternative:**
Could use hardware SPI instead of bit-banging for better performance  
**Recommendation:** Document why software SPI was chosen (pin flexibility?)

---

### 9. Main Loop (`main.cpp`)

**Strengths:**
- Clean initialization sequence
- Readable main loop
- Separation of concerns

**Issues & Recommendations:**

🟡 **MODERATE - Loop Timing:**
```cpp
void loop() {
    switches.readAndDebounce();
    modeController.detectSwitchPatterns();
    modeController.updateStateMachine();
    // ... rest of loop
}
```
**Issue:** No timing control - loop runs as fast as possible, consuming power unnecessarily  
**Recommendation:**
```cpp
void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Run at ~100Hz (10ms cycle time)
    if (now - lastUpdate < 10) return;
    lastUpdate = now;
    
    switches.readAndDebounce();
    modeController.detectSwitchPatterns();
    modeController.updateStateMachine();
    // ...
}
```

🟡 **MODERATE - Heartbeat LED:**
```cpp
pinMode(LED_BUILTIN, OUTPUT);  // Pin 13, shares with SPI CLK
```
**Issue:** LED_BUILTIN conflicts with MAX7219 clock pin - this will cause display issues  
**Recommendation:** Remove or use different pin:
```cpp
// Remove heartbeat LED as it conflicts with display
// pinMode(LED_BUILTIN, OUTPUT);  // This shares pin 13 with MAX7219!
```

🟢 **MINOR - Power-On State:**
Power-on state behavior not documented - loops are all off by default, should be documented

---

## Cross-Cutting Concerns

### Memory Usage

**Analysis:**
- **Global Variables:** Minimal use, mostly hardware instances
- **Stack Usage:** Moderate - several function calls deep but no recursion
- **EEPROM Usage:** 130 bytes (channel + init flag + 128 presets) - well within limits
- **Flash Usage:** Need to compile to verify, but likely ~50% on ATmega328

**Recommendations:**
- ✅ Add a size report in documentation
- Consider using F() macro for string constants to save RAM:
  ```cpp
  // Current: stored in RAM
  // With F(): stored in flash
  Serial.println(F("Init complete"));
  ```

### Error Handling

**Issues:**
- No error reporting mechanism
- Silent failures (e.g., invalid EEPROM data)
- No diagnostic mode

**Recommendations:**
- Add debug serial output controlled by compile-time flag:
  ```cpp
  #ifdef DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #endif
  ```

### Testing

**Observations:**
- No unit tests present (test directory has only README)
- Hardware-dependent code makes testing difficult

**Recommendations:**
- Create mock implementations for hardware:
  ```cpp
  #ifdef UNIT_TEST
  class MockSerial {
      void write(uint8_t byte) { /* capture for testing */ }
  };
  #else
  #define MockSerial Serial
  #endif
  ```
- Add integration tests that can run on actual hardware
- Document manual testing procedures

### Documentation

**Strengths:**
- README with feature descriptions
- Hardware documentation
- Pin assignments

**Gaps:**
- No timing diagrams for switch patterns
- No state machine diagrams
- No schematic or circuit diagram
- No troubleshooting guide

**Recommendations:**
- Add state machine diagram for mode transitions
- Add timing diagram showing switch press patterns
- Add schematic in docs/ folder
- Add troubleshooting section to README

---

## Security & Safety

### EEPROM Corruption
⚠️ **Risk:** Power loss during EEPROM write could corrupt data  
**Mitigation:** Add checksums or use redundant storage

### Switch Bounce
⚠️ **Risk:** Electrical noise could trigger unintended switches  
**Mitigation:** Current debounce adequate, consider adding hardware filtering

### Relay Sequencing
⚠️ **Risk:** Multiple simultaneous relay switches could stress power supply  
**Mitigation:** Implement staggered relay switching (mentioned above)

---

## Performance Considerations

### CPU Usage
- **Current:** Loop runs continuously, ~16MHz ATmega328
- **Estimated Load:** < 5% CPU utilization
- **Recommendation:** Add sleep modes for power saving in battery applications

### Response Time
- **Switch Response:** 30ms debounce + loop time ≈ 40ms
- **MIDI Latency:** < 1ms
- **Display Update:** < 10ms
- **Overall:** Suitable for musical applications (< 50ms perceivable)

### Memory Footprint
- **SRAM:** Estimated 200-300 bytes (out of 2KB)
- **Flash:** Need actual compilation results
- **EEPROM:** 130 bytes used (out of 1KB)

---

## Code Quality Metrics

### Maintainability: 8/10
- Modular design  
- Clear naming  
- Separation of concerns  
- Some code duplication  
- Limited comments in complex sections

### Reliability: 7/10
- Debouncing implemented  
- Input validation in most places  
- EEPROM wear concerns  
- No error recovery  
- Missing edge case handling

### Portability: 6/10
- Arduino standard libraries  
- Platform-independent logic  
- Hard-coded hardware dependencies  
- AVR-specific assumptions

### Testability: 5/10
- No unit tests  
- Hardware dependencies  
- Tight coupling in places  
- Modular structure

---

## Prioritized Recommendations

### 🔴 HIGH PRIORITY (Fix Soon)
1. **EEPROM Wear Leveling** - Add dirty-check before writes to savePreset()
2. **LED_BUILTIN Conflict** - Remove heartbeat LED that conflicts with display
3. **Input Validation** - Add bounds checking to all EEPROM operations
4. **Double-Press Detection** - Improve reliability for global preset activation

### 🟡 MEDIUM PRIORITY (Next Release)
5. **Mode Transition Memory** - Remember previous mode when exiting channel set
6. **Display Flicker** - Implement display buffering to avoid unnecessary redraws
7. **Loop Timing Control** - Add fixed update rate to reduce power consumption
8. **Error Handling** - Add debug output and error reporting
9. **Long Press Timing** - Fix simultaneous press timing issues
10. **Magic Numbers** - Define constants for array sizes and limits

### 🟢 LOW PRIORITY (Future Enhancement)
11. **Unit Tests** - Add test infrastructure and hardware mocks
12. **Documentation** - Add state diagrams, schematics, and troubleshooting guide
13. **Relay Timing** - Add staggered switching to reduce power spikes
14. **Code Optimization** - Reduce code duplication with helper methods
15. **Feature Additions** - MIDI receive, preset copy/paste, preset naming

---

## Code Style Observations

### Style Observations:
- Consistent indentation (2 spaces)
- Clear variable naming
- Uses const appropriately
- Proper header guards
- Consistent brace style

### Issues:
- Mix of `uint8_t` and `int` (prefer explicit types for embedded)
- Some long functions (> 50 lines) could be split
- Missing const correctness in some methods

---

## Conclusion

The project demonstrates embedded systems programming, Arduino development, and hardware interfacing. Production-ready with high-priority issues addressed.

### Strengths:
- Modular architecture  
- Hardware abstraction  
- Documentation present  
- Debouncing and timing implemented  
- Organized code structure

### Improvements Needed:
- EEPROM wear protection  
- Pin conflict resolution  
- Enhanced error handling  
- Better edge case handling  
- Testing infrastructure

### Rating: **B+ (Very Good)**
Ready for use with recommended fixes. Would benefit from addressing high-priority items and adding tests.

---

**Reviewer Notes:**
- Code was reviewed statically without compilation
- Hardware testing recommended to validate timing assumptions
- Consider code review tools like cppcheck or PlatformIO static analyzer
- Would benefit from a second pair of eyes on the MIDI timing and switch debounce logic

