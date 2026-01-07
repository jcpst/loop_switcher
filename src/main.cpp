/**
 * 4-Loop MIDI Switcher
 * ATmega328 (Arduino Uno/Nano)
 *
 * Step 2: Relay hardware validation
 * Cycles through each relay with 500ms delay to verify all 4 click.
 */

#include <Arduino.h>

// ===== PIN DEFINITIONS =====
// Footswitches (active LOW, internal pullup)
const uint8_t SW_PINS[4] = {2, 4, 5, 6};

// Relay drivers (active HIGH)
const uint8_t RELAY_PINS[4] = {7, 8, 9, 10};

// Number of loops/relays
const uint8_t NUM_LOOPS = 4;

// ===== SETUP =====
void setup() {
  // Initialize relay pins as outputs, start LOW (off)
  for (uint8_t i = 0; i < NUM_LOOPS; i++) {
    digitalWrite(RELAY_PINS[i], LOW);
    pinMode(RELAY_PINS[i], OUTPUT);
  }

  // Initialize switch pins with internal pullups
  for (uint8_t i = 0; i < NUM_LOOPS; i++) {
    pinMode(SW_PINS[i], INPUT_PULLUP);
  }
}

// ===== MAIN LOOP =====
void loop() {
  // Cycle through each relay: turn on, wait, turn off
  for (uint8_t i = 0; i < NUM_LOOPS; i++) {
    digitalWrite(RELAY_PINS[i], HIGH);
    delay(500);
    digitalWrite(RELAY_PINS[i], LOW);
    delay(500);
  }
}
