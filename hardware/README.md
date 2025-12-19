# Hardware Assembly Guide
## 4-Loop MIDI Switcher

This guide provides step-by-step instructions for building the 4-Loop MIDI Switcher hardware.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Safety](#safety)
- [Assembly Overview](#assembly-overview)
- [Step-by-Step Assembly](#step-by-step-assembly)
- [Testing & Troubleshooting](#testing--troubleshooting)
- [Enclosure Assembly](#enclosure-assembly)

## Prerequisites

### Required Tools
- Soldering iron (temperature controlled, 300-350°C recommended)
- Solder (lead or lead-free, 0.8mm diameter)
- Wire strippers (22-24 AWG)
- Multimeter (for continuity and voltage testing)
- Small screwdriver set
- Wire cutters / flush cutters
- Helping hands or PCB holder

### Required Skills
- Basic soldering skills
- Ability to read schematics and pin diagrams
- Understanding of polarity (for LEDs, electrolytic capacitors, diodes)
- Basic electronics knowledge

### Reference Documents
Before starting, familiarize yourself with:
- [Bill of Materials (BOM.md)](BOM.md) - All required components
- [Hardware Schematic (SCHEMATIC.md)](SCHEMATIC.md) - Complete pin assignments and circuit diagrams
- [Software Configuration (src/config.h)](../src/config.h) - Pin definitions in code

## Safety

### Electrical Safety
⚠️ **Warning**: Always disconnect power before making connections or modifications.

- Work in a well-ventilated area (soldering fumes)
- Use ESD-safe practices (wrist strap recommended when handling ICs)
- Double-check polarities before applying power
- Start with low voltage testing (use current-limited bench supply if available)

### Audio Equipment Safety
- Test with inexpensive gear first
- Use proper grounding to avoid ground loops
- Verify audio jacks are properly insulated from chassis ground if needed

## Assembly Overview

### Build Sequence
The recommended build order to facilitate testing at each stage:

1. **Power Distribution** - Set up power rails and verify voltages
2. **Microcontroller** - Mount Arduino and verify it powers on
3. **Display System** - Connect MAX7219 and 7-segment display
4. **LED Indicators** - Add 74HC595 and status LEDs
5. **Footswitches** - Wire momentary switches and DIP switches
6. **Relay Drivers** - Install ULN2003 and relays
7. **MIDI Output** - Add MIDI connector and circuitry
8. **Audio Path** - Wire audio jacks and relay contacts (last step)

### Testing Strategy
- Test each subsystem before moving to the next
- Upload test firmware to verify each component works
- Use serial debug output during development
- Only connect audio equipment after all digital circuits are verified

## Step-by-Step Assembly

### 1. Power Distribution System

#### Components
- Arduino Uno or Nano
- 100-470µF bulk capacitor
- 4x 100nF bypass capacitors
- 5V power supply (1A minimum)

#### Procedure
1. **Create Power Rails**:
   - On perfboard or breadboard, create +5V and GND rails
   - Use bus strips or dedicated traces for power distribution

2. **Install Bulk Capacitor**:
   ```
   +5V ----[+100-470µF]---- GND
           (watch polarity!)
   ```
   - Long lead (+) to +5V
   - Short lead (-) to GND
   - Place near power input

3. **Connect Arduino**:
   - Uno: Connect 5V power to DC jack (center positive) or USB
   - Nano: Connect 5V to VIN pin or USB
   - Verify Arduino onboard LED lights up

4. **Test**:
   - Measure voltage between 5V and GND pins: Should read 4.75V - 5.25V
   - No short circuits: Disconnect power immediately if supply current >100mA without load

### 2. Display System (MAX7219 + 7-Segment Display)

#### Components
- MAX7219 LED display driver IC (or pre-assembled module)
- 8-digit 7-segment common-cathode display
- 10kΩ resistor (ISET)
- 100nF bypass capacitor

#### Pin Connections
```
Arduino D11 (MOSI) --> MAX7219 Pin 1 (DIN)
Arduino D13 (SCK)  --> MAX7219 Pin 13 (CLK)
Arduino D12 (SS)   --> MAX7219 Pin 12 (LOAD/CS)
+5V                --> MAX7219 Pin 19 (VCC)
GND                --> MAX7219 Pin 4, 9 (GND)
100nF capacitor    --> Between VCC and GND (close to IC)
10kΩ resistor      --> MAX7219 Pin 18 (ISET) to GND
```

#### Using Pre-Assembled Module
If using a MAX7219 module with integrated display:
1. Connect VCC to +5V
2. Connect GND to GND
3. Connect DIN to D11
4. Connect CS to D12
5. Connect CLK to D13

#### Test Procedure
1. Upload firmware to Arduino (`pio run --target upload`)
2. Power on - display should show MIDI channel for 1 second
3. Display should then show "1" (Bank 1 in Bank Mode)
4. If display is blank or shows random segments, check wiring and ISET resistor

### 3. LED Status Indicators (74HC595 + LEDs)

#### Components
- 74HC595 shift register IC
- 8x LEDs (3mm or 5mm)
- 8x 220-470Ω current limiting resistors
- 100nF bypass capacitor

#### Pin Connections
```
Arduino A0  --> 74HC595 Pin 14 (DS/SER) - Serial Data
Arduino A1  --> 74HC595 Pin 11 (SHCP) - Shift Clock
Arduino A2  --> 74HC595 Pin 12 (STCP) - Latch Clock
+5V         --> 74HC595 Pin 16 (VCC)
+5V         --> 74HC595 Pin 10 (SRCLR) - Master Reset (active LOW, tie HIGH)
GND         --> 74HC595 Pin 8 (GND)
GND         --> 74HC595 Pin 13 (OE) - Output Enable (active LOW, tie LOW)
100nF cap   --> Between VCC and GND
```

#### LED Connections (Active HIGH configuration)
For each of the 8 LEDs:
```
74HC595 Output (Q0-Q7) --> [220Ω Resistor] --> LED Anode (+)
LED Cathode (-)        --> GND
```

Bit mapping:
- Q0 (Pin 15): Relay 1 LED
- Q1 (Pin 1): Relay 2 LED
- Q2 (Pin 2): Relay 3 LED
- Q3 (Pin 3): Relay 4 LED
- Q4 (Pin 4): Preset 1 LED
- Q5 (Pin 5): Preset 2 LED
- Q6 (Pin 6): Preset 3 LED
- Q7 (Pin 7): Preset 4 LED

#### Alternative: Active LOW Configuration
If using common-anode LEDs or wanting to source current:
```
+5V --> [220Ω Resistor] --> LED Anode (+)
LED Cathode (-)         --> 74HC595 Output
```
Set `LED_ACTIVE_LOW = true` in `src/config.h`

#### Test Procedure
1. Upload firmware
2. Power on - all LEDs should be OFF initially
3. Press footswitch 1 in Manual Mode - Relay 1 LED should light
4. Switch to Bank Mode (SW2+SW3) - preset LEDs should indicate selected preset

### 4. Footswitches & DIP Switches

#### Components
- 4x momentary footswitches (SPST normally open)
- 1x 4-position DIP switch package

#### Footswitch Connections
```
Arduino D2 --- [Footswitch 1] --- GND
Arduino D4 --- [Footswitch 2] --- GND
Arduino D5 --- [Footswitch 3] --- GND
Arduino D6 --- [Footswitch 4] --- GND
```

Internal pullup resistors are enabled in software, so no external resistors needed.

#### DIP Switch Connections
The DIP switches share the same pins as footswitches:
```
Arduino D2 --- [DIP Switch Bit 0] --- GND (when ON)
Arduino D4 --- [DIP Switch Bit 1] --- GND (when ON)
Arduino D5 --- [DIP Switch Bit 2] --- GND (when ON)
Arduino D6 --- [DIP Switch Bit 3] --- GND (when ON)
```

DIP switches are read only during power-up, so there's no conflict with footswitches.

#### MIDI Channel Configuration
Binary channel selection (0000 = Channel 1, 1111 = Channel 16):
```
Bit 3 (MSB)  Bit 2  Bit 1  Bit 0 (LSB)  = MIDI Channel Display
   OFF       OFF    OFF      OFF         = Channel 1
   OFF       OFF    OFF      ON          = Channel 2
   OFF       OFF    ON       OFF         = Channel 3
   ...
   ON        ON     ON       ON          = Channel 16
```

#### Test Procedure
1. Power off, set DIP switches to 0000 (all OFF)
2. Power on - display should show "1" for 1 second (MIDI Channel 1)
3. Press each footswitch - switches should toggle relay LEDs in Manual Mode
4. Test long press detection (SW2+SW3 held for 2s to enter Edit Mode)

### 5. Relay Driver Circuit (ULN2003 + Relays)

#### Components
- 1x ULN2003A Darlington array IC
- 4x DPDT 5V relays (60mA coil current)
- Optional: 4x 1kΩ input resistors
- Optional: 4x 1N4148 flyback diodes (ULN2003 has internal diodes)
- 100nF bypass capacitor

#### ULN2003 Pin Connections
```
Arduino D7  --> [1kΩ optional] --> ULN2003 Pin 1 (Input 1)
Arduino D8  --> [1kΩ optional] --> ULN2003 Pin 2 (Input 2)
Arduino D9  --> [1kΩ optional] --> ULN2003 Pin 3 (Input 3)
Arduino D10 --> [1kΩ optional] --> ULN2003 Pin 4 (Input 4)

GND --> ULN2003 Pin 8 (GND)
+5V --> ULN2003 Pin 9 (COM) - For internal flyback diodes

ULN2003 Pin 16 (Output 1) --> Relay 1 Coil (+)
ULN2003 Pin 15 (Output 2) --> Relay 2 Coil (+)
ULN2003 Pin 14 (Output 3) --> Relay 3 Coil (+)
ULN2003 Pin 13 (Output 4) --> Relay 4 Coil (+)

All Relay Coils (-) --> GND
```

#### Critical: Flyback Protection
The ULN2003A has internal flyback diodes that protect against inductive kickback when relay coils are de-energized. **Pin 9 (COM) must be connected to +5V** for these diodes to function.

#### Optional External Flyback Diodes
For additional protection, add diodes across each relay coil:
```
Relay Coil (+) ---[Diode Cathode  Diode Anode]--- Relay Coil (-)
                     (use 1N4148 or 1N4001)
```
Diode cathode (marked end) connects to coil positive terminal.

#### Test Procedure
1. **WITHOUT AUDIO CONNECTIONS**: Upload firmware and power on
2. Press footswitch 1 - should hear Relay 1 click and see corresponding LED
3. Press footswitch 1 again - relay should click again (toggle)
4. Test all 4 relays individually
5. **Current Check**: Measure total supply current with all 4 relays ON
   - Should be ~400-500mA total
   - If higher, check for short circuits

### 6. MIDI Output Circuit

#### Components
- 5-pin DIN connector (female, panel mount)
- 1x 220Ω resistor
- Optional: 6N138 or H11L1 optoisolator + 2x 220Ω resistors

#### Simple MIDI Output (Non-Isolated)
```
Arduino D1 (TX) --> [220Ω] --> DIN-5 Pin 5
Arduino GND     --> DIN-5 Pin 2
                    DIN-5 Pin 1 (Shield, optional chassis ground)
                    DIN-5 Pins 3, 4 not connected
```

#### MIDI Compliant Output (Optoisolated)
For full MIDI specification compliance:
```
+5V --> [220Ω] --> 6N138 LED Anode (Pin 2)
6N138 LED Cathode (Pin 3) --> [220Ω] --> Arduino D1 (TX)

+5V --> DIN-5 Pin 4
6N138 Output (Pin 6) --> DIN-5 Pin 5
DIN-5 Pin 2 --> GND (isolated)
6N138 Pin 5 --> GND (isolated)
```

Refer to 6N138 datasheet for complete optoisolated circuit.

#### Test Procedure
1. Connect MIDI cable to receiving device (MIDI monitor software or hardware)
2. Power on switcher - no MIDI messages should be sent during boot
3. Switch to Bank Mode (SW2+SW3 simultaneously)
4. Press any footswitch - MIDI monitor should show Program Change message
5. Verify correct MIDI channel (set by DIP switches)
6. Verify PC number: Bank 1, Switch 1 = PC 1; Bank 1, Switch 2 = PC 2, etc.

### 7. Audio Path Wiring

⚠️ **Complete all digital circuit testing before connecting audio equipment!**

#### Relay Contact Wiring for True Bypass
Each DPDT relay has 6 terminals:
```
Pole 1:         Pole 2:
  NC  COM  NO     NC  COM  NO
  |    |    |     |    |    |
```

#### Basic Loop Switching Configuration
For each of the 4 loops:
```
Pole 1:
- COM: Main signal path input (from previous loop or input jack)
- NO: Effect send (to effect input)
- NC: Bypass (connects to Pole 2 NC for direct through)

Pole 2:
- COM: Main signal path output (to next loop or output jack)
- NO: Effect return (from effect output)
- NC: Bypass (connects to Pole 1 NC for direct through)

When Relay OFF (de-energized): COM connects to NO
- Signal goes through effect loop

When Relay ON (energized): COM connects to NC
- Effect is bypassed
```

Connect a jumper wire between Pole 1 NC and Pole 2 NC to create the bypass path.

#### Example 4-Loop Signal Chain
```
Input Jack --> Loop 1 Relay --> Loop 2 Relay --> Loop 3 Relay --> Loop 4 Relay --> Output Jack
```

Each relay can independently insert or bypass its effect in the chain.

## Testing & Troubleshooting

### Systematic Testing

#### Power-On Test
- [ ] Display shows MIDI channel for 1 second
- [ ] Display then shows current bank number
- [ ] All LEDs start in OFF state
- [ ] No unusual heating of components
- [ ] Supply current <200mA with no relays active

#### Manual Mode Test
- [ ] Press single switch toggles relay
- [ ] Relay LED indicates correct state
- [ ] Relay audibly clicks
- [ ] All 4 switches work independently

#### Bank Mode Test
- [ ] SW2+SW3 switches to Bank Mode
- [ ] Display shows bank number (1-32)
- [ ] SW1+SW2 decrements bank
- [ ] SW3+SW4 increments bank
- [ ] Single switch press sends MIDI PC
- [ ] Preset LEDs indicate selected preset

#### Edit Mode Test
- [ ] Select preset, hold SW2+SW3 for 2 seconds
- [ ] Display flashes "Edit"
- [ ] Switches toggle relays
- [ ] Hold SW2+SW3 for 2 seconds to save
- [ ] Display shows "SAVEd"
- [ ] Power cycle and reload preset to verify save

### Common Issues

#### Display Not Working
- Check 5V power to MAX7219
- Verify SPI connections (DIN, CLK, CS pins)
- Check ISET resistor is connected (10kΩ to GND)
- Verify display type (must be common cathode)
- Try adjusting brightness in code

#### LEDs Not Lighting
- Check 74HC595 power and ground
- Verify resistor values (should be 220-470Ω)
- Check LED polarity (long lead is anode/+)
- Verify `LED_ACTIVE_LOW` setting in config.h matches wiring
- Check OE pin is tied to GND (enables outputs)

#### Switches Not Responding
- Verify internal pullups are enabled (done in software)
- Check switch wiring (one terminal to Arduino pin, other to GND)
- Test switch continuity with multimeter
- Increase debounce time in config.h if switches are noisy

#### Relays Not Switching
- Check 5V power to relay coils
- Verify ULN2003 connections (inputs and outputs)
- Test Arduino pin outputs with multimeter (should be 0V or 5V)
- Check COM pin (Pin 9) is connected to +5V for flyback protection
- Verify relay coil voltage rating (must be 5V)

#### MIDI Not Sending
- Verify baud rate is 31,250 (set in midi_handler.cpp)
- Check 220Ω resistor is in series with TX output
- Test with MIDI monitor software (MIDI-OX on Windows, etc.)
- Verify DIP switches are read correctly at power-up
- Check TX pin (D1) is not shorted to ground

#### High Power Consumption
- Check for short circuits (measure resistance between power rails)
- Verify no solder bridges between pins
- Check relay coil current individually (each should be ~60mA)
- Reduce display brightness if needed

## Enclosure Assembly

### Recommended Enclosure
- Hammond 1590XX or similar guitar pedal enclosure
- Dimensions: Minimum 125mm x 95mm x 35mm (W x D x H)
- Material: Die-cast aluminum (best shielding) or plastic

### Panel Layout
```
Top Panel:
[SW1]  [SW2]  [SW3]  [SW4]    <- Footswitches
[ LED Row: Relay Status  ]     <- 4 LEDs
[ LED Row: Preset Status ]     <- 4 LEDs
[    7-Segment Display   ]     <- Bank/PC display
```

Front/Back Panels:
- Audio jacks (8x: 4 sends, 4 returns)
- MIDI OUT (5-pin DIN)
- DC power jack (2.1mm)
- DIP switches (for MIDI channel)

### Assembly Tips
1. Drill holes for footswitches, jacks, and connectors before installing electronics
2. Use insulated standoffs to mount PCB/perfboard
3. Use shielded cable for audio connections
4. Keep digital wiring away from audio wiring to minimize noise
5. Connect enclosure to ground for shielding
6. Use cable management (zip ties, wire routing) for clean interior

### Finishing
- Label footswitches (1, 2, 3, 4)
- Label audio jacks (Loop 1-4 Send/Return)
- Label DIP switches (MIDI Channel: 1, 2, 4, 8)
- Add power requirements label (5V DC, 1A)

## Next Steps

### Software Upload
1. Connect Arduino via USB
2. Install PlatformIO or Arduino IDE
3. Open project and upload firmware: `pio run --target upload`
4. Disconnect USB and power via DC jack

### Configuration
1. Set MIDI channel using DIP switches
2. Program presets in Edit Mode
3. Test with your effects and MIDI gear

### Documentation
- Keep a list of your programmed presets
- Document your audio loop order
- Note any custom modifications

## Support & Resources

- **Firmware Source**: See `src/` directory
- **Configuration**: Edit `src/config.h` for pin changes
- **Issues**: Check GitHub repository for known issues
- **Community**: Consider sharing your build photos and mods!

## Safety & Warranty

⚠️ This is a DIY project. Build at your own risk. No warranty implied.

- Do not connect to AC mains directly
- Use only specified 5V DC power supply
- Verify all connections before applying power
- If you smell burning or see smoke, disconnect power immediately
- Seek help from experienced builders if unsure about any step

---

**Document Version**: 1.0  
**Last Updated**: December 2024
