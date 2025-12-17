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
// Ignore MIDI PC messages if in global configuration mode
if (state.currentMode != GLOBAL_CONFIG_MODE) {
  midiInput.poll();
  if (midiInput.hasProgramChange()) {
    uint8_t pc = midiInput.getProgramChange();
    midiInput.clearProgramChange();
    
    if (pc >= 1 && pc <= TOTAL_PRESETS) {
    // Stay in current mode - do NOT auto-switch to BANK_MODE
    // Just load the preset and update display
    
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
}
```

**Mode Context Handling**: When PC is received in any mode (MANUAL, BANK, CC), the bank and preset context variables are updated but the mode is not changed. This allows:
- In MANUAL_MODE: Preset loaded, loops update, but stay in manual control
- In BANK_MODE: Preset loaded and displayed (natural behavior)
- In CC_MODE: Preset loaded, loops update, and CC toggle states restored if saved

#### MIDI Message Format

- **Program Change**: `0xCn pp` where:
  - `n` = MIDI channel (0-15)
  - `pp` = program number (0-127, but device interprets as 1-128)

#### Behavior Specifications

1. **Channel Filtering**: Only respond to PC messages on the configured MIDI channel
2. **Mode Preservation**: Stay in the current mode when PC received (do NOT auto-switch modes)
3. **Global Config Protection**: Skip MIDI polling entirely when in GLOBAL_CONFIG_MODE to avoid processing overhead
4. **Context Updates**: Update bank and preset context in all modes (allows preset recall in MANUAL or CC modes)
5. **Out of Range**: Ignore PC messages < 1 or > 128
6. **Display Feedback**: Flash the received PC number (same as when switch pressed)
7. **Relay Update**: Immediately apply the preset to relays
8. **LED Update**: Update status LEDs to reflect new preset
9. **CC State Restore**: In CC_MODE, also restore CC toggle states if saved for the preset

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
- EDIT_MODE (for editing loop presets)

New:
- CC_MODE (for sending CC messages)
- EDIT_MODE (enhanced - now edits both loops AND CC toggle states)
- GLOBAL_CONFIG_MODE (for all global configuration)
```

**Important Changes**:
- **EDIT_MODE Enhancement**: EDIT_MODE is now unified to handle both loop states and CC toggle states for a preset. Quick press of SW2+SW3 toggles between editing loops and editing CCs. This eliminates the need for a separate CC_EDIT_MODE.
- **GLOBAL_CONFIG_MODE**: A global configuration interface for system-wide settings including MIDI output channel, CC parameters, and other future configuration options. Since CC mode can be hidden from normal mode cycling, all CC configuration must be accessible through this global mode.

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

### Unified Edit Mode

EDIT_MODE is enhanced to allow editing both loop states and CC toggle states for a preset. This consolidates what were previously two separate edit modes into a single, unified editing experience.

**Concept**: Each preset stores both loop states (which loops are on/off) and CC toggle states (which CC switches are on/off in toggle mode). When editing a preset, you can configure both aspects before saving.

#### Entry/Exit

- **Enter**: From BANK_MODE or CC_MODE (after selecting a preset via footswitch or MIDI PC), hold SW2+SW3 for 2 seconds
- **Exit**: Hold SW2+SW3 for 2 seconds (saves both loop and CC states to preset and returns to previous mode)
- **Important**: Can only enter after a specific preset is active (i.e., when `state.activePreset >= 0` and `!state.globalPresetActive`)

#### Toggle Between Loop and CC Editing

- **SW2+SW3 (quick press)**: Toggle between editing loops and editing CC switches
- Display changes to indicate which is being edited:
  - "Edit LoP" (editing loops) or "Edit  CC" (editing CC switches)
  - Scrolling dot animation continues in both modes

#### Behavior

**When editing loops** (default when entering from BANK_MODE):
- Press SW1-SW4 to toggle individual loop states
- Relays update in real-time to hear changes
- LEDs show current loop states

**When editing CC switches** (default when entering from CC_MODE):
- Press SW1-SW4 to toggle CC switch states (only affects toggle-mode switches)
- LEDs show current CC toggle states
- No MIDI messages sent during editing (only when preset is recalled)

**On exit**:
- Both loop states and CC states are saved to the preset
- When the preset is recalled (via footswitch or MIDI PC), both loop and CC states are restored

#### Use Case Example

1. Select preset 1 (Bank 1, Switch 1) in BANK_MODE
2. Enter EDIT_MODE (hold SW2+SW3 for 2s)
3. Edit loops: Set Loop1 ON, Loop2 OFF, Loop3 ON, Loop4 OFF
4. Press SW2+SW3 (quick) to switch to CC editing
5. Edit CCs: Set CC1 ON, CC2 OFF, CC3 ON, CC4 OFF
6. Hold SW2+SW3 for 2s to save and exit
7. Later, when preset 1 is recalled, both loop and CC states are restored

### Global Configuration Mode

GLOBAL_CONFIG_MODE is a system-wide configuration interface for all global settings, including CC parameters.

#### Entry/Exit

- **Enter**: From any mode (except during other long-press operations), hold SW1+SW4 for 2 seconds
- **Exit**: Hold SW1+SW4 for 2 seconds (saves and returns to previous mode)

**Important Implementation Note**: When entering GLOBAL_CONFIG_MODE from an editing mode (EDIT_MODE or CC_EDIT_MODE), the `previousMode` variable must be set to the parent mode (BANK_MODE or CC_MODE), not the edit mode itself. This prevents returning to a potentially invalid edit state after exiting global configuration.

#### Navigation

In GLOBAL_CONFIG_MODE:
- **SW1**: Navigate left (previous item)
- **SW4**: Navigate right (next item)
- **SW2**: Decrease value
- **SW3**: Increase value

#### Configuration Items

The display scrolls through these items:

```
1. MIDI Out Channel
   Display: "Out  CH" followed by "Ch   nn"
   Range: 1-16 (MIDI channels)
   Default: 1 (or last hardware-configured channel)
   Note: Replaces hardware DIP switch configuration with software setting
   
2. CC Mode Enable/Disable
   Display: "CC On  " or "CC Off "
   Default: OFF
   Note: When OFF, CC mode is hidden from mode cycling
   
3. SW1 MIDI Channel
   Display: "C1  CH" followed by "Ch   nn"
   Range: 1-16
   Default: 1
   
4. SW1 CC Number
   Display: "CC1 nnn"
   Range: 0-127
   Default: 1
   
5. SW1 CC Value
   Display: "U1  nnn"
   Range: 1-127
   Default: 127
   
6. SW1 Behavior
   Display: "b1  tGL" (toggle) or "b1  non" (momentary)
   Default: Toggle
   
7. SW2 MIDI Channel
   Display: "C2  CH" followed by "Ch   nn"
   Range: 1-16
   Default: 1
   
8. SW2 CC Number
   Display: "CC2 nnn"
   Range: 0-127
   Default: 2
   
9. SW2 CC Value
   Display: "U2  nnn"
   Range: 1-127
   Default: 127
   
10. SW2 Behavior
    Display: "b2  tGL" or "b2  non"
    Default: Toggle
    
11. SW3 MIDI Channel
    Display: "C3  CH" followed by "Ch   nn"
    Range: 1-16
    Default: 1
    
12. SW3 CC Number
    Display: "CC3 nnn"
    Range: 0-127
    Default: 3
    
13. SW3 CC Value
    Display: "U3  nnn"
    Range: 1-127
    Default: 127
    
14. SW3 Behavior
    Display: "b3  tGL" or "b3  non"
    Default: Toggle
    
15. SW4 MIDI Channel
    Display: "C4  CH" followed by "Ch   nn"
    Range: 1-16
    Default: 1
    
16. SW4 CC Number
    Display: "CC4 nnn"
    Range: 0-127
    Default: 4
    
17. SW4 CC Value
    Display: "U4  nnn"
    Range: 1-127
    Default: 127
    
18. SW4 Behavior
    Display: "b4  tGL" or "b4  non"
    Default: Toggle
```

**Navigation Note**: Configuration items do NOT wrap around. When you reach the last item (SW4 Behavior), pressing SW4 does nothing. You must press SW1 to navigate back through the items.

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
  uint8_t midiChannel;   // 0-15 (MIDI channels 1-16)
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
  EDIT_MODE,        // Now handles both loops and CC editing
  CC_MODE,
  GLOBAL_CONFIG_MODE
};
```

### Mode Transitions

```
    ┌────────────────────────────────────────────────────────────┐
    │                  GLOBAL_CONFIG_MODE                        │
    │          (SW1+SW4 2s hold from any mode)                   │
    │                    SW1+SW4 2s → save & exit                │
    └────────────────────────────────────────────────────────────┘
                              ▲
                              │ SW1+SW4 (2s hold)
                              │
                    ┌──────────────┐
         ┌──────────│ MANUAL_MODE  │◄────────┐
         │          └──────────────┘         │
         │                 │                 │
         │                 │ SW2+SW3         │ SW2+SW3
         │                 │                 │
         │          ┌──────▼──────┐          │
         │          │  BANK_MODE  │──────────┼──────┐
         │          └──────┬──────┘          │      │
         │                 │                 │      │
         │                 │ SW2+SW3 (2s hold, if preset active)
         │                 │                 │      │
         │          ┌──────▼──────┐          │      │
         │          │ EDIT_MODE   │◄─────────┼──────┘
         │          │             │          │  SW2+SW3 (2s hold,
         │          │ SW2+SW3     │          │   if preset active)
         │          │ (quick) to  │          │
         │          │ toggle Loop │          │
         │          │ vs CC edit  │          │
         │          └──────┬──────┘          │
         │                 │                 │
         │                 │ SW2+SW3 (2s hold) → saves both loop & CC
         │                 │                 │
         │          ┌──────▼──────────┐      │
         │          │ SHOWING_SAVED   │      │
         │          └──────┬──────────┘      │
         │                 │                 │
         │                 └──────────►returns to BANK or CC_MODE
         │
         │          ┌──────────────┐
         └──────────│   CC_MODE    │
                    └──────────────┘
```

**Key Points**:
- GLOBAL_CONFIG_MODE is accessible from any mode via SW1+SW4 2-second hold
- EDIT_MODE is now unified - accessible from both BANK_MODE and CC_MODE
- Within EDIT_MODE, quick press SW2+SW3 to toggle between editing loops and editing CCs
- On exit, both loop and CC states are saved to the preset
- Mode cycling (SW2+SW3 quick press): MANUAL_MODE ↔ BANK_MODE ↔ CC_MODE (if enabled)

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
  EDIT_MODE_ANIMATED,      // Enhanced: Shows "Edit LoP" or "Edit  CC"
  SHOWING_CC,              // NEW: Shows "CC" in CC mode
  SHOWING_GLOBAL_CONFIG,   // NEW: Shows global config item
  SHOWING_CONFIG_SAVED     // NEW: Shows "SAVEd" after global config
};
```

### Display Formats

#### SHOWING_CC
```
Display: "  CC    "
Purpose: Indicate CC mode is active
LEDs: Show which CC switches are currently "on"
```

#### EDIT_MODE_ANIMATED (Enhanced)
```
Display: "Edit LoP" (when editing loops) or "Edit  CC" (when editing CC switches)
Purpose: Indicate edit mode is active and what is being edited
Animation: Scrolling dot continues in both modes
LEDs: Show current loop states (when editing loops) or CC toggle states (when editing CCs)
Toggling: Quick press SW2+SW3 switches between "Edit LoP" and "Edit  CC"
```

#### SHOWING_GLOBAL_CONFIG
```
Display varies by config item:
- "CC On  " or "CC Off " (CC mode enable/disable)
- "CC1  12" (CC number for switch 1)
- "U1  127" (CC value for switch 1)
- "b1  tGL" (behavior: toggle)
- "b1  non" (behavior: momentary)
- Similar for switches 2-4
- Future: Other global settings
```

#### SHOWING_CONFIG_SAVED
```
Display: "SAVEd"
Purpose: Confirm global configuration has been saved
Duration: 2 seconds before returning to previous mode
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
0x00     | 1    | MIDI Out Channel     | 0-15 (MIDI channels 1-16)
0x01     | 1    | Init Flag (0x42)     | First boot detection
0x02     | 1    | Preset 1             | Bank 1, Switch 1
0x03     | 1    | Preset 2             | Bank 1, Switch 2
...      | ...  | ...                  | ...
0x81     | 1    | Preset 128           | Bank 32, Switch 4
0x82     | 1    | CC Mode Enabled      | 0x00=OFF, 0x01=ON
0x83     | 1    | SW1 MIDI Channel     | 0-15 (MIDI channels 1-16)
0x84     | 1    | SW1 CC Number        | 0-127
0x85     | 1    | SW1 CC Value         | 1-127
0x86     | 1    | SW1 Behavior         | 0x00=momentary, 0x01=toggle
0x87     | 1    | SW2 MIDI Channel     | 0-15 (MIDI channels 1-16)
0x88     | 1    | SW2 CC Number        | 0-127
0x89     | 1    | SW2 CC Value         | 1-127
0x8A     | 1    | SW2 Behavior         | 0x00=momentary, 0x01=toggle
0x8B     | 1    | SW3 MIDI Channel     | 0-15 (MIDI channels 1-16)
0x8C     | 1    | SW3 CC Number        | 0-127
0x8D     | 1    | SW3 CC Value         | 1-127
0x8E     | 1    | SW3 Behavior         | 0x00=momentary, 0x01=toggle
0x8F     | 1    | SW4 MIDI Channel     | 0-15 (MIDI channels 1-16)
0x90     | 1    | SW4 CC Number        | 0-127
0x91     | 1    | SW4 CC Value         | 1-127
0x92     | 1    | SW4 Behavior         | 0x00=momentary, 0x01=toggle
0x93     | 1    | CC Preset 1          | CC toggle states for preset 1
0x94     | 1    | CC Preset 2          | CC toggle states for preset 2
...      | ...  | ...                  | ...
0x112    | 1    | CC Preset 128        | CC toggle states for preset 128
0x113-   | 748  | Unused               | Available for future use
0x3FF    |      |                      |

CC Preset Byte Format (same as loop preset format):
  Bit 0: SW1 CC toggle state (1=on, 0=off)
  Bit 1: SW2 CC toggle state
  Bit 2: SW3 CC toggle state
  Bit 3: SW4 CC toggle state
  Bits 4-7: Unused (reserved for future)
```

**Total Global Configuration**: 18 bytes
- MIDI Out Channel: 1 byte (at 0x00)
- CC Mode Enabled: 1 byte (at 0x82)
- CC Switch Configs: 16 bytes (4 bytes × 4 switches at 0x83-0x92)

**Total CC Preset States**: 128 bytes (1 byte per preset at 0x93-0x112)
**Total New EEPROM Usage**: 146 bytes (18 config + 128 preset states)
**Remaining Available**: 748 bytes (0x113-0x3FF)

**Note on Address 0x00**: This address is now repurposed to store the MIDI Out Channel (software configuration), replacing the hardware DIP switch method. This provides more flexibility and allows users to change the MIDI channel without opening the device.

### EEPROM Constants

```cpp
// Add to config.h
const uint8_t EEPROM_MIDI_OUT_CHANNEL_ADDR = 0x00; // MIDI out channel (replaces reserved addr)
const uint8_t EEPROM_CC_ENABLED_ADDR = 0x82;
const uint8_t EEPROM_CC_CONFIG_START = 0x83;       // Start of CC switch configs
const uint8_t EEPROM_CC_CONFIG_SIZE = 4;           // Bytes per switch (channel, number, value, behavior)
const uint8_t EEPROM_CC_PRESETS_START = 0x93;      // Start of CC preset states (0x93)
const uint16_t EEPROM_CC_PRESETS_END = 0x112;      // End of CC preset states (0x93 + 127 = 0x112)

// Default values
const uint8_t DEFAULT_MIDI_OUT_CHANNEL = 0;        // MIDI channel 0 (displayed as 1)
const uint8_t DEFAULT_CC_ENABLED = 0;              // OFF by default
const uint8_t DEFAULT_CC_CHANNEL = 0;              // MIDI channel 0 (displayed as 1)
const uint8_t DEFAULT_CC_NUMBER_BASE = 1;          // CC 1, 2, 3, 4 for SW1-4
const uint8_t DEFAULT_CC_VALUE = 127;              // Full value
const uint8_t DEFAULT_CC_BEHAVIOR = 1;             // Toggle mode
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

#### Phase 3: Global Configuration Mode
1. Add GLOBAL_CONFIG_MODE
2. Implement SW1+SW4 entry/exit (2s hold)
3. Implement navigation and value adjustment
4. Add EEPROM save/load for global config
5. Test all configuration items

#### Phase 4: CC Edit Mode
1. Add CC_EDIT_MODE (similar to EDIT_MODE)
2. Implement CC toggle state saving for presets
3. Add EEPROM storage for CC preset states
4. Test preset recall with CC states

#### Phase 5: CC Advanced Features
1. Implement toggle vs momentary behavior
2. Add CC state LEDs
3. Test CC mode enable/disable via global config
4. Test mode cycling with CC mode

### Testing Strategy

#### MIDI IN Testing
- Send PC 1-128 from external MIDI controller
- Verify correct preset loading
- Test channel filtering (send on wrong channel)
- Test invalid PC numbers (0, 129+)
- Verify mode preservation (stay in current mode, don't auto-switch)
- Verify PC messages ignored when in GLOBAL_CONFIG_MODE

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
  EDIT_MODE,           // Enhanced: now edits both loops and CCs
  CC_MODE,
  GLOBAL_CONFIG_MODE
};

// MIDI and CC configuration constants (from EEPROM section above)
const uint8_t EEPROM_MIDI_OUT_CHANNEL_ADDR = 0x00;
const uint8_t EEPROM_CC_ENABLED_ADDR = 0x82;
const uint8_t EEPROM_CC_CONFIG_START = 0x83;
const uint8_t EEPROM_CC_CONFIG_SIZE = 4;        // Now 4 bytes per switch
const uint8_t EEPROM_CC_PRESETS_START = 0x93;
const uint16_t EEPROM_CC_PRESETS_END = 0x112;
const uint8_t DEFAULT_MIDI_OUT_CHANNEL = 0;
const uint8_t DEFAULT_CC_ENABLED = 0;
const uint8_t DEFAULT_CC_CHANNEL = 0;
const uint8_t DEFAULT_CC_NUMBER_BASE = 1;
const uint8_t DEFAULT_CC_VALUE = 127;
const uint8_t DEFAULT_CC_BEHAVIOR = 1;
const uint16_t EDIT_MODE_TOGGLE_DEBOUNCE_MS = 300;  // Debounce for SW2+SW3 quick press
const uint16_t GLOBAL_CONFIG_LONG_PRESS_MS = 2000;
```

### StateManager Additions

```cpp
class StateManager {
public:
  // ... existing members ...
  
  // MIDI configuration
  uint8_t midiOutChannel;  // 0-15 (MIDI channels 1-16), software-configurable
  
  // CC Mode
  bool ccModeEnabled;
  struct CCSwitch {
    uint8_t midiChannel;   // 0-15 (MIDI channels 1-16)
    uint8_t ccNumber;
    uint8_t ccOnValue;
    bool isToggle;
    bool currentState;
  };
  CCSwitch ccSwitches[4];
  
  // Global configuration
  uint8_t globalConfigItem;
  Mode previousMode;  // For returning from GLOBAL_CONFIG_MODE
                      // CRITICAL: If entering GLOBAL_CONFIG_MODE from EDIT_MODE,
                      // previousMode MUST be set to the parent mode (BANK_MODE or CC_MODE)
                      // not EDIT_MODE itself, to prevent returning to invalid edit state
  
  // Unified Edit Mode
  bool editingLoops;  // true = editing loops, false = editing CCs
  bool editModeLoopStates[4];  // Editing loop states for preset
  bool editModeCCStates[4];    // Editing CC toggle states for preset
  
  // Configuration methods
  void loadMidiOutChannel();
  void saveMidiOutChannel();
  void loadCCConfig();
  void saveCCConfig();
  void resetCCConfig();
  bool isCCModeEnabled();
  
  // Preset state methods (now save both loops and CCs)
  void savePreset(uint8_t presetNumber);  // Saves both loop and CC states
  void loadPreset(uint8_t presetNumber);  // Loads both loop and CC states
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

1. **Address 0x00**: Now stores MIDI Out Channel (was reserved/unused)
   - On first boot, read hardware DIP switches one last time and store to 0x00
   - Subsequent boots read from EEPROM, allowing software configuration
   - DIP switches can be removed from hardware after migration
2. Existing preset data (addresses 0x02-0x81) is preserved
3. New CC configuration area starts at 0x82 (previously unused)
4. No migration needed for CC areas - auto-initialize to defaults
5. Init flag at 0x01 remains unchanged

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
4. User must explicitly enable CC mode in GLOBAL_CONFIG_MODE

#### Enabling CC Mode
1. From any mode, hold SW1+SW4 for 2 seconds to enter GLOBAL_CONFIG_MODE
2. Navigate to "CC Off" item using SW1/SW4
3. Press SW3 to change to "CC On"
4. Hold SW1+SW4 for 2 seconds to save and exit
5. CC mode now appears in mode cycling (MANUAL ↔ BANK ↔ CC)

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

