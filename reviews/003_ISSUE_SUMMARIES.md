# Issue Summaries - Review 002 Action Items

**Generated From:** Technical Review 002 (December 19, 2024)  
**Purpose:** Copy-paste ready issue descriptions for tracking action items  
**Usage:** Copy individual issue sections below into GitHub Issues

---

## 🔴 CRITICAL PRIORITY - Phase 1
*Do Before Next Hardware Build*

### Issue: Document Relay Driver Circuit

**Priority:** Critical  
**Area:** Hardware Documentation  
**Effort:** 1-2 hours

#### Description
The relay driver circuit needs to be fully documented including driver ICs (ULN2003/ULN2803 or transistor array) and flyback diodes. This is essential for safe operation and future hardware builds.

#### Background
- Relay coils are inductive loads that generate voltage spikes when switched off
- ATmega328 GPIO pins cannot drive relays directly (40mA max per pin)
- Flyback diodes are required across each relay coil to protect the driver circuit
- Current code assumes proper driver circuit exists but it's not documented

#### Acceptance Criteria
- [ ] Create schematic showing relay driver circuit
- [ ] Document driver IC part number (e.g., ULN2003A, ULN2803A)
- [ ] Confirm flyback diodes are included (1N4001 or equivalent)
- [ ] Document current requirements per relay
- [ ] Add schematic to hardware/ folder as PDF
- [ ] Update README with link to hardware documentation

#### Files to Create/Update
- `hardware/relay_driver_schematic.pdf` (new)
- `hardware/README.md` (new or update)
- `README.md` (add link to hardware docs)

#### References
- Review 002, Section: Hardware Integration Review
- Current relay control code: `src/relay_controller.cpp`

---

### Issue: Verify Hardware SPI Usage for MAX7219

**Priority:** Critical  
**Area:** Performance/Hardware  
**Effort:** 1 hour (investigation + potential fix)

#### Description
Verify that the LedControl library is using hardware SPI for the MAX7219 display driver. The pin configuration matches hardware SPI pins (DIN=D11/MOSI, CLK=D13/SCK, CS=D12/SS), but library source inspection is needed to confirm optimal performance.

#### Background
- Current pin configuration: DIN=D11, CLK=D13, CS=D12
- These match ATmega328 hardware SPI pins perfectly
- LedControl library may use software bit-banging instead of hardware SPI
- Hardware SPI would be significantly faster and reduce CPU usage

#### Acceptance Criteria
- [ ] Inspect LedControl library source code
- [ ] Confirm if hardware SPI or software bit-banging is used
- [ ] If software: Evaluate performance impact
- [ ] Document findings in code comments
- [ ] If needed: Switch to hardware SPI implementation or different library
- [ ] Profile display update timing before/after any changes

#### Technical Details
```cpp
// Current initialization in main.cpp
Display display(MAX7219_DIN, MAX7219_CLK, MAX7219_CS);

// Pins match hardware SPI:
// DIN = D11 (MOSI)
// CLK = D13 (SCK)  
// CS  = D12 (SS)
```

#### Files to Investigate
- LedControl library source
- `src/display.cpp`
- `src/display.h`
- `src/config.h`

#### References
- Review 002, Section: ATmega328-Specific Analysis → Optimization Opportunities
- Display module code: `src/display.cpp`

---

### Issue: Create Comprehensive Hardware Schematic

**Priority:** Critical  
**Area:** Documentation  
**Effort:** 4-6 hours

#### Description
Create a complete hardware schematic PDF documenting all electrical connections, component values, and part numbers. This is essential for building additional units and troubleshooting.

#### Background
- Current documentation only lists pin assignments in config.h
- No complete circuit schematic exists
- Multiple hardware subsystems need documentation:
  - Relay driver circuit (ULN2003 + flyback diodes)
  - 74HC595 LED driver circuit
  - MAX7219 display circuit
  - MIDI output circuit (DIN-5 connector)
  - Power supply requirements

#### Acceptance Criteria
- [ ] Create schematic showing all major subsystems
- [ ] Document component part numbers and values
- [ ] Include power supply specifications (voltage, current)
- [ ] Show MIDI output circuit with proper isolation
- [ ] Document LED current limiting resistors
- [ ] Include PCB layout recommendations (if applicable)
- [ ] Export as PDF to hardware/ folder
- [ ] Add bill of materials (BOM)

#### Subsystems to Document
1. ATmega328 board (Arduino Uno/Nano)
2. 4x footswitch inputs with pullups
3. 4x DIP switches for MIDI channel
4. 4x DPDT relay outputs with drivers
5. 74HC595 shift register + 8 LEDs
6. MAX7219 + 7-segment display
7. MIDI output (5-pin DIN, optoisolated)
8. Power supply circuit

#### Files to Create
- `hardware/schematic_complete.pdf`
- `hardware/BOM.md` (Bill of Materials)
- `hardware/README.md` (assembly guide)

#### References
- Review 002, Section: Recommended Action Items → Critical
- Pin configuration: `src/config.h`

---

## 🟡 HIGH PRIORITY - Phase 2
*Next Software Update*

### Issue: Improve Global Preset Double-Press Detection

**Priority:** High  
**Area:** User Experience / Control Logic  
**Effort:** 2-3 hours

#### Description
Improve the global preset activation mechanism with explicit double-press detection using a time window. Current implementation may not reliably detect quick double-presses, making the global preset feature difficult to use.

#### Current Behavior
The code detects when the same preset button is pressed twice to activate the global preset (all loops off). However, there's no explicit time window enforcement - it relies on the activePreset state still being set when the second press occurs.

#### Problem
- No explicit time window for double-press detection
- Users might trigger accidentally or miss it due to timing
- Detection window is implicit and unpredictable
- May not work consistently across different usage patterns

#### Proposed Solution
Add explicit double-tap detection with a configurable time window (e.g., 500ms):

```cpp
// Add to state_manager.h:
struct AppState {
    // ... existing fields ...
    unsigned long lastPresetPressTime;
    uint8_t lastPressedPreset;
};

// In mode_controller.cpp handleSingleSwitchPress():
unsigned long now = millis();
if (state.lastPressedPreset == switchIndex && 
    (now - state.lastPresetPressTime) < 500) {  // 500ms window
    // Activate global preset
    state.globalPresetActive = true;
    sendMIDIProgramChange(TOTAL_PRESETS, state.midiChannel);
    // Clear tracking
    state.lastPressedPreset = 0xFF;
} else {
    // Normal preset selection
    state.lastPressedPreset = switchIndex;
    state.lastPresetPressTime = now;
    // ... existing preset activation code ...
}
```

#### Acceptance Criteria
- [ ] Add lastPresetPressTime and lastPressedPreset to AppState
- [ ] Implement time-window based double-press detection
- [ ] Use configurable time window constant (500ms recommended)
- [ ] Clear tracking variables after global preset activation
- [ ] Handle edge cases (mode transitions, timeouts)
- [ ] Test with hardware to confirm improved reliability
- [ ] Update documentation with double-press timing requirements

#### Files to Modify
- `src/state_manager.h` (add state fields)
- `src/mode_controller.cpp` (update handleSingleSwitchPress)
- `src/config.h` (add DOUBLE_PRESS_WINDOW_MS constant)
- `README.md` (update control documentation)

#### References
- Review 002, Section: Mode Controller → Issue - Global Preset Activation
- Current implementation: `mode_controller.cpp` lines 142-150

---

### Issue: Add EEPROM Checksum Validation

**Priority:** High  
**Area:** Data Integrity / Reliability  
**Effort:** 2-3 hours

#### Description
Add checksum validation for EEPROM data to detect corruption. Currently, corrupted EEPROM would load invalid preset data without detection, potentially causing unexpected behavior.

#### Background
- EEPROM can become corrupted due to power loss during write, bit rot, or hardware issues
- Current init flag (0x42) provides basic validation but doesn't detect data corruption
- Invalid preset data could cause relay states that don't match saved presets
- Users would have no indication that data is corrupted

#### Proposed Solution
Implement simple XOR checksum for preset data:

```cpp
// Add to state_manager.cpp:
uint8_t calculateChecksum(uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// Update EEPROM layout:
// [INIT_FLAG][PRESET_0][CHECKSUM_0]...[PRESET_N][CHECKSUM_N]
//     0         1          2           3          4

// On save:
void savePreset(uint8_t presetNumber, bool loopStates[4]) {
    // ... existing save code ...
    uint8_t checksum = calculateChecksum(loopStates, 4);
    EEPROM.write(address + 1, checksum);
}

// On load:
bool loadPreset(uint8_t presetNumber, bool loopStates[4]) {
    // ... read preset data ...
    uint8_t stored_checksum = EEPROM.read(address + 1);
    uint8_t calculated_checksum = calculateChecksum(loopStates, 4);
    
    if (stored_checksum != calculated_checksum) {
        // Corruption detected - load default (all off)
        for (int i = 0; i < 4; i++) loopStates[i] = false;
        return false;  // Indicate corruption
    }
    return true;  // Data valid
}
```

#### Acceptance Criteria
- [ ] Implement calculateChecksum() function
- [ ] Update EEPROM memory layout to include checksums
- [ ] Modify savePreset() to write checksum after data
- [ ] Modify loadPreset() to validate checksum
- [ ] Handle corrupted data gracefully (load safe defaults)
- [ ] Add debug output when corruption detected
- [ ] Update ARCHITECTURE.md with new memory layout
- [ ] Test corruption detection with intentionally corrupted EEPROM

#### EEPROM Layout Changes
```
Old layout (1 byte per preset):
[INIT_FLAG][P0][P1][P2]...[P127]

New layout (2 bytes per preset):
[INIT_FLAG][P0][C0][P1][C1]...[P127][C127]
  Addr 0     1   2   3   4        255  256

Space required: 1 + (128 * 2) = 257 bytes (was 129 bytes)
Available space: 1024 bytes
```

#### Files to Modify
- `src/state_manager.cpp` (add checksum functions, update save/load)
- `src/state_manager.h` (update constants if needed)
- `docs/ARCHITECTURE.md` (update memory map)

#### References
- Review 002, Section: Security Considerations → EEPROM Data Integrity
- Current EEPROM code: `state_manager.cpp` lines 82-120

---

### Issue: Create Hardware Integration Test Procedure

**Priority:** High  
**Area:** Testing / Documentation  
**Effort:** 3-4 hours

#### Description
Create a comprehensive hardware integration test procedure document to ensure consistent testing across builds and catch hardware issues early.

#### Background
- No formal test procedure exists
- Hardware projects need systematic testing to catch issues
- Multiple operating modes and features need validation
- MIDI output needs verification with external tools
- Preset storage/recall needs validation

#### Required Test Procedures

##### 1. Power-On Test
- Verify MIDI channel display (1-16 based on DIP switches)
- Check all LEDs off initially
- Confirm all relays off (no clicks during power-up)
- Verify display shows channel then enters default mode

##### 2. Manual Mode Test
- Single switch press toggles loop on/off
- Verify relay click and LED state match
- Test all 4 switches independently
- Check display shows manual mode indicator

##### 3. Bank Mode Test
- SW2+SW3 enters bank mode
- Bank up/down works (SW3+SW4, SW1+SW2)
- Display shows current bank (1-32)
- Preset selection sends correct MIDI PC
- Verify PC numbers with MIDI monitor
- Test double-press for global preset
- Verify LEDs indicate selected preset

##### 4. Edit Mode Test
- Select preset in bank mode
- Hold SW2+SW3 for 2s to enter edit
- Verify "Edit" animation on display
- Toggle loops, verify relays update in real-time
- Exit with SW2+SW3 hold for 2s
- Verify "SAVEd" message displays
- Re-enter bank mode and load preset to confirm save

##### 5. MIDI Channel Configuration Test
- Power off device
- Set DIP switches to different channel (0-15)
- Power on, verify channel display matches (1-16)
- Send PC from preset, verify correct channel with MIDI monitor
- Test multiple channel configurations

##### 6. EEPROM Persistence Test
- Save presets in edit mode (multiple banks)
- Note which presets were saved and their loop states
- Power cycle device completely
- Load saved presets, verify loop states match
- Verify LEDs and relays match saved states

##### 7. Stress Test
- Rapid switch presses in all modes
- Mode transitions during operations
- Bank changes during preset recall
- Continuous operation for extended period

#### Acceptance Criteria
- [ ] Document each test procedure with step-by-step instructions
- [ ] Include expected results for each step
- [ ] Add troubleshooting section for common failures
- [ ] List required test equipment (MIDI monitor, multimeter)
- [ ] Create test checklist for quick validation
- [ ] Include MIDI message verification examples
- [ ] Add photos/diagrams where helpful
- [ ] Create test log template for tracking results

#### Required Tools
- MIDI-OX or similar MIDI monitor software
- MIDI interface for computer
- Multimeter (for checking relay voltage)
- Test audio signal (for testing with actual effects)

#### Files to Create
- `docs/TESTING_PROCEDURE.md`
- `docs/TEST_CHECKLIST.md`
- `docs/TROUBLESHOOTING.md`

#### References
- Review 002, Section: Testing Recommendations
- All source files in `src/` directory

---

## 🟢 MEDIUM PRIORITY - Phase 3
*Nice to Have / Future Updates*

### Issue: Add SoftwareSerial for Debug Output

**Priority:** Medium  
**Area:** Development / Debugging  
**Effort:** 2-3 hours

#### Description
Implement SoftwareSerial for debug output to free up the hardware UART for exclusive MIDI use. Currently, debug mode and MIDI output share the same Serial port, requiring debug to be disabled for MIDI operation.

#### Current Limitation
- Hardware Serial is shared between MIDI TX and DEBUG_MODE
- Debug output interferes with MIDI messages
- Developer must choose between debugging and MIDI testing
- This limits ability to debug MIDI-related issues

#### Proposed Solution
Move debug output to SoftwareSerial on an unused pin (D3 is currently available):

```cpp
// Add to config.h:
#ifdef DEBUG_MODE
#include <SoftwareSerial.h>
const uint8_t DEBUG_RX = 3;  // Not used but required
const uint8_t DEBUG_TX = 3;  // D3 available
extern SoftwareSerial DebugSerial;
#define DEBUG_PRINT(x) DebugSerial.print(x)
#define DEBUG_PRINTLN(x) DebugSerial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// In main.cpp setup():
#ifdef DEBUG_MODE
    DebugSerial.begin(9600);
    DEBUG_PRINTLN("Debug initialized on D3");
#endif
```

#### Benefits
- Hardware Serial dedicated to MIDI (no conflicts)
- Can debug MIDI-related issues while MIDI is active
- Better development workflow
- No impact on production builds (debug disabled)

#### Acceptance Criteria
- [ ] Add SoftwareSerial library include for debug builds
- [ ] Define DEBUG_TX pin (D3) in config.h
- [ ] Create DEBUG_PRINT and DEBUG_PRINTLN macros
- [ ] Update all existing debug output to use macros
- [ ] Initialize SoftwareSerial in setup() when DEBUG_MODE enabled
- [ ] Test debug output on serial terminal
- [ ] Verify MIDI output unaffected during debug
- [ ] Update README with debug setup instructions

#### Pin Allocation
- D3 currently unused
- Will be dedicated to debug TX when DEBUG_MODE enabled
- No conflict with other features

#### Files to Modify
- `src/config.h` (add SoftwareSerial setup)
- `src/main.cpp` (initialize DebugSerial)
- All files with debug output (replace Serial with DEBUG_PRINT)

#### References
- Review 002, Section: Recommended Action Items → Medium Priority
- Current debug implementation in `config.h`

---

### Issue: Optimize 74HC595 LED Driver to Use Hardware SPI

**Priority:** Medium  
**Area:** Performance Optimization  
**Effort:** 3-4 hours

#### Description
Optimize the 74HC595 shift register driver to use hardware SPI instead of software bit-banging. This will improve LED update speed and reduce CPU usage.

#### Current Implementation
The 74HC595 is controlled via software bit-banging on pins A0-A2:
- DATA: A0
- LATCH: A1  
- CLOCK: A2

This works but is slower than hardware SPI and uses CPU cycles.

#### Proposed Solution
Use ATmega328 hardware SPI with a different SS pin:

```cpp
// New pin configuration:
// MOSI (D11) -> 74HC595 DATA (currently used by MAX7219)
// SCK (D13)  -> 74HC595 CLOCK (currently used by MAX7219)
// D9 (or other) -> 74HC595 LATCH (dedicated SS)

// Challenge: Both MAX7219 and 74HC595 would share SPI bus
// Need different SS/LATCH pins and careful timing

// In led_controller.cpp:
#include <SPI.h>

void LedController::shiftOut(uint8_t data) {
    digitalWrite(_latchPin, LOW);
    SPI.transfer(data);  // Hardware SPI
    digitalWrite(_latchPin, HIGH);
}
```

#### Tradeoffs
**Pros:**
- Faster LED updates (hardware vs software)
- Frees up 3 GPIO pins (A0-A2)
- Lower CPU usage
- More robust timing

**Cons:**
- Shares SPI bus with MAX7219 display
- Requires buffering or careful sequencing
- More complex initialization
- Need to verify no bus conflicts

#### Acceptance Criteria
- [ ] Research SPI sharing between MAX7219 and 74HC595
- [ ] Update pin configuration to use hardware SPI
- [ ] Modify led_controller.cpp to use SPI.transfer()
- [ ] Implement proper SS/LATCH control
- [ ] Test for timing conflicts with display updates
- [ ] Verify LED updates are faster (profile if possible)
- [ ] Update documentation with new pin configuration
- [ ] Consider making this optional (conditional compilation)

#### Files to Modify
- `src/config.h` (update pin definitions)
- `src/led_controller.cpp` (use SPI library)
- `src/led_controller.h` (update constructor)
- `src/main.cpp` (update initialization)
- `README.md` (update pin documentation)

#### References
- Review 002, Section: ATmega328-Specific Analysis → Optimization Opportunities
- Current implementation: `led_controller.cpp` lines 23-33

---

### Issue: Add Const Correctness Throughout Codebase

**Priority:** Medium  
**Area:** Code Quality  
**Effort:** 2-3 hours

#### Description
Improve const correctness throughout the codebase by adding `const` qualifiers to function parameters, return values, and methods that don't modify state. This improves code safety and catches potential bugs at compile time.

#### Current State
Some functions use `const` but application is inconsistent. Many read-only parameters and methods could be marked `const`.

#### Examples of Improvements

```cpp
// In relay_controller.h:
// Current:
void update(bool loopStates[4]);

// Improved:
void update(const bool loopStates[4]);

// In display.h:
// Current:
void setChar(uint8_t position, char c, bool dp);

// Improved:
void setChar(uint8_t position, char c, bool dp) const;  // If doesn't modify state

// In state_manager.h:
// Current:
void loadPreset(uint8_t presetNumber, bool loopStates[4]);

// Improved:
bool loadPreset(uint8_t presetNumber, bool loopStates[4]) const;

// Current:
uint8_t calculatePresetNumber(uint8_t bank, uint8_t switchIndex);

// Improved:
uint8_t calculatePresetNumber(uint8_t bank, uint8_t switchIndex) const;
```

#### Areas to Review
1. Function parameters that are read-only
2. Member functions that don't modify object state
3. Return values that shouldn't be modified
4. Local variables that don't change
5. Pointers vs const pointers vs pointer to const

#### Benefits
- Compile-time safety (catches unintended modifications)
- Documents intent (this function doesn't modify parameter)
- Enables compiler optimizations
- Follows C++ best practices
- Improves code maintainability

#### Acceptance Criteria
- [ ] Review all function declarations in header files
- [ ] Add `const` to read-only parameters
- [ ] Add `const` to member functions that don't modify state
- [ ] Add `const` to local variables that don't change
- [ ] Ensure code still compiles with all changes
- [ ] Run full test suite (if exists)
- [ ] Update any callers if needed
- [ ] Document rationale for any non-const choices

#### Files to Review
- All `.h` header files
- All `.cpp` implementation files
- Focus on public interfaces first

#### References
- Review 002, Section: Embedded Best Practices Checklist
- C++ const correctness best practices

---

### Issue: Expand Inline Comments in Complex Sections

**Priority:** Medium  
**Area:** Documentation / Maintainability  
**Effort:** 2-3 hours

#### Description
Add more detailed inline comments to complex sections of code, particularly around state management, timing logic, and MIDI calculations. While the code is generally clear, additional comments would improve maintainability.

#### Sections Needing More Comments

##### 1. Switch Debouncing Logic
`switches.cpp` - The time-based debouncing algorithm could use step-by-step comments:

```cpp
void SwitchHandler::readAndDebounce() {
    unsigned long now = millis();
    
    for (int i = 0; i < 4; i++) {
        bool reading = (digitalRead(switches[i].pin) == LOW);
        
        // Track when the reading changes from last state
        // This resets the debounce timer
        if (reading != switches[i].lastState) {
            switches[i].lastDebounceTime = now;
        }
        
        // Only update current state if reading has been stable
        // for longer than debounce period
        if ((now - switches[i].lastDebounceTime) > debounceMs) {
            // Reading is stable, safe to update state
            if (reading != switches[i].currentState) {
                switches[i].currentState = reading;
                
                // Track timing for press/release detection
                if (reading) {  // Switch pressed
                    switches[i].pressTime = now;
                } else {  // Switch released
                    switches[i].releaseTime = now;
                }
            }
        }
        
        switches[i].lastState = reading;
    }
}
```

##### 2. MIDI Program Change Calculation
`mode_controller.cpp` - Explain the PC number calculation:

```cpp
// Calculate MIDI Program Change number
// Formula: PC = ((bank - 1) * 4) + switchIndex + 1
// Example: Bank 2, Switch 3 = ((2-1)*4) + 3 + 1 = 8
// Banks: 1-32, Switches: 0-3, Result: PC 1-128
uint8_t programChange = ((state.currentBank - 1) * 4) + switchIndex + 1;
```

##### 3. State Machine Transitions
`mode_controller.cpp` - Document mode transition logic:

```cpp
// Mode transitions:
// Manual <-> Bank: SW2+SW3 simultaneous
// Bank -> Edit: SW2+SW3 held 2s (requires active preset)
// Edit -> Bank: SW2+SW3 held 2s (saves changes)
```

##### 4. Display Buffer Management
`display.cpp` - Explain buffering strategy:

```cpp
// Display buffering prevents unnecessary SPI transactions
// Only update MAX7219 when buffer content differs from new value
// Tracks: character value, digit vs char mode, decimal point state
```

##### 5. EEPROM Layout
`state_manager.cpp` - Document memory structure:

```cpp
// EEPROM Layout:
// Address 0: Init flag (0x42 = initialized)
// Address 1-128: Preset data (1 byte per preset)
//   Bits 0-3: Loop states (4 bits for 4 loops)
//   Bits 4-7: Reserved for future use
```

#### Acceptance Criteria
- [ ] Add comments to switch debouncing algorithm
- [ ] Document MIDI PC calculation with examples
- [ ] Explain state machine transitions
- [ ] Document display buffering strategy
- [ ] Clarify EEPROM memory layout
- [ ] Add comments to timing calculations
- [ ] Document any "magic numbers" with explanation
- [ ] Ensure comments explain "why" not just "what"

#### Comment Style Guidelines
- Use // for single-line comments
- Use /* */ for multi-line explanations
- Add ASCII diagrams where helpful (state machines, memory layouts)
- Explain the reasoning behind non-obvious decisions
- Document assumptions and constraints

#### Files to Update
- `src/switches.cpp` (debouncing)
- `src/mode_controller.cpp` (state machine, MIDI)
- `src/display.cpp` (buffering)
- `src/state_manager.cpp` (EEPROM)
- `src/config.h` (pin assignments, constants)

#### References
- Review 002, Section: Embedded Best Practices Checklist
- Review 002, Section: Code Quality Analysis

---

## ⚪ LOW PRIORITY - Phase 4
*Future Enhancements*

### Issue: Implement Simple Unit Tests for State Logic

**Priority:** Low  
**Area:** Testing  
**Effort:** 4-6 hours

#### Description
Add basic unit test infrastructure for hardware-independent logic, focusing on state management and MIDI calculations. Full embedded testing is challenging, but pure logic functions can be tested.

#### Testable Components
Functions that don't require hardware and can be tested in isolation:

1. **MIDI PC Calculation**
   - Input: bank number, switch index
   - Output: program change number (1-128)
   - Edge cases: bank 1 switch 0, bank 32 switch 3

2. **Preset Number Calculation**
   - Similar to PC calculation but different range
   - Test boundary conditions

3. **State Transitions**
   - Mode switching logic
   - Valid/invalid transitions
   - State persistence

4. **EEPROM Address Calculation**
   - Map preset number to EEPROM address
   - Bounds checking

#### Recommended Framework
**ArduinoUnit** or **AUnit** - lightweight test frameworks for Arduino:

```cpp
// Example test structure:
#include <AUnit.h>

test(midi_pc_calculation) {
    // Test bank 1, switch 0 = PC 1
    uint8_t pc = calculateProgramChange(1, 0);
    assertEqual(pc, 1);
    
    // Test bank 1, switch 3 = PC 4
    pc = calculateProgramChange(1, 3);
    assertEqual(pc, 4);
    
    // Test bank 32, switch 3 = PC 128
    pc = calculateProgramChange(32, 3);
    assertEqual(pc, 128);
}

test(eeprom_address_bounds) {
    // Test valid range
    assertTrue(isValidPresetNumber(1));
    assertTrue(isValidPresetNumber(128));
    
    // Test invalid range
    assertFalse(isValidPresetNumber(0));
    assertFalse(isValidPresetNumber(129));
}

test(mode_transitions) {
    AppState state;
    state.currentMode = MODE_MANUAL;
    
    // Manual -> Bank transition
    switchMode(&state, MODE_BANK);
    assertEqual(state.currentMode, MODE_BANK);
    
    // Cannot enter edit without active preset
    switchMode(&state, MODE_EDIT);
    assertNotEqual(state.currentMode, MODE_EDIT);
}
```

#### Implementation Approach
1. Extract testable functions to separate files if needed
2. Create test/ directory with test files
3. Add test environment to platformio.ini
4. Write tests for pure functions first
5. Mock hardware dependencies where necessary
6. Run tests as part of build verification

#### Acceptance Criteria
- [ ] Choose and set up test framework (AUnit recommended)
- [ ] Create test/ directory structure
- [ ] Write tests for MIDI PC calculation
- [ ] Write tests for EEPROM address calculation
- [ ] Write tests for bounds checking
- [ ] Write tests for state validation
- [ ] Add test environment to platformio.ini
- [ ] Document how to run tests in README
- [ ] Ensure tests pass on initial run
- [ ] Set up CI to run tests automatically (optional)

#### Files to Create
- `test/test_midi.cpp`
- `test/test_state.cpp`
- `test/test_eeprom.cpp`
- `platformio.ini` (add test environment)
- `docs/TESTING.md` (test documentation)

#### References
- Review 002, Section: Testing Recommendations → Unit Tests
- AUnit framework: https://github.com/bxparks/AUnit

---

### Issue: Add Display Brightness Adjustment

**Priority:** Low  
**Area:** User Experience / Power Optimization  
**Effort:** 1-2 hours

#### Description
Add the ability to adjust display brightness, potentially saving power and allowing users to customize for their environment. The MAX7219 supports 16 intensity levels (0-15).

#### Current Implementation
```cpp
// Fixed intensity in display initialization:
lc.setIntensity(0, 8);  // Medium brightness (0-15 scale)
```

#### Proposed Solutions

##### Option 1: Fixed Lower Brightness (Simplest)
```cpp
// Reduce default brightness for power savings
lc.setIntensity(0, 4);  // Lower intensity, still readable indoors
```
**Savings:** ~50mA reduction  
**Effort:** 5 minutes

##### Option 2: User-Adjustable via Long Press (Advanced)
```cpp
// Long press SW1 in Manual mode to cycle brightness
// Levels: Low (2), Medium (6), High (10), Max (15)
if (sw1LongPress && currentMode == MANUAL) {
    brightnessLevel = (brightnessLevel + 1) % 4;
    uint8_t intensity = brightnessLevels[brightnessLevel];
    display.setIntensity(intensity);
    // Save to EEPROM for persistence
}
```
**Savings:** User-dependent  
**Effort:** 2-3 hours

##### Option 3: Ambient Light Sensor (Future)
Use analog input (A3-A5 available) with photoresistor for auto-adjust.

#### Acceptance Criteria
- [ ] Decide on implementation option
- [ ] Update display initialization with new intensity
- [ ] If adjustable: Add control mechanism
- [ ] If adjustable: Store preference in EEPROM
- [ ] Test visibility at different brightness levels
- [ ] Measure power consumption change
- [ ] Update documentation with brightness info

#### Power Impact
```
Current: 100-150mA @ intensity 8
Reduced: 50-100mA @ intensity 4
Savings: ~50mA (10-15% total system power)
```

#### Files to Modify
- `src/display.cpp` (intensity setting)
- `src/config.h` (brightness constants)
- `src/state_manager.cpp` (if saving to EEPROM)
- `README.md` (document brightness control)

#### References
- Review 002, Section: Power Consumption Analysis
- MAX7219 datasheet: intensity control

---

### Issue: Create Troubleshooting Guide

**Priority:** Low  
**Area:** Documentation  
**Effort:** 2-3 hours

#### Description
Create a comprehensive troubleshooting guide for common issues users might encounter during assembly, testing, or operation.

#### Common Issues to Document

##### 1. Display Issues
- No display on power-up
  - Check power supply (5V, adequate current)
  - Verify MAX7219 wiring (DIN, CLK, CS)
  - Test with simple MAX7219 example sketch
- Display flickering or garbled
  - Check for loose connections
  - Verify proper grounding
  - Reduce display intensity

##### 2. MIDI Issues
- No MIDI output
  - Check baud rate (31,250)
  - Verify TX pin connected to MIDI circuit
  - Test with MIDI monitor software
  - Check MIDI output circuit (optoisolator, resistors)
- Wrong MIDI messages
  - Verify DIP switch configuration
  - Check PC calculation in code
  - Test with known-good MIDI device

##### 3. Switch Issues
- Switches not responding
  - Check switch wiring (active LOW expected)
  - Verify internal pullups enabled
  - Test switch continuity with multimeter
  - Check debounce settings
- False triggering
  - Increase debounce time
  - Check for noise on input lines
  - Add capacitors across switches (hardware)

##### 4. Relay Issues
- Relays not clicking
  - Check driver circuit (ULN2003)
  - Verify power supply current capacity
  - Test relay coil directly (12V/5V depending on relay)
  - Check flyback diodes orientation
- Relays clicking but no audio switching
  - Check relay contact wiring
  - Verify DPDT relay connections
  - Test audio continuity with multimeter
  - Check for cold solder joints

##### 5. LED Issues
- LEDs not lighting
  - Check 74HC595 wiring
  - Verify current limiting resistors
  - Test 74HC595 with simple sketch
  - Check power supply
- Wrong LEDs lighting
  - Verify bit order in shiftOut
  - Check wiring order (Q0-Q7)
  - Review LED configuration in code

##### 6. Mode Issues
- Cannot enter bank mode
  - Check SW2+SW3 simultaneous press detection
  - Verify switch timing window
  - Test switches individually first
- Cannot save presets
  - Check EEPROM initialization
  - Verify EEPROM not write-protected
  - Check available EEPROM space

#### Troubleshooting Workflow
```
1. Power-up test
   ├─ Display shows channel? → YES: Continue
   │                        → NO: Check display circuit
   ├─ LEDs all off? → YES: Continue  
   │                → NO: Check LED driver
   └─ Relays all off? → YES: Continue
                      → NO: Check relay driver

2. Basic function test
   ├─ Manual mode works? → YES: Continue
   │                     → NO: Check switches
   ├─ Bank mode works? → YES: Continue
   │                   → NO: Check mode switching
   └─ MIDI output works? → YES: System OK
                         → NO: Check MIDI circuit
```

#### Acceptance Criteria
- [ ] Document all common failure modes
- [ ] Provide step-by-step diagnostic procedures
- [ ] Include multimeter test points
- [ ] Add photos of common wiring errors
- [ ] Create diagnostic flowcharts
- [ ] Include voltage/current specifications
- [ ] Link to component datasheets
- [ ] Add "still stuck?" section with support info

#### Files to Create
- `docs/TROUBLESHOOTING.md`
- `docs/images/` (troubleshooting photos)

#### References
- Review 002, Section: Recommended Action Items
- All component datasheets

---

### Issue: Design PCB Layout for Multiple Builds

**Priority:** Low  
**Area:** Hardware / Manufacturing  
**Effort:** 8-16 hours

#### Description
Create a printed circuit board (PCB) layout if building multiple units. This improves reliability, reduces assembly time, and creates a more professional product.

#### Current State
- Breadboard or point-to-point wiring
- Time-consuming assembly
- Potential for wiring errors
- Difficult to replicate

#### Proposed PCB Features
1. **Main Board**
   - Arduino Nano socket
   - 4x relay outputs with drivers
   - 74HC595 LED driver circuit
   - MAX7219 display connector
   - MIDI output circuit
   - Power input with protection
   - All connections labeled

2. **Control Board**
   - 4x footswitch connectors
   - 4x DIP switch for MIDI channel
   - 8x LED positions with current limiting
   - Connects to main board via header

3. **Display Board**
   - MAX7219 + 7-segment display
   - Connects to main board via cable

#### Design Considerations
- 2-layer PCB (keeps cost low)
- Through-hole components where possible (easier assembly)
- Proper ground plane
- MIDI isolation circuit
- Relay driver with flyback protection integrated
- Test points for debugging
- Mounting holes for enclosure
- Clear component labeling

#### PCB Design Process
1. Create schematic in KiCad or Eagle
2. Assign footprints to components
3. Design PCB layout
4. Check design rules (clearance, trace width)
5. Generate Gerber files
6. Order prototype from OSH Park, JLCPCB, or similar
7. Assemble and test prototype
8. Iterate if needed
9. Document assembly process

#### Acceptance Criteria
- [ ] Complete schematic in PCB design software
- [ ] Layout PCB with proper trace widths
- [ ] Include all necessary circuits
- [ ] Add test points and labels
- [ ] Pass design rule check
- [ ] Generate Gerber files
- [ ] Order and test prototype (optional)
- [ ] Document BOM with part numbers
- [ ] Create assembly instructions
- [ ] Share design files in hardware/ folder

#### Files to Create
- `hardware/pcb/schematic.pdf`
- `hardware/pcb/layout.pdf`
- `hardware/pcb/gerbers.zip`
- `hardware/pcb/BOM.csv`
- `hardware/pcb/assembly_guide.md`
- KiCad/Eagle project files

#### References
- Review 002, Section: Recommended Action Items → Low Priority
- Current schematic (to be created first)

---

## 📊 Priority Summary

### Phase 1: Critical (Before Next Build)
- Document Relay Driver Circuit
- Verify Hardware SPI for MAX7219
- Create Hardware Schematic

**Estimated Total:** 6-9 hours

### Phase 2: High Priority (Next Update)
- Improve Double-Press Detection
- Add EEPROM Checksum
- Create Test Procedure

**Estimated Total:** 7-10 hours

### Phase 3: Medium Priority (Nice to Have)
- Add SoftwareSerial Debug
- Optimize 74HC595 with SPI
- Improve Const Correctness
- Expand Inline Comments

**Estimated Total:** 9-13 hours

### Phase 4: Low Priority (Future)
- Add Unit Tests
- Display Brightness Adjustment
- Troubleshooting Guide
- PCB Layout

**Estimated Total:** 15-25 hours

---

## 🎯 Quick Start Recommendations

### For Immediate Production
Focus on **Phase 1 Critical items** only:
1. Document relay driver circuit (hardware safety)
2. Verify SPI performance (ensure optimal operation)
3. Create complete schematic (enable replication)

### For Next Software Release
Complete **Phase 2 High Priority** items:
1. Fix double-press detection (user experience)
2. Add EEPROM checksum (data integrity)
3. Create test procedure (quality assurance)

### For Long-Term Improvement
Work through **Phase 3 & 4** as time permits:
- Focus on items that provide most value for your use case
- Consider community feedback on priorities
- Skip items that don't align with project goals

---

**Document Version:** 1.0  
**Generated:** December 19, 2024  
**Source:** Technical Review 002  
**Usage:** Copy individual issue sections into GitHub Issues as needed
