# MIDI Enhancements Design Document

**Issue:** #23 - MIDI Enhancements  
**Status:** Design Phase  
**Date:** 2025-12-17

## Overview

This document outlines the design for two major enhancements to the loop switcher:
1. **MIDI IN**: Support for incoming MIDI Program Change messages to load presets
2. **CC Mode**: A third operating mode that sends MIDI Control Change messages

## Table of Contents

1. [MIDI IN Design](#midi-in-design)
2. [CC Mode Design](#cc-mode-design)
3. [State Machine Updates](#state-machine-updates)
4. [Display States](#display-states)
5. [EEPROM Layout Changes](#eeprom-layout-changes)
6. [Hardware Requirements](#hardware-requirements)
7. [Implementation Considerations](#implementation-considerations)

---

## MIDI IN Design

### Overview

Add support for receiving MIDI Program Change (PC) messages to automatically load presets. When a PC message matching the device's MIDI channel is received, the device will:
- Switch to BANK_MODE if not already in it
- Calculate the appropriate bank and preset
- Load and activate the preset

### Hardware Requirements

**Current State:**
- TX (Pin 1) used for MIDI output via hardware UART
- RX (Pin 0) is currently unused

**Required Changes:**
- RX (Pin 0) will be used for MIDI input
- MIDI input circuit needed (opto-isolator + current limiting resistor)
- Standard MIDI IN hardware specification

### Software Architecture

#### MIDI Input Parser

```cpp
class MidiInputHandler {
public:
  void begin(uint8_t midiChannel);
  void poll();  // Called in main loop
  bool hasProgramChange();
  uint8_t getProgramChange();
  void clearProgramChange();
  
private:
  uint8_t channel;
  bool pcReceived;
  uint8_t pcValue;
  
  // MIDI parsing state
  enum MidiParserState {
    WAITING_FOR_STATUS,
    WAITING_FOR_DATA
  };
  MidiParserState state;
  uint8_t statusByte;
  
  void parseIncomingByte(uint8_t byte);
};
```

#### Integration with State Manager

```cpp
// In main.cpp loop():
midiInput.poll();
if (midiInput.hasProgramChange()) {
  uint8_t pc = midiInput.getProgramChange();
  midiInput.clearProgramChange();
  
  if (pc >= 1 && pc <= TOTAL_PRESETS) {
    // Switch to bank mode if not already
    if (state.currentMode != BANK_MODE) {
      state.currentMode = BANK_MODE;
      state.displayState = SHOWING_BANK;
    }
    
    // Calculate bank and preset (PC 1-128)
    uint8_t bank = (pc - 1) / PRESETS_PER_BANK + 1;      // Banks 1-32
    uint8_t preset = (pc - 1) % PRESETS_PER_BANK;        // Presets 0-3
    
    state.currentBank = bank;
    state.activePreset = preset;
    state.loadPreset(pc);
    relays.update(state.loopStates);
    
    // Flash the PC on display
    state.flashingPC = pc;
    state.displayState = FLASHING_PC;
    state.pcFlashStartTime = millis();
  }
}
```

#### MIDI Message Format

- **Program Change**: `0xCn pp` where:
  - `n` = MIDI channel (0-15)
  - `pp` = program number (0-127, but device interprets as 1-128)

#### Behavior Specifications

1. **Channel Filtering**: Only respond to PC messages on the configured MIDI channel
2. **Mode Switching**: Automatically switch to BANK_MODE when valid PC received
3. **Out of Range**: Ignore PC messages < 1 or > 128
4. **Display Feedback**: Flash the received PC number (same as when switch pressed)
5. **Relay Update**: Immediately apply the preset to relays
6. **LED Update**: Update status LEDs to reflect new preset

### Error Handling

- Invalid MIDI messages are silently ignored
- Parser automatically resynchronizes on invalid data
- Running status is NOT supported (full status byte required each time)

---

## CC Mode Design

### Overview

Add a third operating mode (CC_MODE) where the four footswitches send MIDI Control Change messages instead of switching loops. Each switch can send a configurable CC number with a configurable value when pressed.

### Mode Structure

```
Current Modes:
- MANUAL_MODE
- BANK_MODE
- EDIT_MODE

New:
- CC_MODE
- CC_CONFIG_MODE (for configuring CC parameters)
```

### CC Mode Behavior

When in CC_MODE:
- Pressing SW1-SW4 sends a MIDI CC message
- Each switch has its own:
  - CC number (0-127)
  - CC "on" value (1-127)
  - CC "off" value (always 0)
- Switches can be configured as:
  - **Momentary**: Sends "on" value when pressed, "off" value (0) when released
  - **Toggle**: Sends "on" value on first press, "off" value (0) on second press
- Display shows "CC" when in CC mode
- LEDs show which switches are currently "on"

### CC Configuration Mode

A special configuration mode for setting up CC parameters.

#### Entry/Exit

- **Enter**: From CC_MODE, hold SW2+SW3 for 2 seconds
- **Exit**: Hold SW2+SW3 for 2 seconds (saves and returns to CC_MODE)

#### Navigation

In CC_CONFIG_MODE:
- **SW1**: Navigate left (previous item)
- **SW4**: Navigate right (next item)
- **SW2**: Decrease value
- **SW3**: Increase value

#### Configuration Items

The display scrolls through these items:

```
1. CC Mode Enable/Disable
   Display: "CC On" or "CC Off"
   Default: OFF
   Note: When OFF, CC mode is hidden from mode cycling
   
2. SW1 CC Number
   Display: "CC1 nnn"
   Range: 0-127
   Default: 1
   
3. SW1 CC Value
   Display: "U1 nnn"
   Range: 1-127
   Default: 127
   
4. SW1 Behavior
   Display: "b1  tGL" (toggle) or "b1  non" (momentary)
   Default: Toggle
   
5. SW2 CC Number
   Display: "CC2 nnn"
   Range: 0-127
   Default: 2
   
6. SW2 CC Value
   Display: "U2 nnn"
   Range: 1-127
   Default: 127
   
7. SW2 Behavior
   Display: "b2  tGL" or "b2  non"
   Default: Toggle
   
8. SW3 CC Number
   Display: "CC3 nnn"
   Range: 0-127
   Default: 3
   
9. SW3 CC Value
   Display: "U3 nnn"
   Range: 1-127
   Default: 127
   
10. SW3 Behavior
    Display: "b3  tGL" or "b3  non"
    Default: Toggle
    
11. SW4 CC Number
    Display: "CC4 nnn"
    Range: 0-127
    Default: 4
    
12. SW4 CC Value
    Display: "U4 nnn"
    Range: 1-127
    Default: 127
    
13. SW4 Behavior
    Display: "b4  tGL" or "b4  non"
    Default: Toggle
```

Configuration items wrap around (after last item, goes back to first).

#### Display Format

- **7-segment limitations**: Can show characters and numbers
- **Position format**: Use first 3 digits for label, last 3 for value
- **Examples**:
  - "CC1  12" - SW1 CC number is 12
  - "U1  127" - SW1 value is 127
  - "b1  tGL" - SW1 behavior is toggle
  - "b1  non" - SW1 behavior is momentary
  - "CC Off " - CC mode is disabled

### MIDI CC Message Format

- **Control Change**: `0xBn cc vv` where:
  - `n` = MIDI channel (0-15)
  - `cc` = controller number (0-127)
  - `vv` = controller value (0-127)

### Toggle State Tracking

```cpp
struct CCSwitch {
  uint8_t ccNumber;      // 0-127
  uint8_t ccOnValue;     // 1-127
  bool isToggle;         // true = toggle, false = momentary
  bool currentState;     // true = on, false = off (for toggle mode)
};
```

---

## State Machine Updates

### New Mode Enum

```cpp
enum Mode {
  MANUAL_MODE,
  BANK_MODE,
  EDIT_MODE,
  CC_MODE,
  CC_CONFIG_MODE
};
```

### Mode Transitions

```
                    ┌──────────────┐
         ┌──────────│ MANUAL_MODE  │◄────────┐
         │          └──────────────┘         │
         │                 │                 │
         │                 │ SW2+SW3         │ SW2+SW3
         │                 │                 │
         │          ┌──────▼──────┐          │
         │          │  BANK_MODE  │──────────┘
         │          └──────┬──────┘
         │                 │
         │                 │ SW2+SW3 (2s hold, if preset active)
         │                 │
         │          ┌──────▼──────┐
         │          │ EDIT_MODE   │
         │          └──────┬──────┘
         │                 │
         │                 │ SW2+SW3 (2s hold) → saves
         │                 │
         │          ┌──────▼──────────┐
         │          │ SHOWING_SAVED   │
         │          └──────┬──────────┘
         │                 │
         │                 └──────────►BANK_MODE
         │
         │          ┌──────────────┐
         └──────────│   CC_MODE    │◄──────────┐
         │          └──────┬───────┘           │
         │                 │                   │
         │                 │ SW2+SW3 (2s hold) │
         │                 │                   │
         │          ┌──────▼────────────┐      │
         │          │  CC_CONFIG_MODE   │──────┘
         │          └───────────────────┘
         │                 │ SW2+SW3 (2s hold) → saves
         │
         └─────────────────┘ (cycles through modes)
```

### Mode Cycling

Currently: MANUAL_MODE ↔ BANK_MODE

With CC mode:
- If CC enabled: MANUAL_MODE → BANK_MODE → CC_MODE → MANUAL_MODE
- If CC disabled: MANUAL_MODE ↔ BANK_MODE (existing behavior)

Mode cycling triggered by **quick press** of SW2+SW3 (not long press).

---

## Display States

### New Display States

```cpp
enum DisplayState {
  SHOWING_MANUAL,
  SHOWING_BANK,
  FLASHING_PC,
  SHOWING_SAVED,
  EDIT_MODE_ANIMATED,
  SHOWING_CC,           // NEW: Shows "CC" in CC mode
  SHOWING_CC_CONFIG,    // NEW: Shows CC config item
  SHOWING_CC_SAVED      // NEW: Shows "SAVEd" after CC config
};
```

### Display Formats

#### SHOWING_CC
```
Display: "  CC    "
Purpose: Indicate CC mode is active
LEDs: Show which CC switches are currently "on"
```

#### SHOWING_CC_CONFIG
```
Display varies by config item:
- "CC On  " or "CC Off "
- "CC1  12" (CC number for switch 1)
- "U1  127" (CC value for switch 1)
- "b1  tGL" (behavior: toggle)
- "b1  non" (behavior: momentary)
- Similar for switches 2-4
```

---

## EEPROM Layout Changes

### Current Layout (1KB EEPROM)

```
Address  | Size | Content              | Notes
---------|------|----------------------|---------------------------
0x00     | 1    | (Reserved)           | Reserved for compatibility
0x01     | 1    | Init Flag (0x42)     | First boot detection
0x02     | 1    | Preset 1             | Bank 1, Switch 1
0x03     | 1    | Preset 2             | Bank 1, Switch 2
...      | ...  | ...                  | ...
0x81     | 1    | Preset 128           | Bank 32, Switch 4
0x82-    | 894  | Unused               | Available for future use
0x3FF    |      |                      |
```

### New Layout with CC Mode

```
Address  | Size | Content              | Notes
---------|------|----------------------|---------------------------
0x00     | 1    | (Reserved)           | Reserved for compatibility
0x01     | 1    | Init Flag (0x42)     | First boot detection
0x02     | 1    | Preset 1             | Bank 1, Switch 1
0x03     | 1    | Preset 2             | Bank 1, Switch 2
...      | ...  | ...                  | ...
0x81     | 1    | Preset 128           | Bank 32, Switch 4
0x82     | 1    | CC Mode Enabled      | 0x00=OFF, 0x01=ON
0x83     | 1    | SW1 CC Number        | 0-127
0x84     | 1    | SW1 CC Value         | 1-127
0x85     | 1    | SW1 Behavior         | 0x00=momentary, 0x01=toggle
0x86     | 1    | SW2 CC Number        | 0-127
0x87     | 1    | SW2 CC Value         | 1-127
0x88     | 1    | SW2 Behavior         | 0x00=momentary, 0x01=toggle
0x89     | 1    | SW3 CC Number        | 0-127
0x8A     | 1    | SW3 CC Value         | 1-127
0x8B     | 1    | SW3 Behavior         | 0x00=momentary, 0x01=toggle
0x8C     | 1    | SW4 CC Number        | 0-127
0x8D     | 1    | SW4 CC Value         | 1-127
0x8E     | 1    | SW4 Behavior         | 0x00=momentary, 0x01=toggle
0x8F-    | 880  | Unused               | Available for future use
0x3FF    |      |                      |
```

**Total CC Configuration**: 13 bytes  
**Remaining Available**: 880 bytes

**Note on Address 0x00**: This address remains reserved for backward compatibility. In earlier firmware versions, it stored the MIDI channel. Since MIDI channel is now hardware-configured via DIP switches (read at startup), this address is no longer used but is kept reserved to avoid potential conflicts if old EEPROM data is present. Future firmware could repurpose this address if needed.

### EEPROM Constants

```cpp
// Add to config.h
const uint8_t EEPROM_CC_ENABLED_ADDR = 0x82;
const uint8_t EEPROM_CC_CONFIG_START = 0x83;  // Start of CC switch configs
const uint8_t EEPROM_CC_CONFIG_SIZE = 3;      // Bytes per switch (number, value, behavior)

// Default values
const uint8_t DEFAULT_CC_ENABLED = 0;         // OFF by default
const uint8_t DEFAULT_CC_NUMBER_BASE = 1;     // CC 1, 2, 3, 4 for SW1-4
const uint8_t DEFAULT_CC_VALUE = 127;         // Full value
const uint8_t DEFAULT_CC_BEHAVIOR = 1;        // Toggle mode
```

---

## Hardware Requirements

### MIDI IN Circuit

Standard MIDI IN circuit required:

```
MIDI IN Jack (DIN-5)
Pin 5 (MIDI signal) ──┬── 220Ω ──┬── Opto-isolator LED anode
                      │          │
Pin 4 (MIDI signal) ──┘          │
                                 │
                                 └── Opto-isolator LED cathode ── GND
                                 
Opto-isolator collector ── +5V
Opto-isolator emitter ──── RX (Pin 0)
```

**Components:**
- Opto-isolator (e.g., 6N138, 6N139, or HCPL-2601)
- 220Ω resistor for MIDI current limiting
- 1kΩ pullup resistor on RX side (often internal to opto)
- Diode for protection (1N4148)

**Note:** MIDI IN and MIDI OUT can share the same MIDI channel configuration from DIP switches.

### No Additional Hardware for CC Mode

CC mode is purely software - uses existing:
- Footswitches (for CC triggering)
- MIDI OUT (for CC messages)
- Display (for configuration)
- LEDs (for state indication)

---

## Implementation Considerations

### Memory Impact

#### SRAM Usage Estimate

New global objects:
```cpp
MidiInputHandler midiInput;           // ~10 bytes
```

CC Mode state in StateManager:
```cpp
bool ccModeEnabled;                   // 1 byte
CCSwitch ccSwitches[4];               // 4 × 4 = 16 bytes
uint8_t ccConfigItem;                 // 1 byte
                                      // Total: ~18 bytes
```

**Total New SRAM**: ~28 bytes  
**Current Usage**: ~630 bytes  
**New Total**: ~658 bytes (32% of 2KB)  
**Remaining**: ~1366 bytes - Still plenty of headroom

#### Flash Usage Estimate

- MIDI Input Parser: ~500 bytes
- CC Mode Logic: ~800 bytes
- CC Config Mode: ~600 bytes
- Display Updates: ~400 bytes

**Total New Flash**: ~2300 bytes  
**ATmega328 Flash**: 32KB  
**Current Usage**: ~15-20KB estimated  
**New Total**: ~17-22KB (53-69% of available)  
**Remaining**: ~10-15KB - Acceptable

### Software Serial Considerations

If hardware UART is unavailable or conflicts arise:

**Option 1**: Use SoftwareSerial for MIDI IN
- Pros: Hardware UART stays dedicated to MIDI OUT
- Cons: Less reliable at 31250 baud, CPU overhead

**Option 2**: Use SoftwareSerial for MIDI OUT
- Pros: Hardware UART for reliable MIDI IN
- Cons: MIDI OUT timing less precise

**Recommendation**: Keep hardware UART for MIDI OUT (timing critical), use SoftwareSerial for MIDI IN if needed.

### Timing Analysis

#### Main Loop Additions

```
Operation             | Frequency    | Duration | Impact
----------------------|--------------|----------|--------
MIDI input poll       | ~10 KHz      | ~20 µs   | Low
MIDI parsing          | ~1 KHz       | ~10 µs   | Minimal
CC mode logic         | ~100 Hz      | ~5 µs    | Minimal
CC config updates     | ~10 Hz       | ~10 µs   | Minimal
----------------------|--------------|----------|--------
New main loop time    | Current      | ~650 µs  | 0.65% CPU
Still well within budget
```

### Development Phases

#### Phase 1: MIDI IN
1. Add hardware MIDI IN circuit
2. Implement MidiInputHandler class
3. Test MIDI parsing with known PC messages
4. Integrate with StateManager
5. Test preset loading from MIDI IN

#### Phase 2: CC Mode Basic
1. Add CC_MODE to mode enum
2. Implement basic CC sending on switch press
3. Add CC mode to display
4. Test MIDI CC output

#### Phase 3: CC Configuration
1. Add CC_CONFIG_MODE
2. Implement navigation and value adjustment
3. Add EEPROM save/load for CC config
4. Test all configuration items

#### Phase 4: CC Advanced Features
1. Implement toggle vs momentary behavior
2. Add CC state LEDs
3. Test CC mode enable/disable
4. Test mode cycling with CC mode

### Testing Strategy

#### MIDI IN Testing
- Send PC 1-128 from external MIDI controller
- Verify correct preset loading
- Test channel filtering (send on wrong channel)
- Test invalid PC numbers (0, 129+)
- Verify mode switching from MANUAL to BANK

#### CC Mode Testing
- Configure each switch with unique CC numbers
- Test momentary behavior (on press, off on release)
- Test toggle behavior (on/off on successive presses)
- Verify CC messages on MIDI monitor
- Test CC config save/load across power cycles

### Debug Mode Compatibility

When DEBUG_MODE is enabled:
- MIDI OUT is disabled (Serial used for debug)
- MIDI IN can still be tested with Serial input simulation
- CC configuration can be tested (no MIDI output)
- Use debug prints to verify MIDI parsing logic

---

## API Changes

### Config.h Additions

```cpp
// New modes
enum Mode {
  MANUAL_MODE,
  BANK_MODE,
  EDIT_MODE,
  CC_MODE,
  CC_CONFIG_MODE
};

// CC configuration constants
const uint8_t EEPROM_CC_ENABLED_ADDR = 0x82;
const uint8_t EEPROM_CC_CONFIG_START = 0x83;
const uint8_t EEPROM_CC_CONFIG_SIZE = 3;
const uint8_t DEFAULT_CC_ENABLED = 0;
const uint8_t DEFAULT_CC_NUMBER_BASE = 1;
const uint8_t DEFAULT_CC_VALUE = 127;
const uint8_t DEFAULT_CC_BEHAVIOR = 1;
const uint16_t CC_CONFIG_LONG_PRESS_MS = 2000;
```

### StateManager Additions

```cpp
class StateManager {
public:
  // ... existing members ...
  
  // CC Mode
  bool ccModeEnabled;
  struct CCSwitch {
    uint8_t ccNumber;
    uint8_t ccOnValue;
    bool isToggle;
    bool currentState;
  };
  CCSwitch ccSwitches[4];
  uint8_t ccConfigItem;
  
  // CC configuration
  void loadCCConfig();
  void saveCCConfig();
  void resetCCConfig();
  bool isCCModeEnabled();
};
```

### MidiHandler Additions

```cpp
// New functions in midi_handler.h/cpp
void sendMIDIControlChange(uint8_t controller, uint8_t value, uint8_t channel);
```

### New Class: MidiInputHandler

```cpp
class MidiInputHandler {
public:
  void begin(uint8_t midiChannel);
  void poll();
  bool hasProgramChange();
  uint8_t getProgramChange();
  void clearProgramChange();
  
private:
  uint8_t channel;
  bool pcReceived;
  uint8_t pcValue;
  enum MidiParserState {
    WAITING_FOR_STATUS,
    WAITING_FOR_DATA
  };
  MidiParserState state;
  uint8_t statusByte;
  void parseIncomingByte(uint8_t byte);
};
```

---

## Migration and Compatibility

### EEPROM Migration

When updating firmware with these changes:

1. Existing preset data (addresses 0x02-0x81) is preserved
2. New CC configuration area starts at 0x82 (previously unused)
3. No migration needed - new areas auto-initialize to defaults
4. Init flag at 0x01 remains unchanged

### Backward Compatibility

- Devices without MIDI IN hardware will simply ignore the feature
- CC mode defaults to DISABLED, preserving existing two-mode behavior
- Existing mode cycling (MANUAL ↔ BANK) unchanged when CC disabled
- All existing preset and bank functionality preserved

### User Experience

#### First Boot with New Firmware
1. Device boots normally
2. CC mode is disabled by default
3. Behavior identical to previous firmware
4. User must explicitly enable CC mode in CC_CONFIG_MODE

#### Enabling CC Mode
1. Cycle to CC_MODE (press SW2+SW3 briefly to cycle through modes)
2. Once in CC_MODE, enter CC_CONFIG_MODE by holding SW2+SW3 for 2 seconds
3. Navigate to "CC Off" item
3. Press SW3 to change to "CC On"
4. Hold SW2+SW3 to save
5. CC mode now appears in mode cycling

---

## Future Enhancements

### Possible Extensions

1. **MIDI IN Song Select**: Use Song Select messages to switch banks
2. **MIDI IN CC**: Respond to incoming CC to toggle loops
3. **MIDI Merge**: Combine MIDI IN and MIDI OUT (requires MIDI THRU circuit)
4. **CC Learn Mode**: Press switch while receiving CC to learn CC number
5. **CC Value Learning**: Learn both on and off values from incoming MIDI
6. **PC Recall in CC Mode**: Allow PC messages to switch between CC "scenes"
7. **CC Banks**: Multiple banks of CC configurations (like loop presets)

---

## Risks and Mitigation

### Risk 1: MIDI IN Reliability
**Concern**: Software parsing of MIDI at 31250 baud may drop messages  
**Mitigation**: Use hardware UART (Serial.read()) which has hardware buffer  
**Fallback**: If reliability issues arise, reduce main loop overhead

### Risk 2: Memory Constraints
**Concern**: Additional features may exceed SRAM limits  
**Mitigation**: Careful memory profiling, optimize data structures  
**Fallback**: Make CC mode optional via compile-time flag

### Risk 3: User Interface Complexity
**Concern**: CC configuration mode may be confusing  
**Mitigation**: Clear display labels, logical navigation flow  
**Fallback**: Provide detailed user manual, video tutorial

### Risk 4: MIDI Channel Conflicts
**Concern**: Incoming PC and outgoing CC on same channel may cause loops  
**Mitigation**: Document proper MIDI routing, allow channel separation  
**Fallback**: Add option to disable MIDI IN when in CC mode

---

## Success Criteria

### MIDI IN Success Criteria
- [ ] Receive PC messages on configured MIDI channel
- [ ] Ignore PC messages on other channels
- [ ] Load correct preset for PC 1-128
- [ ] Display flashes PC number when received
- [ ] Auto-switch to BANK_MODE on PC receive
- [ ] No false triggers from MIDI noise

### CC Mode Success Criteria
- [ ] Send correct CC number and value on switch press
- [ ] Momentary mode: send "on" on press, "off" on release
- [ ] Toggle mode: alternate between "on" and "off" values
- [ ] LED shows correct CC switch states
- [ ] Display shows "CC" when in CC mode
- [ ] Mode cycling includes CC mode when enabled

### CC Configuration Success Criteria
- [ ] Navigate through all 13 configuration items
- [ ] Adjust values using SW2/SW3 (up/down)
- [ ] Save configuration to EEPROM
- [ ] Load configuration on boot
- [ ] CC mode defaults to OFF
- [ ] When OFF, CC mode hidden from cycling

---

## Conclusion

This design provides a comprehensive approach to adding MIDI IN and CC Mode functionality to the loop switcher while:
- Preserving existing functionality
- Maintaining backward compatibility
- Staying within hardware constraints
- Providing clear user interface
- Following established code patterns

The modular design allows for phased implementation and testing, reducing risk and allowing for iterative refinement based on user feedback.

