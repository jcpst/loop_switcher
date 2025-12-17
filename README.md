# 4-Loop MIDI Switcher with Bank Mode

A programmable guitar effects loop switcher with MIDI control, built on the ATmega328 platform (Arduino Uno/Nano compatible). Features 4 audio loops, MIDI program change output, bank selection, and a 7-segment display.

## Features

- **4 Audio Loops**: Independent DPDT relay switching for true bypass
- **Three Operating Modes**:
  - **Manual Mode**: Direct loop on/off control via footswitches
  - **Bank Mode**: 32 banks x 4 presets = 128 MIDI program changes
  - **Edit Mode**: Edit loop states for stored presets
- **MIDI Output**: Sends Program Change messages (PC 1-128) on hardware-configurable channel (1-16)
- **7-Segment Display**: Shows current bank or program change
- **Status LEDs**: 8 LEDs (4 relay state + 4 preset indicators) via 74HC595 shift register
- **Global Preset Mode**: Access preset 128 in any Bank
- **Hardware MIDI Channel Selection**: Set MIDI channel (1-16) using 4 DIP switches during power-up

## Hardware Requirements

### Components
- ATmega328-based board (Arduino Uno or Nano)
- 4x momentary footswitches (active LOW with pullups)
- 4x DIP switches (for MIDI channel selection, shares footswitch pins)
- MAX7219 7-segment display driver with 8-digit display
- 4x DPDT relays for audio switching
- 74HC595 shift register + 8x LEDs (status indicators)
- MIDI output circuit (hardware UART)

### Pin Configuration
See `src/config.h` for detailed pin assignments

### MIDI Channel Configuration
The MIDI output channel (1-16) is set using 4 DIP switches connected to the footswitch pins:
- **SW1 pin (D2)**: Bit 0 (LSB)
- **SW2 pin (D4)**: Bit 1
- **SW3 pin (D5)**: Bit 2
- **SW4 pin (D6)**: Bit 3 (MSB)

The DIP switches are read during power-up/reset only. The configured channel is displayed for 1 second during startup (shown as 1-16). The 4-bit binary value represents MIDI channels 0-15:
- `b0000` (all switches OFF) = MIDI Channel 0 (displayed as Channel 1)
- `b0001` (SW1 ON) = MIDI Channel 1 (displayed as Channel 2)
- `b0010` (SW2 ON) = MIDI Channel 2 (displayed as Channel 3)
- ...
- `b1111` (all switches ON) = MIDI Channel 15 (displayed as Channel 16)

**Wiring**: Each DIP switch should connect the footswitch pin to ground when ON. The internal pullup resistors ensure the pins read HIGH when switches are OFF.

**Important**: There is no conflict between DIP switches and footswitches because DIP switches are only read during setup, before the main loop begins processing footswitch presses.

### LED Status Indicators
- **4 Relay LEDs**: Show currently applied loop states (what's driving the relays)
- **4 Preset LEDs**: Show which preset is selected (OFF in Manual Mode or when Global Preset active)
- Configure `LED_ACTIVE_LOW` in `config.h` based on wiring (source vs sink current)

## Controls

### Switch Combinations

| Switches | Action | Mode |
|----------|--------|------|
| SW2 + SW3 | Toggle Manual/Bank mode | Any |
| SW2 (2s hold) + SW3 (2s hold) | Enter/Exit Edit Mode | Bank (after preset selected) |
| SW1 + SW2 | Bank down | Bank |
| SW3 + SW4 | Bank up | Bank |
| Single switch | Toggle loop / Send PC / Toggle loop in edit | Manual / Bank / Edit |

### Manual Mode
- Press individual switches to toggle loops on/off
- Display shows loop states via indicators

### Bank Mode
- Select bank (1-32) using SW1+SW2 (down) or SW3+SW4 (up)
- Press individual switch to send MIDI PC: `PC = (bank x 4) + switch + 1`
- Double-press same switch to activate Global Preset (all loops off)
- Display shows current bank number

### Edit Mode
- After selecting a preset in Bank Mode, hold SW2+SW3 for 2 seconds to enter
- Display flashes "Edit" (1 second on, 1 second off)
- Press individual switches to enable/bypass loops for the current preset
- Relays update in real-time to hear changes
- Hold SW2+SW3 for 2 seconds to save and exit
- Display shows "SAVEd" for 2 seconds, then returns to Bank Mode

## Display States

| Display | Meaning |
|---------|---------|
| Channel number (1-16) | MIDI channel during startup (1s) |
| Bank number (1-32) | Current bank in Bank Mode |
| Flashing PC number | Program Change being sent |
| Flashing "Edit" | Edit Mode active |
| "SAVEd" | Preset changes saved |
| Dot indicators | Individual loop states |
| Center dot | Global Preset active |

## Building and Uploading

### PlatformIO
```bash
# Build
pio run

# Upload
pio run --target upload

# Serial monitor
pio device monitor
```

### Debug Mode
Debug output can be enabled to help with development and troubleshooting. **WARNING**: Debug output uses the same hardware Serial port as MIDI TX, so they cannot be used simultaneously.

To enable debug mode:
1. Uncomment `#define DEBUG_MODE` in `src/config.h`, OR
2. Build with the `uno_debug` environment: `pio run -e uno_debug`

Debug output includes:
- MIDI initialization and program changes
- Mode transitions (Manual ↔ Bank ↔ Edit)
- Bank changes
- Preset loading and saving
- MIDI channel configuration

Only enable debug mode during development when MIDI output is not needed.