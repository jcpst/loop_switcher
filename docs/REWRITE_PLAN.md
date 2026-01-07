# Firmware Rewrite Plan

**Created:** January 7, 2026  
**Status:** In Progress  
**Current Step:** Step 2 (Relay Validation) - Ready for hardware test

---

## Overview

Rewrite firmware from scratch using an incremental, hardware-validated approach. Each step is tested on real hardware before proceeding to the next. This replaces the previous over-abstracted codebase that was developed without hardware validation.

### Goals

- Validate each hardware layer before adding the next
- Keep code simple—no premature abstractions
- Test on hardware at every step
- Build confidence in each subsystem before integration

### Design Decisions

- **4-switch UX retained**: Long-press (≥2s) of SW2+SW3 for Edit Mode entry/exit
- **MIDI channel**: Default to channel 1; read footswitches during `setup()` to override (4-bit binary, 0b0000 = channel 1)
- **74HC595 LEDs**: Deferred until core functionality is stable
- **Long press vs. short press**: Track `sw2sw3HoldStart` timestamp when both pressed; evaluate on *release*—short (<2s) toggles mode, long (≥2s) triggers Edit entry/exit

---

## Steps

### Step 1: Archive and Skeleton ✅ COMPLETE

**Goal:** Clean slate with minimal compilable code.

**Actions:**
- [x] Move existing `src/*.cpp` and `src/*.h` (except `config.h`) to `src_archive/`
- [x] Create minimal `main.cpp` with pin definitions and basic setup

**Verification:** Project compiles with PlatformIO.

---

### Step 2: Relay Hardware Validation ⬅️ CURRENT

**Goal:** Confirm all 4 relays click correctly.

**Implementation:**
- Cycle through each relay: ON 500ms → OFF 500ms
- Raw `digitalWrite()` only—no abstractions

**Verification:**
- [ ] Flash firmware: `pio run -t upload`
- [ ] All 4 relays click in sequence
- [ ] Each relay clicks once per cycle (no double-clicking)
- [ ] Timing feels like ~500ms per relay

**Code location:** `src/main.cpp` - current implementation

---

### Step 3: Footswitch Input with Debounce

**Goal:** Each switch toggles its corresponding relay.

**Implementation:**
- Add inline time-based debounce in `loop()`
- Track `lastDebounceTime` and `lastState` per switch
- On stable press, toggle corresponding relay state
- Remove the relay cycling code from Step 2

**Verification:**
- [ ] Each switch toggles its relay on/off
- [ ] No spurious toggles (debounce working)
- [ ] Responsive feel (<50ms perceived latency)

---

### Step 4: 7-Segment Display (MAX7219)

**Goal:** Display shows loop states and bank numbers.

**Implementation:**
- Use LedControl library directly in `main.cpp`
- Display static number first (e.g., "1")
- Then display loop states (show which loops are active)

**Verification:**
- [ ] Display powers on and shows digits
- [ ] All segments work (test with "8")
- [ ] Loop states display correctly when toggled

---

### Step 5: MIDI Output and Channel Config

**Goal:** Send MIDI Program Change messages.

**Implementation:**
- Initialize Serial at 31250 baud
- Create inline `sendProgramChange(uint8_t pc, uint8_t channel)` function
- In `setup()`: read footswitches as 4-bit value
  - If 0b0000 (all open), use channel 1
  - Otherwise, use binary value as channel (1–15)
- On switch press, send corresponding PC

**Verification:**
- [ ] MIDI monitor receives PC messages
- [ ] Correct PC numbers (1–4 in Manual Mode)
- [ ] Channel configuration works (hold switches during power-on)

**Note:** Manual Mode is complete after this step.

---

### Step 6: Bank Mode with State Machine

**Goal:** Navigate banks and send correct PC values.

**Implementation:**
- Add `Mode` enum: `MANUAL_MODE`, `BANK_MODE`
- Track `sw2sw3HoldStart` when both SW2+SW3 pressed
- On release:
  - If held < 2s → toggle Manual/Bank mode
  - If held ≥ 2s → (reserved for Edit Mode in Step 7)
- Bank navigation:
  - SW1+SW2 simultaneous: bank down
  - SW3+SW4 simultaneous: bank up
- PC calculation: `((bank - 1) * 4) + switch + 1`

**Verification:**
- [ ] SW2+SW3 short press toggles between Manual and Bank mode
- [ ] Display shows bank number in Bank Mode
- [ ] Bank up/down works (SW1+SW2, SW3+SW4)
- [ ] Correct PC sent: bank 1 switch 1 = PC 1, bank 2 switch 1 = PC 5, etc.

---

### Step 7: EEPROM Presets and Edit Mode

**Goal:** Save/load presets, edit loop states.

**Implementation:**
- EEPROM layout:
  - Address 0: Reserved
  - Address 1: Init flag (0x42)
  - Address 2–129: Presets 1–128 (4 bits each, packed)
- `savePreset()` with dirty-check (only write if changed)
- `loadPreset()` to restore loop states
- Edit Mode:
  - Enter: SW2+SW3 held ≥ 2s when preset active in Bank Mode
  - Toggle loops with individual switches
  - Relays update live to hear changes
  - Exit/save: SW2+SW3 held ≥ 2s again

**Verification:**
- [ ] Enter Edit Mode with 2s hold (display shows "Edit" or similar)
- [ ] Toggle loops in Edit Mode, relays respond
- [ ] Exit with 2s hold, preset saved
- [ ] Power cycle, reload preset, loop states match

---

### Step 8: Refactor into Modules (Optional)

**Goal:** Extract helpers only if code becomes unwieldy.

**Criteria for extraction:**
- File exceeds ~300 lines
- Clear logical separation exists
- Extraction improves readability

**Potential modules:**
- `switches.h/cpp` - debounce and pattern detection
- `display.h/cpp` - MAX7219 wrapper
- `midi.h/cpp` - MIDI output helpers
- `eeprom.h/cpp` - preset storage

**Note:** Only proceed if Steps 1–7 result in messy code. Simplicity is preferred.

---

## Deferred Features

These features are intentionally deferred until core functionality is stable:

- **74HC595 LED shift register**: Status LEDs for relay states and preset indicators
- **Config menu**: In-device configuration UI documented on GitHub
- **Display animations**: "Edit" scrolling, "Saved" flash, PC flash
- **Global preset (PC 128)**: Double-press same preset to activate all-loops-off
- **EEPROM checksum**: Data integrity validation

---

## File Structure

```
src/
  main.cpp      # All implementation (until Step 8)
  config.h      # Retained but simplified if needed

src_archive/    # Previous implementation for reference
  display.cpp
  display.h
  led_controller.cpp
  led_controller.h
  midi_handler.cpp
  midi_handler.h
  mode_controller.cpp
  mode_controller.h
  relays.cpp
  relays.h
  state_manager.cpp
  state_manager.h
  switches.cpp
  switches.h
```

---

## Testing Checklist

Use this checklist when validating each step:

### Hardware Test Procedure

1. Connect hardware (relays, switches, display, MIDI)
2. Flash firmware: `pio run -t upload`
3. Observe behavior matches expected for current step
4. Document any issues before proceeding

### MIDI Testing

- Use MIDI monitor software (e.g., MIDI Monitor on macOS, MIDI-OX on Windows)
- Verify channel and PC numbers match expected values
- Check for duplicate messages or missed messages

### EEPROM Testing

- Save preset, power cycle, verify restoration
- Test boundary presets (1, 128)
- Test after multiple save/load cycles

---

## References

- [ARCHITECTURE.md](ARCHITECTURE.md) - System design and hardware connections
- [Previous reviews](../reviews/) - Issues identified in original codebase
- [Hardware documentation](../hardware/) - Circuit schematics

