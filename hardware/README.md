# Hardware Assembly Guide
## 4-Loop MIDI Switcher

Reference documents:
- [Bill of Materials (BOM.md)](BOM.md)
- [Hardware Schematic (SCHEMATIC.md)](SCHEMATIC.md)
- [Software Configuration (src/config.h)](../src/config.h)

## Build Sequence

1. Power Distribution
2. Display System (MAX7219)
3. LED Indicators (74HC595)
4. Footswitches & DIP Switches
5. Relay Drivers (ULN2003)
6. MIDI Output
7. Audio Path (final step)

## Step-by-Step Assembly

### 1. Power Distribution

Connect 100-470µF bulk capacitor between +5V and GND near power input. Add 100nF bypass capacitors at each IC (MAX7219, 74HC595, ULN2003). Verify 4.75-5.25V on power rails.

### 2. Display System (MAX7219)

#### Pin Connections
```
Arduino D11 (MOSI) --> MAX7219 Pin 1 (DIN)
Arduino D13 (SCK)  --> MAX7219 Pin 13 (CLK)
Arduino D12 (SS)   --> MAX7219 Pin 12 (LOAD/CS)
+5V                --> MAX7219 Pin 19 (VCC)
GND                --> MAX7219 Pin 4, 9 (GND)
10kΩ resistor      --> MAX7219 Pin 18 (ISET) to GND
```

For pre-assembled modules, connect VCC, GND, DIN (D11), CS (D12), CLK (D13).

Upload firmware and verify display shows MIDI channel on power-up.

### 3. LED Status Indicators (74HC595)

#### Pin Connections
```
Arduino A0  --> 74HC595 Pin 14 (SER)
Arduino A1  --> 74HC595 Pin 11 (SHCP)
Arduino A2  --> 74HC595 Pin 12 (STCP)
+5V         --> 74HC595 Pin 16 (VCC), Pin 10 (SRCLR)
GND         --> 74HC595 Pin 8 (GND), Pin 13 (OE)
```

#### LED Connections
Active HIGH: `74HC595 Output --> [220-470Ω] --> LED Anode --> LED Cathode --> GND`

Active LOW: `+5V --> [220-470Ω] --> LED Anode --> LED Cathode --> 74HC595 Output`
(Set `LED_ACTIVE_LOW = true` in config.h)

Bit mapping: Q0-Q3 (Relay 1-4), Q4-Q7 (Preset 1-4)

### 4. Footswitches & DIP Switches

#### Connections
```
Arduino D2 --- [Footswitch 1 / DIP Bit 0] --- GND
Arduino D4 --- [Footswitch 2 / DIP Bit 1] --- GND
Arduino D5 --- [Footswitch 3 / DIP Bit 2] --- GND
Arduino D6 --- [Footswitch 4 / DIP Bit 3] --- GND
```

Internal pullups enabled in software. DIP switches read at power-up only (no conflict with footswitches).

MIDI channel binary: 0000 = Ch 1, 1111 = Ch 16

### 5. Relay Drivers (ULN2003)

#### Pin Connections
```
Arduino D7-D10 --> ULN2003 Pin 1-4 (Inputs)
GND            --> ULN2003 Pin 8
+5V            --> ULN2003 Pin 9 (COM - enables internal flyback diodes)

ULN2003 Pin 16-13 (Outputs) --> Relay 1-4 Coil (+)
Relay Coils (-)              --> GND
```

**Critical:** Pin 9 must connect to +5V for flyback protection.

Test by pressing footswitches - each should click the corresponding relay.

### 6. MIDI Output

#### Simple Circuit
```
Arduino D1 (TX) --> [220Ω] --> DIN-5 Pin 5
Arduino GND     --> DIN-5 Pin 2
```

#### Optoisolated (MIDI Compliant)
```
+5V --> [220Ω] --> 6N138 Pin 2 (Anode)
6N138 Pin 3 (Cathode) --> [220Ω] --> Arduino D1 (TX)
+5V --> DIN-5 Pin 4
6N138 Pin 6 (Output) --> DIN-5 Pin 5
6N138 Pin 5, DIN-5 Pin 2 --> GND (isolated)
```

Test with MIDI monitor. Verify correct channel from DIP switches.

### 7. Audio Path

Each DPDT relay switches between effect loop and bypass:

```
Pole 1: COM (signal in) --> NO (effect send) or NC (bypass)
Pole 2: COM (signal out) --> NO (effect return) or NC (bypass)
```

When relay OFF: Signal through effect (COM → NO)
When relay ON: Bypassed (COM → NC, jumper NC1 to NC2)

Chain: Input → Loop 1 → Loop 2 → Loop 3 → Loop 4 → Output

## Testing Checklist

### Power-On
- [ ] Display shows MIDI channel (1 second)
- [ ] Display shows bank number
- [ ] All LEDs OFF initially
- [ ] Current <200mA (no relays active)

### Modes
- [ ] Manual: Single switch toggles relay + LED
- [ ] Bank: SW2+SW3 switches mode, SW1+SW2/SW3+SW4 change banks
- [ ] MIDI: Footswitch sends Program Change
- [ ] Edit: Hold SW2+SW3 (2s) enters edit, toggles work, save with SW2+SW3 (2s)

## Troubleshooting

| Issue | Check |
|-------|-------|
| Display blank | 5V power, SPI pins, ISET resistor, common-cathode display |
| LEDs wrong | Polarity, resistor values, `LED_ACTIVE_LOW` setting, OE pin grounded |
| Switches no response | Pullups enabled (software), wiring, continuity |
| Relays not switching | 5V power, ULN2003 connections, Pin 9 to +5V |
| No MIDI | Baud rate 31250, 220Ω resistor, DIP switches, D1 not shorted |
| High current | Short circuits, solder bridges, relay coil current |

---

**Version**: 1.0 | **Updated**: December 2024
