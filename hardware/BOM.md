# Bill of Materials (BOM)
## 4-Loop MIDI Switcher

This BOM lists all components required to build the 4-Loop MIDI Switcher with MIDI control.

### Microcontroller
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 1 | Arduino Uno R3 / Nano | ATmega328P Development Board | 5V, 16MHz, 32KB Flash, 2KB SRAM | Either Uno or Nano works |

### Switches & User Interface
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 4 | - | Momentary Footswitch | SPST NO, momentary, guitar pedal style | Active LOW with internal pullup |
| 1 | - | 4-Position DIP Switch | Through-hole or SMD | Shares pins with footswitches |

### Relays & Drivers
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 4 | Omron G5V-2 / Panasonic TQ2 / Finder 40.52 / HK19F | DPDT Relay | 5V DC coil, 60mA, 2A contacts | True bypass audio switching |
| 1 | ULN2003A / ULN2803A | Darlington Transistor Array | 7/8 channel, 500mA per channel | Drives relay coils, has internal flyback diodes |

### Display System
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 1 | MAX7219CNG | LED Display Driver IC | SPI interface, drives 8 digits | Pre-assembled module recommended |
| 1 | - | 8-Digit 7-Segment LED Display | Common cathode | Often included with MAX7219 module |

### LED Status Indicators
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 1 | 74HC595 / SN74HC595N | 8-Bit Shift Register | CMOS, 25mA output | Drives 8 status LEDs |
| 8 | - | LED (3mm or 5mm) | Red, green, yellow, or blue | 4 relay LEDs + 4 preset LEDs |

### MIDI Interface
| Qty | Part Number | Description | Specifications | Notes |
|-----|-------------|-------------|----------------|-------|
| 1 | - | 5-Pin DIN Connector (Female) | 180° or 270° PCB mount | MIDI output jack |
| 1 | 6N138 / H11L1 (Optional) | Optoisolator | High-speed optocoupler | Optional, for MIDI compliance |

### Resistors
| Qty | Value | Type | Power | Notes |
|-----|-------|------|-------|-------|
| 8 | 220Ω - 470Ω | 1/4W | - | LED current limiting (one per LED) |
| 1 | 220Ω | 1/4W | - | MIDI output current limiting |
| 4 | 1kΩ | 1/4W | - | ULN2003 input protection (optional) |
| 1 | 10kΩ | 1/4W | - | MAX7219 ISET resistor (sets segment current) |
| 2 | 220Ω (if using optoisolator) | 1/4W | - | Optoisolator LED current limiting |

### Capacitors
| Qty | Value | Type | Voltage | Notes |
|-----|-------|------|---------|-------|
| 1 | 100-470µF | Electrolytic | 16V+ | Bulk power supply filtering |
| 4 | 100nF | Ceramic | 50V | Bypass caps for ICs (MAX7219, 74HC595, ULN2003, Arduino) |

### Diodes (Optional)
| Qty | Part Number | Description | Notes |
|-----|-------------|-------------|-------|
| 4 | 1N4148 / 1N4001 | Flyback Diode | Optional, ULN2003 has internal flyback diodes |

### Power Supply
| Qty | Description | Specifications | Notes |
|-----|-------------|----------------|-------|
| 1 | 5V DC Power Supply | 5V DC, 1-1.5A, regulated | 2.1mm barrel jack (center positive) or USB |

### Connectors & Hardware
| Qty | Description | Notes |
|-----|-------------|-------|
| 8 | 1/4" (6.35mm) Mono/TRS Jacks | For audio loop send/return connections |
| - | Wire, 22-24 AWG | Stranded wire for connections |
| - | Perfboard / PCB | For mounting components |
| - | Enclosure | Guitar pedal sized enclosure |
| - | Knobs / Hardware | Mounting screws, standoffs |

### Audio Path Components (Not Included)
These are part of the audio signal path and depend on your specific design:
- Input/output jacks
- Audio bypass switching (depends on relay wiring configuration)
- Jack wiring and shielded cable

## Component Notes

### Relay Selection
- **DPDT Configuration Required**: Each relay must have 2 poles, 2 throws (6 terminals)
- **Coil Voltage**: 5V DC to match Arduino power supply
- **Coil Current**: Typically 40-70mA per relay (check ULN2003 can handle 4x simultaneously)
- **Contact Rating**: 1-2A minimum for audio signals (guitar effects typically draw <100mA)

### LED Current Limiting Resistors
Calculate per LED based on color:
- **Red/Yellow LEDs**: Vf ≈ 2.0V → R = (5V - 2.0V) / 0.015A = 220Ω
- **Green LEDs**: Vf ≈ 2.2V → R = (5V - 2.2V) / 0.015A = 220Ω
- **Blue/White LEDs**: Vf ≈ 3.0V → R = (5V - 3.0V) / 0.015A = 150Ω (use 220Ω for dimmer)

### MAX7219 ISET Resistor
- **10kΩ**: ~12.8mA per segment, moderate brightness
- **4.7kΩ**: ~27mA per segment, very bright (maximum recommended)
- **22kΩ**: ~5.8mA per segment, dimmer (lower power)

### Optional Components
- **6N138 Optoisolator**: For full MIDI compliance and ground isolation
- **External flyback diodes**: ULN2003A has internal diodes, external ones add extra protection

## Power Budget

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| ATmega328P | 15mA | Active at 16MHz |
| MAX7219 + Display | 100-150mA | Depends on brightness setting |
| 74HC595 + 8 LEDs | 80-160mA | 10-20mA per LED |
| 4× Relay Coils | 240mA | 60mA each, all energized |
| ULN2003A | 5mA | Quiescent current |
| MIDI Output | 5mA | If optoisolated |
| **Total Typical** | **450-580mA** | **Recommend 1A supply** |
| **Idle (no relays)** | **200-330mA** | Display and MCU only |

## Sourcing Notes

### Recommended Suppliers
- **Electronics**: Digi-Key, Mouser, Arrow Electronics
- **Arduino**: Official Arduino store, SparkFun, Adafruit
- **Modules**: Amazon, AliExpress (MAX7219 module, relay modules)
- **Enclosure**: Tayda Electronics, Hammond Manufacturing

### Module Recommendations
Consider pre-assembled modules to simplify construction:
- **MAX7219 8-digit display module**: Complete with display and resistors (~$3-5)
- **ULN2003 relay driver board**: Pre-wired with screw terminals (~$2-3)
- **4-channel relay module**: Complete with relays, drivers, and LEDs (~$5-8)

### Budget Estimate
- **Minimum (using modules)**: $30-40
- **Component-level build**: $25-35
- **With quality enclosure and jacks**: $60-80

## Revision History
- **v1.0** (December 2024): Initial BOM
