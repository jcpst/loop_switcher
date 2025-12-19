# Complete Hardware Schematic
## 4-Loop MIDI Switcher

**Project:** Loop Switcher v1.0  
**Platform:** ATmega328P (Arduino Uno/Nano)  
**Date:** December 2024  
**Repository:** github.com/jcpst/loop_switcher

---

## Table of Contents
- [System Overview](#system-overview)
- [Complete Pin Assignment](#complete-pin-assignment)
- [Subsystem Details](#subsystem-details)
  - [1. Microcontroller](#1-microcontroller---atmega328p)
  - [2. Footswitch Inputs](#2-footswitch-inputs-4x)
  - [3. MIDI Channel DIP Switches](#3-midi-channel-dip-switches-4x)
  - [4. Relay Driver Circuit](#4-relay-driver-circuit-4x-dpdt-relays)
  - [5. LED Status Indicators](#5-led-status-indicators-8x-via-74hc595)
  - [6. MAX7219 Display Driver](#6-max7219-display-driver--7-segment-display)
  - [7. MIDI Output Circuit](#7-midi-output-circuit)
  - [8. Power Supply](#8-power-supply)
- [Circuit Diagrams](#circuit-diagrams)
- [Additional Resources](#additional-resources)

---

## System Overview

This document provides complete hardware specifications for building a 4-loop guitar effects switcher with MIDI control.

### Key Features

| Feature | Specification |
|---------|---------------|
| **Audio Loops** | 4 independent DPDT relay switched loops |
| **MIDI Output** | Program Change messages (PC 1-128) |
| **MIDI Channels** | 16 channels, hardware selectable via DIP switches |
| **Display** | MAX7219-driven 7-segment LED display |
| **Status LEDs** | 8 LEDs via 74HC595 shift register (4 relay + 4 preset) |
| **Footswitches** | 4 momentary switches with 30ms software debounce |
| **Power** | 5V DC regulated, ~400-500mA typical, 1A recommended |
| **Presets** | 128 programmable presets (32 banks × 4 presets) |
| **Storage** | EEPROM-based preset storage with wear leveling |

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ATmega328P (Arduino)                     │
│                                                             │
│  D2-D6: Footswitches + DIP Switches (MIDI channel)         │
│  D7-D10: Relay drivers → ULN2003 → Relays                  │
│  D11-D13: SPI → MAX7219 → 7-segment display                │
│  A0-A2: 74HC595 shift register → 8 status LEDs             │
│  D1 (TX): MIDI output → DIN-5 connector                    │
└─────────────────────────────────────────────────────────────┘
```

---

## Complete Pin Assignment

### Arduino to ATmega328P Pin Mapping

| Arduino Pin | ATmega328 Pin | Function | Connection | Direction |
|-------------|---------------|----------|------------|-----------|
| **D0 (RX)** | PD0 (Pin 2) | Not Used | - | - |
| **D1 (TX)** | PD1 (Pin 3) | MIDI Output | MIDI TX via 220Ω to DIN-5 Pin 5 | Output |
| **D2** | PD2 (Pin 4) | Footswitch 1 / DIP Bit 0 | SW1 to GND (internal pullup) | Input |
| **D3** | PD3 (Pin 5) | Not Used | Available for expansion | - |
| **D4** | PD4 (Pin 6) | Footswitch 2 / DIP Bit 1 | SW2 to GND (internal pullup) | Input |
| **D5** | PD5 (Pin 11) | Footswitch 3 / DIP Bit 2 | SW3 to GND (internal pullup) | Input |
| **D6** | PD6 (Pin 12) | Footswitch 4 / DIP Bit 3 | SW4 to GND (internal pullup) | Input |
| **D7** | PD7 (Pin 13) | Relay 1 Control | ULN2003 Input 1 (Pin 1) | Output |
| **D8** | PB0 (Pin 14) | Relay 2 Control | ULN2003 Input 2 (Pin 2) | Output |
| **D9** | PB1 (Pin 15) | Relay 3 Control | ULN2003 Input 3 (Pin 3) | Output |
| **D10** | PB2 (Pin 16) | Relay 4 Control | ULN2003 Input 4 (Pin 4) | Output |
| **D11 (MOSI)** | PB3 (Pin 17) | MAX7219 Data | MAX7219 Pin 1 (DIN) - SPI | Output |
| **D12 (SS)** | PB4 (Pin 18) | MAX7219 CS | MAX7219 Pin 12 (LOAD/CS) - SPI | Output |
| **D13 (SCK)** | PB5 (Pin 19) | MAX7219 Clock | MAX7219 Pin 13 (CLK) - SPI | Output |
| **A0** | PC0 (Pin 23) | 74HC595 Data | 74HC595 Pin 14 (SER/DS) | Output |
| **A1** | PC1 (Pin 24) | 74HC595 Clock | 74HC595 Pin 11 (SHCP/SRCLK) | Output |
| **A2** | PC2 (Pin 25) | 74HC595 Latch | 74HC595 Pin 12 (STCP/RCLK) | Output |
| **A3** | PC3 (Pin 26) | Not Used | Available for expansion | - |
| **A4 (SDA)** | PC4 (Pin 27) | Not Used | Available for I2C expansion | - |
| **A5 (SCL)** | PC5 (Pin 28) | Not Used | Available for I2C expansion | - |

**Pin Usage Summary:**
- **18 of 20 pins used** (D0, D3, A3-A5 available for expansion)
- **Hardware UART:** D1 (TX) for MIDI output
- **Hardware SPI:** D11 (MOSI), D12 (SS), D13 (SCK) for MAX7219
- **Analog pins A0-A2:** Used as digital outputs

---

## Subsystem Details

### 1. Microcontroller - ATmega328P

**Component:** Arduino Uno R3 or Arduino Nano (both use ATmega328P)

#### Specifications

| Parameter | Value |
|-----------|-------|
| Microcontroller | ATmega328P |
| Clock Speed | 16 MHz (external crystal) |
| Flash Memory | 32 KB (program storage) |
| SRAM | 2 KB (runtime variables) |
| EEPROM | 1 KB (preset storage: 128 presets) |
| Operating Voltage | 5V DC regulated |
| I/O Pins Used | 18 of 20 available |
| Current Consumption | ~15mA (MCU only at 16MHz) |

#### Board Selection

- **Arduino Uno R3:** Standard form factor, easier to mount, DIP socket option
- **Arduino Nano:** Compact size, breadboard-friendly, SMD ATmega328
- **Compatibility:** Both boards are pin-compatible with this design

#### Power Input Options

1. **USB (5V):** Development only, limited to 500mA
2. **DC Jack (7-12V):** Uses onboard 7805 regulator (may overheat at high current)
3. **Direct 5V:** Connect external 5V supply to 5V pin and GND (recommended, bypasses regulator)

---

### 2. Footswitch Inputs (4x)

Four momentary footswitches provide user control with active LOW logic and internal pullup resistors.

#### Pin Assignments

| Switch | Arduino Pin | ATmega328 Pin | Function |
|--------|-------------|---------------|----------|
| **SW1** | D2 | PD2 (Pin 4) | Loop 1 toggle / Preset 1 / Bank down |
| **SW2** | D4 | PD4 (Pin 6) | Loop 2 toggle / Preset 2 / Bank down |
| **SW3** | D5 | PD5 (Pin 11) | Loop 3 toggle / Preset 3 / Bank up |
| **SW4** | D6 | PD6 (Pin 12) | Loop 4 toggle / Preset 4 / Bank up |

#### Circuit Diagram

```
        +5V
         |
        [20kΩ Internal Pull-up Resistor]
         |
Arduino Pin (D2/D4/D5/D6)----+
         |
        ===  Footswitch (SPST, Normally Open)
         |
        GND

Logic: HIGH when not pressed, LOW when pressed
```

#### Connection Details

- **Type:** Momentary SPST normally-open footswitches (guitar pedal style)
- **Wiring:** One terminal to Arduino pin, other terminal to GND
- **Pull-up:** Internal (enabled in software via `pinMode(pin, INPUT_PULLUP)`)
  - Internal pullup resistance: ~20-50kΩ (typical ATmega328 spec)
- **Logic:** Pin reads HIGH (5V) when not pressed, LOW (0V) when pressed
- **Debounce:** 30ms software debouncing (configured in `config.h`)

#### Component Specifications

- **Switch Type:** SPST momentary, normally open
- **Recommended:** Standard guitar pedal footswitches (soft-touch or heavy-duty)
- **Current Rating:** Minimal (digital input only, <1mA)
- **Mounting:** Panel mount with nut

---

### 3. MIDI Channel DIP Switches (4x)

Four DIP switches configure MIDI output channel (1-16) at power-up. These share the same pins as footswitches but are read only during initialization.

#### Pin Assignments

| Bit Position | Arduino Pin | Weight | Description |
|--------------|-------------|--------|-------------|
| **Bit 0 (LSB)** | D2 (SW1 pin) | +1 | Least significant bit |
| **Bit 1** | D4 (SW2 pin) | +2 | Channel select bit 1 |
| **Bit 2** | D5 (SW3 pin) | +4 | Channel select bit 2 |
| **Bit 3 (MSB)** | D6 (SW4 pin) | +8 | Most significant bit |

#### Channel Configuration Examples

| DIP Switches (B3 B2 B1 B0) | Binary | MIDI Channel (0-15) | Display (1-16) |
|----------------------------|--------|---------------------|----------------|
| OFF OFF OFF OFF | 0000 | 0 | Channel 1 |
| ON  OFF OFF OFF | 0001 | 1 | Channel 2 |
| OFF ON  OFF OFF | 0010 | 2 | Channel 3 |
| ON  ON  OFF OFF | 0011 | 3 | Channel 4 |
| OFF OFF OFF ON  | 0100 | 4 | Channel 5 |
| ... | ... | ... | ... |
| ON  ON  ON  ON  | 1111 | 15 | Channel 16 |

#### Circuit Diagram

```
Arduino Pin (D2/D4/D5/D6)----+
                             |
                            [20kΩ Internal Pullup to +5V]
                             |
                             +---- DIP Switch Terminal
                             |
                            === DIP Switch (when ON: connects to GND)
                             |
                            GND

When DIP switch OFF: Pin reads HIGH (pullup)
When DIP switch ON:  Pin reads LOW (pulled to GND)
```

#### Implementation Notes

- **Component:** 4-position DIP switch package (through-hole or SMD)
- **Wiring:** Each switch pulls corresponding pin to GND when ON
- **Reading Sequence:** DIP switches are read once in `setup()` before main loop starts
- **Display:** Selected MIDI channel shown on 7-segment display for 1 second at startup
- **No Conflict:** Footswitch polling begins after DIP switch reading is complete
- **Hardware Read Function:** See `StateManager::readMidiChannelFromHardware()` in `src/state_manager.cpp`

---

### 4. Relay Driver Circuit (4x DPDT Relays)

Four DPDT (Double-Pole Double-Throw) relays provide true bypass audio switching, driven by a ULN2003A Darlington transistor array.

#### Pin Assignments

| Relay | Arduino Pin | ULN2003 Input | ULN2003 Output | Relay Coil |
|-------|-------------|---------------|----------------|------------|
| **Relay 1** | D7 | Input 1 (Pin 1) | Output 1 (Pin 16) | 5V, 60mA |
| **Relay 2** | D8 | Input 2 (Pin 2) | Output 2 (Pin 15) | 5V, 60mA |
| **Relay 3** | D9 | Input 3 (Pin 3) | Output 3 (Pin 14) | 5V, 60mA |
| **Relay 4** | D10 | Input 4 (Pin 4) | Output 4 (Pin 13) | 5V, 60mA |

#### ULN2003A Darlington Array Specifications

| Parameter | Value |
|-----------|-------|
| **Part Number** | ULN2003A or ULN2803A (8-channel version) |
| **Channels Used** | 4 of 7 available |
| **Input Logic** | TTL/CMOS compatible (Arduino 5V) |
| **Output Current** | 500mA per channel (max), 60mA typical for relays |
| **Output Voltage** | 50V max |
| **Input Pins** | 1-7 (using 1-4) |
| **Output Pins** | 16-10 (using 16-13, inverted order) |
| **Common Pin** | Pin 8: GND |
| **COM Pin** | Pin 9: Connect to +5V for internal flyback diodes |
| **Internal Protection** | Built-in flyback diodes from outputs to COM pin |

#### Circuit Diagram

```
Arduino Pin (D7-D10)
     |
     +---[1kΩ optional]--- ULN2003 Input (Pin 1-4)
                                |
                           [Darlington
                            Transistor
                             Pair]
                                |
                           ULN2003 Output (Pin 16-13)---+
                                                         |
                                                    Relay Coil (+)
                                                         |
                                                     [Coil ~60mA]
                                                         |
                                                    Relay Coil (-)
                                                         |
                                                        GND

ULN2003 Pin 8: GND
ULN2003 Pin 9 (COM): +5V (provides return path for internal flyback diodes)

Optional: External flyback diode (1N4148 or 1N4001) across coil
          Cathode (marked end) to coil (+), Anode to coil (-)
```

#### CRITICAL: Flyback Protection

⚠️ **IMPORTANT:** The ULN2003A has **internal flyback diodes** that protect against inductive kickback when relay coils are de-energized. Pin 9 (COM) **MUST** be connected to +5V for these diodes to function properly.

**Why Flyback Protection is Needed:**
- Relay coils are inductive loads
- When coil is de-energized, magnetic field collapse induces high-voltage spike
- Without protection, this can damage the driver IC and microcontroller
- Flyback diode provides current path during spike

#### Relay Specifications

| Parameter | Specification |
|-----------|---------------|
| **Type** | DPDT (Double-Pole Double-Throw) |
| **Coil Voltage** | 5V DC |
| **Coil Current** | 40-70mA typical (60mA nominal) |
| **Contact Configuration** | 2 poles × 2 throws = 6 terminals per relay |
| **Contact Rating** | 1-2A @ 30V DC minimum (for audio signals) |
| **Switching Type** | Non-latching (continuous power required) |
| **Recommended Models** | Omron G5V-2, Panasonic TQ2, Finder 40.52, HK19F |
| **Mounting** | PCB mount (through-hole or SMD) or socket |

#### DPDT Relay Contact Configuration

```
        Pole 1                    Pole 2
         |                         |
    NC   COM   NO             NC   COM   NO
    |     |     |             |     |     |
   (1)   (2)   (3)           (4)   (5)   (6)

NC  = Normally Closed (connected when relay is energized)
COM = Common terminal (pole)
NO  = Normally Open (connected when relay is de-energized)
```

#### Audio Switching Example (True Bypass)

```
Relay State: OFF (De-energized)
    COM connects to NO
    Signal path: IN → Effect Send → [Effect] → Effect Return → OUT
    Loop is IN CIRCUIT

Relay State: ON (Energized)  
    COM connects to NC
    Signal path: Direct bypass (jumper wire between Pole 1 NC and Pole 2 NC)
    Loop is BYPASSED

Typical Wiring:
    Pole 1 COM: Main signal in (from previous loop or input)
    Pole 1 NO:  Effect send (to effect input)
    Pole 1 NC:  Bypass path (jumper to Pole 2 NC)
    Pole 2 COM: Main signal out (to next loop or output)
    Pole 2 NO:  Effect return (from effect output)
    Pole 2 NC:  Bypass path (jumper to Pole 1 NC)
```

---

### 5. LED Status Indicators (8x via 74HC595)

Eight status LEDs driven by a 74HC595 8-bit shift register, using only 3 Arduino pins.

#### Pin Assignments

| 74HC595 Pin | Function | Arduino Pin | Description |
|-------------|----------|-------------|-------------|
| **Pin 14 (DS/SER)** | Serial Data | A0 | Data input (serial) |
| **Pin 11 (SHCP)** | Shift Clock | A1 | Shift register clock |
| **Pin 12 (STCP)** | Latch Clock | A2 | Storage register clock (output update) |
| **Pin 13 (OE)** | Output Enable | GND | Active LOW, tie to GND for always enabled |
| **Pin 10 (SRCLR)** | Master Reset | +5V | Active LOW, tie HIGH for normal operation |
| **Pin 8** | Ground | GND | Ground |
| **Pin 16** | Power | +5V | 5V supply |
| **Pins 15, 1-7 (Q0-Q7)** | Outputs | LEDs | 8 parallel outputs to LEDs |

#### LED Bit Mapping

| Bit | Output Pin | LED Function | Indicates |
|-----|------------|--------------|-----------|
| **Bit 0** | Q0 (Pin 15) | Relay 1 LED | Loop 1 relay state (on/off) |
| **Bit 1** | Q1 (Pin 1) | Relay 2 LED | Loop 2 relay state (on/off) |
| **Bit 2** | Q2 (Pin 2) | Relay 3 LED | Loop 3 relay state (on/off) |
| **Bit 3** | Q3 (Pin 3) | Relay 4 LED | Loop 4 relay state (on/off) |
| **Bit 4** | Q4 (Pin 4) | Preset 1 LED | Preset 1 selected (Bank Mode only) |
| **Bit 5** | Q5 (Pin 5) | Preset 2 LED | Preset 2 selected (Bank Mode only) |
| **Bit 6** | Q6 (Pin 6) | Preset 3 LED | Preset 3 selected (Bank Mode only) |
| **Bit 7** | Q7 (Pin 7) | Preset 4 LED | Preset 4 selected (Bank Mode only) |

#### Circuit Diagram

```
Arduino A0 ──────> 74HC595 Pin 14 (SER - Serial Data Input)
Arduino A1 ──────> 74HC595 Pin 11 (SHCP - Shift Register Clock)
Arduino A2 ──────> 74HC595 Pin 12 (STCP - Storage Register Clock / Latch)

+5V ─────────────> 74HC595 Pin 16 (VCC)
+5V ─────────────> 74HC595 Pin 10 (SRCLR - Master Reset, active LOW)
GND ─────────────> 74HC595 Pin 8 (GND)
GND ─────────────> 74HC595 Pin 13 (OE - Output Enable, active LOW)

[100nF capacitor] between VCC and GND (close to IC)

For each LED output (×8):
  Active HIGH configuration (LED_ACTIVE_LOW = false in config.h):
    74HC595 Q0-Q7 ──> [220-470Ω Resistor] ──> LED Anode (+) ──> LED Cathode (-) ──> GND

  Active LOW configuration (LED_ACTIVE_LOW = true in config.h):
    +5V ──> [220-470Ω Resistor] ──> LED Anode (+) ──> LED Cathode (-) ──> 74HC595 Q0-Q7
```

#### LED Current Limiting Resistor Calculation

For active HIGH configuration:

```
R = (Vsupply - Vf) / If

Where:
  Vsupply = 5V (Arduino/74HC595 output HIGH)
  Vf = LED forward voltage
  If = Desired LED current (typically 10-20mA)

Examples:
  Red/Yellow LED (Vf = 2.0V): R = (5V - 2.0V) / 0.015A = 200Ω → use 220Ω
  Green LED (Vf = 2.2V):      R = (5V - 2.2V) / 0.015A = 187Ω → use 220Ω
  Blue/White LED (Vf = 3.0V): R = (5V - 3.0V) / 0.015A = 133Ω → use 150Ω or 220Ω (dimmer)
```

#### LED Specifications

| Parameter | Value |
|-----------|-------|
| **LED Type** | Standard 3mm or 5mm indicator LEDs |
| **Forward Voltage (Vf)** | Red/Yellow: ~2.0V, Green: ~2.2V, Blue/White: ~3.0V |
| **Forward Current (If)** | 20mA typical max, 10-15mA recommended for visibility |
| **Current Limiting Resistor** | 220Ω (for 15mA) to 470Ω (for 7mA) |
| **74HC595 Output Current** | 70mA max per pin, 35mA recommended, 10-15mA safe |
| **Total Current (8 LEDs)** | 80-120mA @ 10-15mA per LED |

#### Software Configuration

In `src/config.h`:
```cpp
const bool LED_ACTIVE_LOW = false;  // Set to true if LEDs wired: +5V -> resistor -> LED -> 74HC595
```

---

### 6. MAX7219 Display Driver + 7-Segment Display

MAX7219 LED display driver controls an 8-digit 7-segment display using SPI communication.

#### Pin Assignments

| MAX7219 Pin | Function | Arduino Pin | Description |
|-------------|----------|-------------|-------------|
| **Pin 1 (DIN)** | Data In | D11 (MOSI) | SPI serial data input |
| **Pin 13 (CLK)** | Clock | D13 (SCK) | SPI clock |
| **Pin 12 (LOAD/CS)** | Chip Select | D12 (SS) | Load/latch signal (active LOW) |
| **Pin 19 (VCC)** | Power | +5V | 5V supply |
| **Pin 4, 9 (GND)** | Ground | GND | Ground |
| **Pin 18 (ISET)** | Current Set | via 10kΩ to GND | Sets segment current |
| **Pins 14-17, 20-23** | Digit Drivers | Display | Digit 0-7 cathode drivers |
| **Pins 2, 3, 5-8, 10, 11** | Segment Drivers | Display | Segment A-G, DP drivers |

#### Circuit Diagram

```
Arduino D11 (MOSI) ────> MAX7219 Pin 1 (DIN)
Arduino D13 (SCK)  ────> MAX7219 Pin 13 (CLK)
Arduino D12 (SS)   ────> MAX7219 Pin 12 (LOAD/CS)

+5V ────────────────────> MAX7219 Pin 19 (VCC)
GND ────────────────────> MAX7219 Pin 4, Pin 9 (GND)

[100nF capacitor] between VCC and GND (as close to IC as possible)

MAX7219 Pin 18 (ISET) ──> [10kΩ Resistor] ──> GND

Display connections:
  MAX7219 Digit pins → 7-segment display digit cathodes
  MAX7219 Segment pins → 7-segment display segment anodes
  (Refer to MAX7219 datasheet for exact pin mapping)
```

#### ISET Resistor Calculation

The ISET resistor sets the segment current:

```
Iseg = 100 × (Vref / RSET)

Where:
  Vref ≈ 1.28V (MAX7219 internal reference)
  RSET = ISET resistor value

Examples:
  RSET = 10kΩ:  Iseg = 100 × (1.28V / 10,000Ω) = 12.8mA per segment
  RSET = 4.7kΩ: Iseg = 100 × (1.28V / 4,700Ω) = 27mA per segment (brighter, max recommended)
  RSET = 22kΩ:  Iseg = 100 × (1.28V / 22,000Ω) = 5.8mA per segment (dimmer, lower power)

Recommended: 10kΩ for moderate brightness
```

#### Display Specifications

| Parameter | Value |
|-----------|-------|
| **Display Type** | 8-digit 7-segment LED display |
| **Common Type** | Common cathode (required for MAX7219) |
| **Segment Current** | Set by ISET resistor (~12.8mA @ 10kΩ) |
| **Total Current** | 100-150mA (depends on brightness and digits lit) |
| **Brightness Control** | Software configurable (0-15 levels) via `setIntensity()` |
| **Module Option** | Pre-assembled MAX7219 module with display recommended |

#### SPI Communication

| Parameter | Value |
|-----------|-------|
| **Library** | LedControl v1.0.6 (wayoda/LedControl) |
| **Interface** | Hardware SPI (uses ATmega328 SPI peripheral) |
| **Speed** | 10MHz max, Arduino typically uses 4MHz |
| **Mode** | SPI Mode 0 (CPOL=0, CPHA=0) |
| **Bit Order** | MSB first |

#### Display Usage

The display shows:
- **Startup:** MIDI channel (1-16) for 1 second
- **Manual Mode:** Blank or loop indicators
- **Bank Mode:** Current bank number (1-32)
- **MIDI Send:** Flashing program change number (1-128)
- **Edit Mode:** Flashing "Edit" message
- **Save:** "SAVEd" message for 2 seconds

#### Pre-Assembled Module Option

Many suppliers offer MAX7219 modules with integrated 8-digit display, resistors, and connectors:
- **Advantages:** Simplified wiring, tested configuration, compact
- **Connection:** 5-pin interface (VCC, GND, DIN, CS, CLK)
- **Cost:** ~$3-5 USD
- **Recommended:** Easier for beginners

---

### 7. MIDI Output Circuit

MIDI output uses hardware UART (Serial) on Arduino pin D1 (TX).

#### Pin Assignments

| Signal | Arduino Pin | ATmega328 Pin | MIDI DIN-5 Pin |
|--------|-------------|---------------|----------------|
| **MIDI TX** | D1 (TX) | PD1 (Pin 3) | Pin 5 (via 220Ω) |
| **MIDI Ground** | GND | GND | Pin 2 |
| **MIDI Shield** | - | - | Pin 1 (optional, chassis) |

#### MIDI Specifications

| Parameter | Value |
|-----------|-------|
| **Baud Rate** | 31,250 baud (MIDI standard) |
| **Data Format** | 8-N-1 (8 data bits, no parity, 1 stop bit) |
| **Logic Levels** | TTL/5V from Arduino TX |
| **Current Loop** | 5mA nominal (MIDI spec with optoisolator) |
| **Connector** | 5-pin DIN female (180° or 270° type) |
| **Cable** | Standard MIDI cable (DIN-5 male to male) |
| **Messages Sent** | Program Change (0xC0-0xCF) only |
| **Channel** | Configurable via DIP switches (0-15, displayed as 1-16) |
| **Program Range** | PC 1-128 (transmitted as MIDI 0-127) |

#### Simple MIDI Output Circuit (Non-Isolated)

```
Arduino D1 (TX) ──> [220Ω Resistor] ──> DIN-5 Pin 5 (MIDI TX Signal)

Arduino GND ──> DIN-5 Pin 2 (MIDI Ground)

DIN-5 Pin 1: Shield (optional, connect to chassis ground)
DIN-5 Pin 3: Not connected (used for MIDI IN on receiving device)
DIN-5 Pin 4: Not connected (used in optoisolated circuit)
```

**Advantages:**
- Simple, minimal components
- Works for most applications
- Direct connection

**Disadvantages:**
- Not MIDI compliant (spec requires optoisolation)
- No galvanic isolation (potential ground loops)

#### MIDI Compliant Circuit (Optoisolated - Recommended)

```
                    +5V
                     |
                  [220Ω]
                     |
                     +──> DIN-5 Pin 4
                     |
         ┌───────────┴───────────┐
         │   6N138 Optoisolator  │
         │                       │
    +5V ─┤ Pin 8 (VCC)           │
         │                       │
Arduino  │         Pin 2 (Anode) ├──> [220Ω] ──> +5V
D1 (TX) ─┤ [220Ω]  Pin 3 (Cathode)│
         │                       │
         │         Pin 6 (Output)├───> DIN-5 Pin 5 (MIDI TX)
         │                       │
    GND ─┤ Pin 5 (GND isolated)  ├──> DIN-5 Pin 2 (GND isolated)
         │                       │
         └───────────────────────┘

Note: 6N138 provides galvanic isolation per MIDI specification
Alternative: H11L1 optoisolator can also be used
```

**Advantages:**
- Full MIDI specification compliance
- Galvanic isolation prevents ground loops
- Protects equipment in complex setups

**Disadvantages:**
- Additional components required
- Slightly more complex wiring

**Recommendation:** Use optoisolated circuit for professional builds and complex rigs. Simple circuit is acceptable for standalone use.

#### MIDI DIN-5 Connector Pinout

```
      View from solder side of female panel-mount connector
      
           3     1
             \ /
          4   o   2
             / \
               5
```

| Pin | Function | Connection |
|-----|----------|------------|
| **1** | Shield | Optional, chassis ground for cable shield |
| **2** | Ground | MIDI ground (isolated in optoisolated circuit) |
| **3** | Not Connected | Used for MIDI IN |
| **4** | +5V (optoisolated) | Used in optoisolated circuit only |
| **5** | MIDI TX | MIDI signal output |

#### Software Implementation

MIDI output is implemented in `src/midi_handler.cpp`:

```cpp
const uint32_t MIDI_BAUD = 31250;  // MIDI standard baud rate

void initMIDI() {
  Serial.begin(MIDI_BAUD);
}

void sendMIDIProgramChange(uint8_t program, uint8_t channel) {
  uint8_t statusByte = 0xC0 | (channel & 0x0F);  // Program Change + channel
  uint8_t programByte = (program - 1) & 0x7F;    // PC 1-128 maps to MIDI 0-127
  
  Serial.write(statusByte);
  Serial.write(programByte);
}
```

**MIDI Program Change Message Format:**
```
Byte 1: Status Byte = 0xCn (n = MIDI channel 0-15)
Byte 2: Program Number = 0-127
```

**Important:** Debug output and MIDI TX share the same hardware Serial port. Do not enable `DEBUG_MODE` when using MIDI output.

---

### 8. Power Supply

System requires regulated 5V DC power supply.

#### Power Requirements

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| **ATmega328P** | 15mA | 16MHz active mode |
| **MAX7219 + Display** | 100-150mA | Brightness dependent |
| **74HC595 + 8 LEDs** | 80-160mA | 10-20mA per LED |
| **4× Relay Coils** | 240mA | 60mA each, all energized |
| **ULN2003A** | 5mA | Quiescent current |
| **MIDI Output** | 5mA | If optoisolated |
| **Total (Typical)** | **450-580mA** | **Recommend 1A supply** |
| **Idle (no relays)** | **200-330mA** | Display and MCU only |

#### Power Supply Specifications

| Parameter | Specification |
|-----------|---------------|
| **Voltage** | 5V DC ± 5% (4.75V - 5.25V) |
| **Current Rating** | 1A minimum, 1.5A recommended for headroom |
| **Connector** | 2.1mm barrel jack (center positive) or equivalent |
| **Regulation** | Regulated (switching or linear OK) |
| **Ripple** | <100mV peak-to-peak |
| **Protection** | Overcurrent and short-circuit protection recommended |

#### Power Input Options

**Option 1: Direct 5V Input (Recommended)**
```
External 5V supply → Arduino 5V pin + GND
```
- Bypasses onboard regulator (more efficient)
- Full current available for external components
- No heat dissipation issues

**Option 2: 9V Input via Arduino Regulator**
```
9V supply → Arduino DC jack or VIN pin
```
- Onboard 7805 regulates to 5V
- Convenient but regulator may overheat with 400-500mA load
- Power dissipation = (9V - 5V) × 0.5A = 2W (significant heat)
- Use heatsink on regulator if using this method

**Option 3: USB Power (Development Only)**
```
USB 5V → Arduino USB connector
```
- Limited to 500mA (USB 2.0 spec)
- May not be sufficient with all relays ON (requires 450-580mA)
- Acceptable for development and testing
- Not recommended for production use

#### Power Distribution

**Power Rail Setup:**
```
Power Input (+5V) ───┬─── Arduino 5V pin
                     ├─── MAX7219 VCC
                     ├─── 74HC595 VCC
                     ├─── ULN2003 Pin 9 (COM)
                     └─── 4× Relay coils (via ULN2003)

Ground (GND) ────────┬─── Arduino GND
                     ├─── MAX7219 GND
                     ├─── 74HC595 GND
                     ├─── ULN2003 Pin 8
                     ├─── All LED cathodes
                     ├─── All switch commons
                     └─── MIDI DIN-5 Pin 2
```

#### Bypass Capacitors (Essential)

**Bulk Capacitor:**
- **Value:** 100-470µF electrolytic
- **Location:** Near power input
- **Purpose:** Filter low-frequency noise, provide current during relay switching
- **Polarity:** Watch polarity! (+) to +5V, (-) to GND

**Bypass Capacitors (one per IC):**
- **Value:** 100nF (0.1µF) ceramic
- **Location:** As close as possible to each IC's VCC and GND pins
- **ICs requiring bypass caps:**
  - MAX7219
  - 74HC595
  - ULN2003A
  - Arduino/ATmega328 (usually has onboard cap)
- **Purpose:** Filter high-frequency noise, stabilize IC operation

#### Circuit Diagram

```
Power Input (+5V) ───┬─── [100-470µF Bulk Cap] ─── GND
                     │
                     ├─── Arduino 5V ─┬─ [100nF] ─ GND
                     │                │
                     ├─── MAX7219 VCC ┬─ [100nF] ─ GND
                     │                │
                     ├─── 74HC595 VCC ┬─ [100nF] ─ GND
                     │                │
                     └─── ULN2003 COM ┬─ [100nF] ─ GND
                                      │
                    All GND connections tied together (star ground recommended)
```

#### Power-On Sequence

1. Power supply stabilizes to 5V
2. Arduino bootloader runs (~2 seconds)
3. Firmware reads DIP switches for MIDI channel
4. Display shows MIDI channel for 1 second
5. System initializes to Bank Mode, Bank 1
6. All relays OFF, LEDs show initial state

#### Power-Off Behavior

- Relays immediately de-energize (loops switch to bypass)
- Display turns off
- All LEDs turn off
- No audio pops (depends on external audio muting circuit if present)

---

## Circuit Diagrams

### Complete System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Power Supply (5V, 1A)                         │
│                                                                         │
│  [Bulk Cap 470µF] ────┬──────────┬──────────┬──────────┬──────────    │
│                       │          │          │          │               │
└───────────────────────┼──────────┼──────────┼──────────┼───────────────┘
                        │          │          │          │
                        ▼          ▼          ▼          ▼
            ┌───────────────┐  ┌──────┐  ┌──────┐  ┌──────┐
            │  ATmega328P   │  │MAX   │  │74HC  │  │ULN   │
            │   (Arduino)   │  │7219  │  │595   │  │2003  │
            │               │  │      │  │      │  │      │
            │  D2-D6: SW    │  └──┬───┘  └──┬───┘  └──┬───┘
            │  D7-D10: Rel  │     │         │         │
            │  D11-D13: SPI │     │         │         │
            │  A0-A2: Shift │     │         │         ▼
            │  D1: MIDI TX  │     │         │    ┌────────┐
            └───────────────┘     │         │    │ Relays │
                    │             │         │    │  (4x)  │
                    │             ▼         ▼    └────────┘
                    │      ┌──────────┐  ┌────────┐
                    │      │ Display  │  │  LEDs  │
                    │      │ (8-digit)│  │  (8x)  │
                    │      └──────────┘  └────────┘
                    │
                    └──> MIDI OUT (DIN-5)

Input Devices:
  • 4 Footswitches → D2, D4, D5, D6
  • 4-bit DIP Switch → D2, D4, D5, D6 (read at power-up)
```

### Minimal Working Circuit (Core Components Only)

For initial testing, this minimal circuit verifies basic functionality:

```
┌─────────────────┐
│  Arduino Nano   │
│                 │
│  D2 ──┤SW1├──┐  │
│  D4 ──┤SW2├──┼──┤───── GND
│  D5 ──┤SW3├──┤  │
│  D6 ──┤SW4├──┘  │
│                 │
│  D11──┐         │
│  D12──┤ SPI     │──> To MAX7219 module → Display
│  D13──┘         │
│                 │
│  D1 ─[220Ω]────┤───── MIDI DIN-5 Pin 5
│                 │
└─────────────────┘

Power: USB or 5V supply
This minimal circuit lets you test:
  • Switch reading and mode changes
  • Display output
  • MIDI transmission
```

### Full Production Circuit

Add these for complete functionality:
```
• Relay drivers (ULN2003 + 4 relays)
• LED indicators (74HC595 + 8 LEDs + resistors)
• DIP switches for MIDI channel
• Proper power supply with bulk and bypass caps
• Audio jacks and relay contact wiring
• Enclosure and panel hardware
```

---

## Additional Resources

### Reference Documents

- **Bill of Materials:** [hardware/BOM.md](BOM.md) - Complete component list with part numbers
- **Assembly Guide:** [hardware/README.md](README.md) - Step-by-step assembly instructions
- **Software Configuration:** [src/config.h](../src/config.h) - Pin definitions and constants
- **Architecture:** [docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) - Software architecture
- **Code Review:** [reviews/002_TECHNICAL_REVIEW.md](../reviews/002_TECHNICAL_REVIEW.md) - Technical analysis

### Datasheets

Essential datasheets for this project:
- **ATmega328P:** Microcontroller datasheet
- **ULN2003A:** Darlington transistor array
- **74HC595:** 8-bit shift register
- **MAX7219:** LED display driver
- **6N138:** High-speed optoisolator (for MIDI)

### Software Repository

- **Repository:** https://github.com/jcpst/loop_switcher
- **Firmware:** PlatformIO project in `src/` directory
- **Libraries:** LedControl v1.0.6 (automatically installed via PlatformIO)

### Build Tips

1. **Start with minimal circuit** - Test core functionality before adding relays
2. **Test each subsystem independently** - Verify display, then LEDs, then relays
3. **Use pre-assembled modules** - MAX7219 display modules simplify construction
4. **Add bypass capacitors** - Essential for stable operation
5. **Label everything** - Mark pins, wires, and components during assembly
6. **Measure voltages** - Verify 5V on all IC power pins before powering on
7. **Use debug mode** - Enable `DEBUG_MODE` in config.h during development (disables MIDI)

### Safety Notes

⚠️ **Safety First:**
- Always disconnect power before making changes
- Verify polarity before connecting power
- Do not exceed 5.5V on any input
- Use current-limited power supply during initial testing
- Add fuse or current limiting for protection

---

## Document Information

**Version:** 1.0  
**Last Updated:** December 2024  
**Status:** Production Ready  
**License:** See [LICENSE](../LICENSE) file

**Created by:** Hardware schematic documentation  
**For:** 4-Loop MIDI Switcher (jcpst/loop_switcher)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Dec 2024 | Initial comprehensive schematic documentation |

---

**End of Hardware Schematic Documentation**
