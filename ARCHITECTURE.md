# Architecture Overview

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         HARDWARE LAYER                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Switches (4)    Relays (4)    MAX7219      74HC595    Serial  │
│      │               │           │             │          │     │
│   (INPUT)        (OUTPUT)    (SPI-like)   (Shift Reg)  (MIDI)  │
└──────┬───────────────┬────────────┬────────────┬─────────┬─────┘
       │               │            │            │         │
┌──────▼───────────────▼────────────▼────────────▼─────────▼─────┐
│                    HARDWARE ABSTRACTION                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  SwitchHandler  RelayController  Display  LedController  MIDI   │
│                                                                 │
│  - Debouncing   - Relay control  - 7-seg  - Shift reg  - PC    │
│  - Long press   - All off        - Chars  - 8 LEDs     - Ch    │
│  - Simult.      - Update array   - Anim   - Polarity   - Baud  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## State Machine

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
         │                 │ SW2+SW3 (2s hold) → saves to EEPROM
         │                 │
         │          ┌──────▼──────────┐
         │          │ SHOWING_SAVED   │
         │          └──────┬──────────┘
         │                 │
         │                 │ (2s timeout)
         │                 │
         │          ┌──────▼──────┐
         │          │  BANK_MODE  │
         │          └──────┬──────┘
         │                 │
         │ SW1+SW4         │         Exit
         │ (long press)    │         SW2+SW3
         │          ┌──────▼──────────┐
         └──────────│ CHANNEL_SET_    │
                    │     MODE        │
                    └─────────────────┘
                           │
                           │ (5s timeout)
                           │
                    ┌──────▼──────┐
                    │  BANK_MODE  │
                    └─────────────┘
```

### Display States

```
MANUAL_MODE ────────► SHOWING_MANUAL
                      (Shows: "4_3_2_1_" with active loops)

BANK_MODE ──────────► SHOWING_BANK
                      (Shows: "bAnK 01" - "bAnK 32")
    │
    └─ Switch press ─► FLASHING_PC
                      (Shows: PC number for 1s)
                      └─► Returns to SHOWING_BANK

EDIT_MODE ──────────► EDIT_MODE_ANIMATED
                      (Shows: "Edit" with scrolling dot)
    │
    └─ Exit ─────────► SHOWING_SAVED
                      (Shows: "SAvEd" for 2s)
                      └─► Returns to SHOWING_BANK

CHANNEL_SET_MODE ───► SHOWING_CHANNEL
                      (Shows: "Chan 01" - "Chan 16")
```

---

## Memory Map

### EEPROM Layout (ATmega328 - 1KB available)

```
Address  | Size | Content              | Notes
---------|------|----------------------|---------------------------
0x00     | 1    | MIDI Channel (1-16)  | Validated on read
0x01     | 1    | Init Flag (0x42)     | First boot detection
0x02     | 1    | Preset 1             | Bank 1, Switch 1
0x03     | 1    | Preset 2             | Bank 1, Switch 2
0x04     | 1    | Preset 3             | Bank 1, Switch 3
0x05     | 1    | Preset 4             | Bank 1, Switch 4
...      | ...  | ...                  | ...
0x81     | 1    | Preset 128           | Bank 32, Switch 4
0x82-    |      | Unused (894 bytes)   | Available for future use
0x3FF    |      |                      |

Preset Byte Format:
  Bit 0: Loop 1 state (1=on, 0=off)
  Bit 1: Loop 2 state
  Bit 2: Loop 3 state
  Bit 3: Loop 4 state
  Bits 4-7: Unused (reserved for future)
```

### SRAM Usage (ATmega328 - 2KB available)

```
Category          | Estimated Size | Notes
------------------|----------------|----------------------------------
Global Objects    | ~100 bytes     | Hardware controllers
StateManager      | ~40 bytes      | State variables + arrays
Switch States     | ~40 bytes      | 4 × SwitchState structs
Stack             | ~200 bytes     | Function calls (no recursion)
Arduino Core      | ~200 bytes     | Serial buffers, etc.
LedControl Lib    | ~50 bytes      | Display driver state
------------------|----------------|----------------------------------
Total Used        | ~630 bytes     | ~31% of available SRAM
Available         | ~1400 bytes    | Plenty of headroom
```

---

## Timing Characteristics

### Critical Timing Paths

```
Switch Press Detection:
  Physical press → Debounce (30ms) → Detection → Action
  Total: ~30-40ms (acceptable for human input)

MIDI Output:
  Decision → sendMIDI() → Serial.write() → TX
  Total: <1ms (excellent for MIDI)

Relay Switching:
  Command → digitalWrite() → Relay coil energize
  Total: <1ms software + ~5-10ms relay mechanical

Display Update:
  State change → update() → SPI-like transfer → Display
  Total: <10ms (fast enough to appear instant)

Main Loop Frequency:
  Current: Runs as fast as possible (~1000-10000 Hz)
  Recommendation: Throttle to 100Hz for power efficiency
```

---

## Switch Pattern Detection

### Combination Detection Window

```
Time →
    
SW1 pressed ───┐                    
               │◄────100ms───►│      Simultaneous window
SW2 pressed ────────┐         │      Both detected as combo
                    │         │      if within window
                ┌───▼─────────▼────┐
                │  Pattern Check   │
                │  SW1 + SW2?     │
                └──────────────────┘
```

### Long Press Detection

```
Time →

SW1 pressed ──┐
              │
              │◄────1000ms (LONG_PRESS_MS)────►│
              │                                 │
              │                            Trigger
              │                            longPress
              │                            (once)
              │
              └──────────────────────────────┐
                                             │
                                        SW1 released
```

---

## Hardware Connections Summary

```
ATmega328 Pin Map:
                    ┌────────┐
                  ┌─│ RESET  │─┐
                  │ └────────┘ │
           RX ────┤ D0      D13├──── MAX_CLK (MOSI conflict noted!)
           TX ────┤ D1      D12├──── MAX_CS
           SW1────┤ D2      D11├──── MAX_DIN
                  │ D3      D10├──── RELAY4
           SW2────┤ D4       D9├──── RELAY3
           SW3────┤ D5       D8├──── RELAY2
           SW4────┤ D6       D7├──── RELAY1
                  │            │
     SR_DATA──────┤ A0       A5│
     SR_CLK───────┤ A1       A4│
     SR_LATCH─────┤ A2       A3│
                  │            │
                  └────────────┘

Notes:
- D0/D1 (RX/TX) used for MIDI out (TX only)
- A0-A2 used as digital outputs for shift register
- All switches use internal pullups (active LOW)
- D13 shared with built-in LED (CONFLICT - see review!)
```

---

## Key Algorithms

### Debouncing Algorithm

```cpp
Time-based Debounce (Current Implementation):

                 lastDebounceTime
                       │
    ┌──────────────────▼──────────────┐
    │  Reading changed from last?     │
    │  Yes → Reset timer              │
    │  No  → Check if stable          │
    └─────────┬───────────────────────┘
              │
    ┌─────────▼──────────────────────┐
    │  Stable for > DEBOUNCE_MS?     │
    │  Yes → Accept new state        │
    │  No  → Ignore, keep checking   │
    └────────────────────────────────┘

Benefits: Simple, works well for most buttons
Drawback: Rapid noise can keep resetting timer
```

---

## Performance Profile

```
Operation             | Frequency    | Duration | Impact
----------------------|--------------|----------|--------
Switch read & debounce| ~10 KHz      | ~10 µs   | Low
Pattern detection     | ~10 KHz      | ~20 µs   | Low
State updates         | ~100 Hz      | ~5 µs    | Minimal
Relay write          | ~10 Hz       | ~1 µs    | Minimal
LED shift out        | ~100 Hz      | ~50 µs   | Low
Display update       | ~100 Hz      | ~500 µs  | Moderate
EEPROM write         | ~0.01 Hz     | ~3.3 ms  | Negligible
----------------------|--------------|----------|--------
Main loop time       | Current      | ~600 µs  | 0.6% CPU
Main loop capacity   | @ 1 KHz      | 60%      | Plenty
```

---

## Future Architecture Considerations

### Possible Enhancements

1. **MIDI Input**
   - Add MIDI input parser
   - Respond to PC messages to change presets
   - Requires SoftwareSerial or second UART

2. **Preset Names**
   - Store 8-character names in EEPROM
   - Display on 7-segment when selected
   - Requires 128 × 8 = 1024 bytes (all remaining EEPROM)

3. **USB MIDI**
   - Replace Serial MIDI with USB MIDI
   - Requires ATmega32U4 (Leonardo/Micro)
   - Uses MIDIUSB library

4. **Expression Pedal**
   - Add analog input for expression pedal
   - Send MIDI CC messages
   - Requires ADC and continuous MIDI transmission

---

This architecture document complements the code review by showing the relationships between modules and data flow through the system.
