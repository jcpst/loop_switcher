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
#include "led_controller.h"

// ===== PIN ARRAYS =====
const uint8_t switchPins[4] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};
const uint8_t relayPins[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// ===== HARDWARE INSTANCES =====
Display display(MAX_DIN_PIN, MAX_CLK_PIN, MAX_CS_PIN);
SwitchHandler switches(switchPins, DEBOUNCE_MS, SIMULTANEOUS_WINDOW_MS, LONG_PRESS_MS);
RelayController relays(relayPins);
LedController leds(SR_DATA_PIN, SR_CLOCK_PIN, SR_LATCH_PIN, LED_ACTIVE_LOW);

// ===== STATE AND CONTROLLER =====
StateManager state;
ModeController modeController(state, switches, relays);

// ===== SETUP =====
void setup() {
  // Initialize hardware modules
  switches.begin();  // Must be called first - enables pullups for DIP switch reading
  relays.begin();
  display.begin();
  leds.begin();
  initMIDI();

  // Initialize state (reads MIDI channel from DIP switches on footswitch pins)
  state.initialize();

  // Show configured MIDI channel on display (convert 0-15 to 1-16 for display)
  display.displayChannel(state.midiChannel + 1);
  delay(CHANNEL_DISPLAY_MS);

  // Update initial display and LEDs
  display.update(state.displayState, state.getDisplayValue(), state.loopStates, state.globalPresetActive);
  leds.update(state.loopStates, state.currentMode, state.activePreset, state.globalPresetActive);
}

// ===== MAIN LOOP =====
void loop() {
  switches.readAndDebounce();
  modeController.detectSwitchPatterns();
  modeController.updateStateMachine();

  // Determine which loop states are currently applied to relays
  const bool* appliedLoopStates;
  if (state.currentMode == EDIT_MODE) {
    appliedLoopStates = state.editModeLoopStates;
    relays.update(state.editModeLoopStates);
  } else {
    appliedLoopStates = state.loopStates;
    relays.update(state.loopStates);
  }

  // Update LEDs with applied loop states
  leds.update(appliedLoopStates, state.currentMode, state.activePreset, state.globalPresetActive);

  // Update display
  display.update(
    state.displayState,
    state.getDisplayValue(),
    state.getDisplayLoops(),
    state.globalPresetActive,
    state.editModeAnimFrame);
}