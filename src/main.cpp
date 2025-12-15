/*
 * 4-Loop MIDI Switcher with Bank Mode
 * ATmega328 (Arduino Uno/Nano compatible)
 *
 * Hardware:
 * - 4 momentary footswitches (active LOW with pullups)
 * - MAX7219 7-segment display driver (3 digits)
 * - 4 DPDT relays for audio switching
 * - MIDI output on hardware UART
 */
#include <Arduino.h>

#include "config.h"
#include "midi_handler.h"
#include "display.h"
#include "switches.h"
#include "relays.h"
#include "state_manager.h"
#include "mode_controller.h"

// ===== PIN ARRAYS =====
const uint8_t switchPins[4] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};
const uint8_t relayPins[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// ===== HARDWARE INSTANCES =====
Display display(MAX_DIN_PIN, MAX_CLK_PIN, MAX_CS_PIN);
SwitchHandler switches(switchPins, DEBOUNCE_MS, SIMULTANEOUS_WINDOW_MS, LONG_PRESS_MS);
RelayController relays(relayPins);

// ===== STATE AND CONTROLLER =====
StateManager state;
ModeController modeController(state, switches, relays);

// ===== SETUP =====
void setup() {
  // Built-in LED for heartbeat (shares pin with SPI CLK, but that's OK for testing)
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize hardware modules
  switches.begin();
  relays.begin();
  display.begin();
  initMIDI();

  // Initialize state (loads MIDI channel from EEPROM)
  state.initialize();

  // Update initial display
  display.update(state.displayState, state.getDisplayValue(), state.loopStates, state.globalPresetActive);
}

// ===== MAIN LOOP =====
void loop() {
  switches.readAndDebounce();
  modeController.detectSwitchPatterns();
  modeController.updateStateMachine();

  // Update relays
  if (state.currentMode == MANUAL_MODE) {
    relays.update(state.loopStates);
  } else if (state.currentMode == EDIT_MODE) {
    // In edit mode, update relays with edit buffer states
    relays.update(state.editModeLoopStates);
  }

  // Update display
  display.update(
    state.displayState,
    state.getDisplayValue(),
    state.getDisplayLoops(),
    state.globalPresetActive);
}