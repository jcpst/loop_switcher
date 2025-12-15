# 4-Loop MIDI Switcher with Bank Mode

A programmable guitar effects loop switcher with MIDI control, built on the ATmega328 platform (Arduino Uno/Nano compatible). Features 4 audio loops, MIDI program change output, bank selection, and a 7-segment display.

## Features

- **4 Audio Loops**: Independent DPDT relay switching for true bypass
- **Three Operating Modes**:
  - **Manual Mode**: Direct loop on/off control via footswitches
  - **Bank Mode**: 32 banks x 4 presets = 128 MIDI program changes
  - **Edit Mode**: Edit loop states for stored presets
- **MIDI Output**: Sends Program Change messages (PC 1-128) on configurable channel (1-16)
- **7-Segment Display**: Shows current bank, MIDI channel, or program change
- **Global Preset Mode**: Special "all loops off" mode in Bank Mode
- **Persistent Settings**: MIDI channel stored in EEPROM

## Hardware Requirements

### Components
- ATmega328-based board (Arduino Uno or Nano)
- 4x momentary footswitches (active LOW with pullups)
- MAX7219 7-segment display driver with 3-digit display
- 4x DPDT relays for audio switching
- MIDI output circuit (hardware UART)

### Pin Configuration
See `src/config.h` for detailed pin assignments

## Controls

### Switch Combinations

| Switches | Action | Mode |
|----------|--------|------|
| SW1 (long press) + SW4 (long press) | Enter MIDI channel setup | Any |
| SW2 + SW3 | Toggle Manual/Bank mode | Any |
| SW2 (2s hold) + SW3 (2s hold) | Enter/Exit Edit Mode | Bank (after preset selected) |
| SW1 + SW2 | Bank down / Channel down | Bank / Channel Set |
| SW3 + SW4 | Bank up / Channel up | Bank / Channel Set |
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

### Channel Set Mode
- Long press SW1+SW4 to enter
- Use SW1+SW2 / SW3+SW4 to adjust MIDI channel (1-16)
- Press SW2+SW3 to save and exit
- Auto-exits after timeout

## Display States

| Display | Meaning |
|---------|---------|
| Bank number (1-32) | Current bank in Bank Mode |
| Channel number (1-16) | MIDI channel in Channel Set Mode |
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