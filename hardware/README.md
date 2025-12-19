# Hardware Documentation - 4-Loop MIDI Switcher

## Overview

This document provides hardware specifications and circuit details for the 4-Loop MIDI Switcher. The design is based on an ATmega328 microcontroller (Arduino Uno/Nano) with relay-switched audio loops.

## Table of Contents

- [Relay Driver Circuit](#relay-driver-circuit)
- [Component Specifications](#component-specifications)
- [Pin Assignments](#pin-assignments)
- [Current Requirements](#current-requirements)
- [PCB Design Files](#pcb-design-files)

---

## Relay Driver Circuit

### Critical Requirements

The ATmega328 GPIO pins **cannot drive relays directly**. Each GPIO pin has a maximum current rating of 40mA, while typical DPDT relays require 60-80mA to energize. Additionally, relay coils are **inductive loads** that generate dangerous voltage spikes (back-EMF) when switched off, which can damage the microcontroller.

### Driver Circuit Design

Each of the 4 relay outputs (D7-D10) uses an identical driver circuit. This design uses discrete transistors for maximum flexibility and clarity.

**Components per relay:**
- **Transistor:** 2N2222 NPN (or equivalent: BC547, 2N3904)
- **Base Resistor:** 1kΩ (limits base current to ~5mA)
- **Flyback Diode:** 1N4148 (protects against back-EMF)
- **Relay:** FINDER-36 DPDT relay (or equivalent 5V DPDT relay)

**Alternative: Integrated Driver IC**

For a more compact design, you can use a Darlington transistor array IC instead of discrete transistors:
- **ULN2003A** (7 channels) or **ULN2803A** (8 channels)
- Includes built-in base resistors and flyback diodes
- Single IC can drive all 4 relays
- Simplifies PCB layout and reduces component count
- Trade-off: Slightly higher voltage drop (Vce(sat) ≈ 1V vs 0.3V for discrete)

**Note:** This documentation describes the discrete transistor approach as shown in the schematic. If using a ULN IC, refer to the manufacturer's datasheet for pin connections.

### Circuit Schematic

```
                       +5V
                        |
                        +--------+--------+
                        |        |        |
                     [Relay]  [1N4148]    |
                      Coil    Cathode     |
                        |        |        |
                        +----+---+        |
                             |            |
Arduino GPIO (D7-D10)        | C          |
     |                  +----+----+       |
     +---[1kΩ]----------| B       |       |
                        |  2N2222 |       |
                        |   NPN   |       |
                        +----+----+       |
                             | E          |
                             |            |
                            GND          GND

Legend:
- B = Base, C = Collector, E = Emitter
- Flyback diode: Cathode (band) to +5V, Anode to Collector
- All 4 circuits share common +5V and GND
```

### Operation

1. **Relay OFF (GPIO LOW):**
   - Transistor base = 0V
   - Transistor is OFF (non-conducting)
   - No current flows through relay coil
   - Relay contacts remain in NC (normally closed) position

2. **Relay ON (GPIO HIGH):**
   - GPIO outputs 5V
   - Base current = (5V - 0.7V) / 1kΩ ≈ 4.3mA
   - Transistor saturates (fully ON)
   - Collector-emitter acts as closed switch
   - Relay coil energizes (~60mA from +5V rail)
   - Relay contacts switch to NO (normally open) position

3. **Switching OFF (GPIO HIGH → LOW):**
   - Transistor turns OFF
   - Relay coil current drops rapidly
   - Collapsing magnetic field induces voltage spike (up to -200V)
   - Flyback diode conducts, safely dissipating energy
   - Voltage spike is clamped to ~0.7V below +5V

### Why This Design

#### Transistor Switch (2N2222)
- **Current Gain:** β ≈ 100-300 (typical 150)
- **Base Current:** 4.3mA from Arduino GPIO (well within 40mA limit)
- **Collector Current:** 60mA (relay coil current)
- **Saturation:** Vce(sat) ≈ 0.3V (minimal voltage drop)
- **Switching Speed:** Fast enough for relay control (nanoseconds vs. milliseconds)

#### Base Resistor (1kΩ)
- Limits base current to safe levels
- Ensures transistor saturation: Ic/Ib = 60mA/4.3mA ≈ 14 (well below β)
- Protects GPIO pin from excessive current draw

#### Flyback Diode (1N4148)
- **Critical for safety:** Relay coils store energy in magnetic field
- Without diode: Back-EMF can reach -200V, destroying transistor and MCU
- Diode conducts reverse current, safely dissipating stored energy
- **Polarity:** Cathode (marked band) connects to +5V, anode to relay coil
- 1N4148 specifications:
  - Forward voltage drop: 0.7V
  - Reverse voltage: 100V (adequate for 5V relay)
  - Fast switching: 4ns (handles rapid relay switching)

**Alternative diodes:**
- 1N4001 (slower, but common in through-hole builds)
- 1N5817 Schottky (lower voltage drop, faster switching)

---

## Component Specifications

### Relays

**Part Number:** FINDER 36.11.9.005.4011 (or equivalent)

**Specifications:**
- **Type:** DPDT (Double Pole Double Throw)
- **Coil Voltage:** 5VDC
- **Coil Current:** 56.25mA typical (60mA worst-case)
- **Coil Resistance:** ~89Ω
- **Contact Rating:** 10A @ 250VAC (audio signals well within limits)
- **Contact Configuration:** 
  - Pin 19: Common (COM)
  - Pin 28: Normally Open (NO)
  - Pin 18: Normally Closed (NC)
- **Switching Time:** ~10ms operate, ~5ms release

**Alternative Relays:**
- Omron G5V-2-H1-5DC (surface mount)
- HJR-3FF-S-Z 5VDC (PCB mount)
- Any 5V DPDT relay with coil current ≤ 80mA

### Audio Switching Configuration

Each relay provides true bypass switching for one audio loop:

```
     DPDT Relay Contacts
     
Pin 19 (COM) ----+---- Audio Input
                 |
         +-------+-------+
         |               |
Pin 28 (NO)         Pin 18 (NC)
  Loop Send        Loop Return
  
When relay OFF: Signal flows Input → Loop Return (bypass)
When relay ON:  Signal flows Input → Loop Send (effect in loop)
```

### Transistors

**Part Number:** 2N2222A or 2N2222 (NPN)

**Specifications:**
- **Collector-Emitter Voltage:** 40V (5V operation provides 8x safety margin)
- **Collector Current:** 800mA continuous (60mA relay = 7.5% of rating)
- **Current Gain (hFE):** 100-300 @ Ic=150mA
- **Power Dissipation:** 500mW (actual: ~18mW = P = Ic × Vce(sat) = 60mA × 0.3V)

**Alternatives:**
- BC547 (lower current, cheaper, adequate for this application)
- 2N3904 (common, interchangeable with 2N2222)
- PN2222 (plastic package version of 2N2222)

### Flyback Diodes

**Part Number:** 1N4148

**Specifications:**
- **Type:** Fast switching silicon diode
- **Forward Current:** 300mA continuous (60mA relay = 20% of rating)
- **Peak Reverse Voltage:** 100V
- **Forward Voltage Drop:** 0.7V @ 10mA
- **Reverse Recovery Time:** 4ns

**Alternatives:**
- 1N4001 (slower, 1A rating, common in through-hole)
- 1N5817 (Schottky, 0.45V drop, faster)
- Any small signal diode rated ≥ 100V reverse voltage

---

## Pin Assignments

See `src/config.h` for complete pin configuration. Relay-related pins:

| Arduino Pin | Function | Direction | Notes |
|-------------|----------|-----------|-------|
| D7 | Relay 1 Driver | Output | Active HIGH, drives transistor base via 1kΩ |
| D8 | Relay 2 Driver | Output | Active HIGH, drives transistor base via 1kΩ |
| D9 | Relay 3 Driver | Output | Active HIGH, drives transistor base via 1kΩ |
| D10 | Relay 4 Driver | Output | Active HIGH, drives transistor base via 1kΩ |

**Important:** Pins are configured in `relays.cpp`:
```cpp
pinMode(relayPins[i], OUTPUT);
digitalWrite(relayPins[i], LOW);  // Relays OFF at startup
```

---

## Current Requirements

### Per Relay (Active)

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| Relay Coil | 60mA | Drawn from +5V rail |
| Transistor Base | 4.3mA | Drawn from GPIO pin |
| Flyback Diode | 0mA | Only conducts during turn-off transient |

### System Total (Worst Case: All 4 Relays ON)

| Component | Current | Source |
|-----------|---------|--------|
| ATmega328 | 15mA | +5V rail |
| MAX7219 Display | 100-150mA | +5V rail (depends on brightness) |
| 74HC595 + LEDs | 10-80mA | +5V rail (depends on LED current) |
| 4× Relay Coils | 240mA | +5V rail (4 × 60mA) |
| **Total** | **400-500mA** | +5V rail required |

### Power Supply Requirements

- **Voltage:** 5VDC ±5% (4.75V - 5.25V)
- **Current Capacity:** Minimum 600mA, recommended 1A
- **Regulation:** Required for stable relay operation
- **Connector:** 2.1mm barrel jack (center positive) or USB

**Important Notes:**
1. USB ports typically provide 500mA, which is marginal for this circuit
2. Recommend dedicated 5V/1A wall adapter for reliable operation
3. Arduino's onboard regulator (7-12V input) is NOT recommended due to heat dissipation

---

## PCB Design Files

### KiCad Project

The complete PCB design is available in the `hardware/loop_switcher/` directory:

- **Schematic:** `loop_switcher.kicad_sch`
- **PCB Layout:** `loop_switcher.kicad_pcb`
- **Project:** `loop_switcher.kicad_pro`

To view/edit these files, install [KiCad 9.0](https://www.kicad.org/) or later.

### Schematic PDF

For quick reference, a PDF export of the relay driver schematic is available:
- [`relay_driver_schematic.pdf`](relay_driver_schematic.pdf)

This PDF shows:
- Complete relay driver circuit for all 4 channels
- Component values and part numbers
- Pin connections to Arduino
- Power supply connections

### Manufacturing Files

When ready for PCB fabrication, generate Gerber files from KiCad:
1. Open `loop_switcher.kicad_pcb`
2. File → Fabrication Outputs → Gerbers
3. Generate Drill Files
4. Zip all files for upload to PCB manufacturer (JLCPCB, OSH Park, etc.)

---

## Safety Notes

### Electrical Safety

1. **Flyback Diodes are MANDATORY**
   - Operating relay without flyback diode WILL damage circuit
   - Back-EMF can exceed 200V, destroying transistor and MCU
   - Always verify diode polarity before powering up

2. **Power Supply**
   - Ensure adequate current capacity (≥600mA)
   - Use regulated 5V supply (switching noise can affect audio)
   - Add 100µF capacitor near relay driver circuit for decoupling

3. **First Power-Up**
   - Check all diode polarities (cathode band toward +5V)
   - Verify transistor orientation (flat side, pin order)
   - Use current-limited power supply initially (100mA limit)
   - Monitor current draw (should be ~15mA with relays OFF)

### Audio Signal Integrity

1. **Ground Loops**
   - Keep audio ground separate from digital ground where possible
   - Use star grounding topology (single point ground)
   - Connect audio ground to digital ground at ONE point only

2. **Relay Placement**
   - Keep relay coils away from audio traces
   - Route audio signals away from digital signals
   - Consider shielded cable for external loop connections

3. **Clicking/Popping**
   - Relay switching inherently creates mechanical clicks
   - This is normal for true bypass switching
   - Flyback diodes minimize electrical noise during switching

---

## Troubleshooting

### Relay Not Switching

**Symptom:** GPIO goes HIGH but relay doesn't click

**Possible Causes:**
1. Transistor installed backwards → Check orientation (flat side)
2. Base resistor too large → Verify 1kΩ value (Brown-Black-Red-Gold)
3. Insufficient power supply current → Measure voltage under load
4. Faulty transistor → Test with multimeter (diode test mode)
5. Faulty relay coil → Measure coil resistance (should be ~89Ω)

### Relay Clicks but Transistor/Arduino Damaged

**Symptom:** Circuit worked initially, now MCU doesn't respond

**Cause:** Missing or reversed flyback diode

**Fix:**
1. Replace damaged transistor and MCU
2. Install flyback diode with correct polarity
3. Verify diode with multimeter before power-up

### Intermittent Switching

**Symptom:** Relay switches inconsistently

**Possible Causes:**
1. Poor power supply → Check voltage under load (should be >4.75V)
2. Loose connections → Reflow solder joints
3. Insufficient base current → Verify base resistor value
4. Relay rated for higher voltage → Use proper 5V relay

### High Current Draw

**Symptom:** Power supply overloads or Arduino resets

**Possible Causes:**
1. Shorted relay coil → Disconnect relays, test individually
2. Transistor not saturating → Check base resistor, replace transistor
3. Multiple components faulty → Check each relay circuit independently

---

## References

### Datasheets

**Note:** External datasheet links may change over time. If a link is broken, search for the part number on the manufacturer's website or a datasheet aggregator like Digi-Key or Mouser.

- [2N2222A Transistor Datasheet](https://www.onsemi.com/pdf/datasheet/p2n2222a-d.pdf) - ON Semiconductor (search: "2N2222A datasheet")
- [1N4148 Diode Datasheet](https://www.vishay.com/docs/81857/1n4148.pdf) - Vishay (search: "1N4148 datasheet")
- [FINDER 36.11 Relay Datasheet](https://gfinder.findernet.com/public/attachments/36/EN/S36EN.pdf) - Finder (search: "FINDER 36.11 datasheet")
- [ATmega328P Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) - Microchip (search: "ATmega328P datasheet")

### Technical Resources

- [Arduino GPIO Current Limits](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/)
- [Relay Driver Design Guide](https://www.ti.com/lit/an/slva139/slva139.pdf)
- [Back-EMF Protection](https://www.electronics-tutorials.ws/blog/relay-switch-circuit.html)

### Related Documentation

- [Main README](../README.md) - Software features and build instructions
- [Architecture Documentation](../docs/ARCHITECTURE.md) - System design overview
- [Technical Review 002](../reviews/002_TECHNICAL_REVIEW.md) - Hardware integration review

---

## Revision History

| Date | Version | Changes | Author |
|------|---------|---------|--------|
| 2025-12-19 | 1.0 | Initial documentation of relay driver circuit | GitHub Copilot |

---

**For questions or hardware support, please open an issue on the [GitHub repository](https://github.com/jcpst/loop_switcher).**
