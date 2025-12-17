#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== DEBUG CONFIGURATION =====
// Uncomment to enable debug output via Serial
// WARNING: Debug output interferes with MIDI TX (both use hardware Serial)
// Only enable during development when MIDI is not needed
// #define DEBUG_MODE

#ifdef DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// ===== PIN DEFINITIONS =====
// Footswitches (active LOW, internal pullup)
const uint8_t SW1_PIN = 2;
const uint8_t SW2_PIN = 4;
const uint8_t SW3_PIN = 5;
const uint8_t SW4_PIN = 6;

// Relay drivers (active HIGH)
const uint8_t RELAY1_PIN = 7;
const uint8_t RELAY2_PIN = 8;
const uint8_t RELAY3_PIN = 9;
const uint8_t RELAY4_PIN = 10;

// MAX7219 SPI (using LedControl library)
const uint8_t MAX_DIN_PIN = 11;
const uint8_t MAX_CLK_PIN = 13;
const uint8_t MAX_CS_PIN = 12;

// 74HC595 shift register for status LEDs
const uint8_t SR_DATA_PIN = A0;   // SER / DS - Serial data input
const uint8_t SR_CLOCK_PIN = A1;  // SHCP / SRCLK - Shift register clock
const uint8_t SR_LATCH_PIN = A2;  // STCP / RCLK - Storage register clock (latch)
const bool LED_ACTIVE_LOW = false; // Set true if LEDs wired: +5V -> resistor -> LED -> 74HC595 output

// MIDI uses hardware UART TX (pin 1 on Uno/Nano)

// ===== CONSTANTS =====
// System configuration
const uint8_t NUM_LOOPS = 4;
const uint8_t NUM_BANKS = 32;
const uint8_t PRESETS_PER_BANK = 4;
const uint8_t TOTAL_PRESETS = NUM_BANKS * PRESETS_PER_BANK;  // 128
const uint8_t MAIN_LOOP_INTERVAL_MS = 10;  // For 100Hz update rate

// Timing
const uint8_t DEBOUNCE_MS = 30;
const uint8_t SIMULTANEOUS_WINDOW_MS = 100;
const uint16_t LONG_PRESS_MS = 1000;
const uint16_t EDIT_MODE_LONG_PRESS_MS = 2000;
const uint16_t PC_FLASH_MS = 1000;
const uint16_t EDIT_ANIM_INTERVAL_MS = 150;
const uint16_t SAVED_DISPLAY_MS = 2000;
const uint16_t CHANNEL_DISPLAY_MS = 1000;

// EEPROM layout
// Address 0: Reserved (previously used for MIDI channel)
const uint8_t EEPROM_INIT_FLAG_ADDR = 1;
const uint8_t EEPROM_PRESETS_START_ADDR = 2;  // Presets 1-128 stored at addresses 2-129
const uint8_t EEPROM_INIT_MAGIC = 0x42;        // Magic byte to detect first boot

// Default MIDI channel 0-15 (used in constructor before hardware read in initialize())
const uint8_t DEFAULT_MIDI_CHANNEL = 0;

// ===== ENUMS =====
enum Mode {
  MANUAL_MODE,
  BANK_MODE,
  EDIT_MODE
};

#endif
