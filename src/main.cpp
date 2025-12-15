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
#include <EEPROM.h>

#include "config.h"
#include "midi_handler.h"
#include "display.h"
#include "switches.h"
#include "relays.h"

// ===== PIN ARRAYS =====
const uint8_t switchPins[4] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};
const uint8_t relayPins[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// ===== HARDWARE INSTANCES =====
Display display(MAX_DIN_PIN, MAX_CLK_PIN, MAX_CS_PIN);
SwitchHandler switches(switchPins, DEBOUNCE_MS, SIMULTANEOUS_WINDOW_MS, LONG_PRESS_MS);
RelayController relays(relayPins);

// ===== GLOBAL STATE =====
Mode currentMode = MANUAL_MODE;
DisplayState displayState = SHOWING_MANUAL;

uint8_t currentBank = 1;
uint8_t midiChannel = DEFAULT_MIDI_CHANNEL;
bool loopStates[4] = {false, false, false, false};

unsigned long pcFlashStartTime = 0;
uint8_t flashingPC = 0;
unsigned long channelModeStartTime = 0;

// Global preset tracking
int8_t activePreset = -1;  // -1 = none, 0-3 = switch index
bool globalPresetActive = false;

// Edit mode tracking
bool editModeLoopStates[4] = {false, false, false, false};
unsigned long editModeFlashTime = 0;
bool editModeFlashState = false;
unsigned long savedDisplayStartTime = 0;

// ===== FORWARD DECLARATIONS =====
void detectSwitchPatterns();
void updateStateMachine();
void handleSingleSwitchPress(uint8_t switchIndex);
void enterChannelSetMode();
void exitChannelSetMode();
void enterEditMode();
void exitEditMode();

// ===== SETUP =====
void setup() {
  // Built-in LED for heartbeat (shares pin with SPI CLK, but that's OK for testing)
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize hardware modules
  switches.begin();
  relays.begin();
  display.begin();
  initMIDI();

  // Load MIDI channel from EEPROM
  uint8_t storedChannel = EEPROM.read(EEPROM_CHANNEL_ADDR);
  if (storedChannel >= 1 && storedChannel <= 16) {
    midiChannel = storedChannel;
  } else {
    midiChannel = DEFAULT_MIDI_CHANNEL;
    EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  }

  // Update initial display
  uint8_t displayValue = (displayState == SHOWING_BANK || displayState == FLASHING_PC) ?
                          (displayState == FLASHING_PC ? flashingPC : currentBank) : midiChannel;
  display.update(displayState, displayValue, loopStates, globalPresetActive);
}

// ===== MAIN LOOP =====
void loop() {
  switches.readAndDebounce();
  detectSwitchPatterns();
  updateStateMachine();

  // Update relays
  if (currentMode == MANUAL_MODE) {
    relays.update(loopStates);
  } else if (currentMode == EDIT_MODE) {
    // In edit mode, update relays with edit buffer states
    relays.update(editModeLoopStates);
  }

  // Update display
  uint8_t displayValue = (displayState == SHOWING_BANK || displayState == FLASHING_PC) ?
                          (displayState == FLASHING_PC ? flashingPC : currentBank) : midiChannel;
  bool* displayLoops = (currentMode == EDIT_MODE) ? editModeLoopStates : loopStates;
  display.update(displayState, displayValue, displayLoops, globalPresetActive);
}

// ===== SWITCH PATTERN DETECTION =====
void detectSwitchPatterns() {
  unsigned long now = millis();

  // Check for long press of outer switches (mode change to channel set)
  if (switches.isLongPress(0, 3)) {
    enterChannelSetMode();
    return;
  }

  // Don't process other patterns while holding outer switches
  if (switches.isPressed(0) && switches.isPressed(3)) {
    return;
  }

  // Check for 2-second long press of center switches (edit mode)
  // Only allow in BANK_MODE when a preset is active
  if (currentMode == BANK_MODE && activePreset != -1) {
    if (switches.isLongPress(1, 2, EDIT_MODE_LONG_PRESS_MS)) {
      enterEditMode();
      return;
    }
  }

  // In edit mode, check for exit
  if (currentMode == EDIT_MODE) {
    if (switches.isLongPress(1, 2, EDIT_MODE_LONG_PRESS_MS)) {
      exitEditMode();
      return;
    }

    // Don't process other patterns while holding center switches in edit mode
    if (switches.isPressed(1) && switches.isPressed(2)) {
      return;
    }
  }

  // Check for simultaneous presses (within window)
  bool sw1Pressed = switches.isRecentPress(0);
  bool sw2Pressed = switches.isRecentPress(1);
  bool sw3Pressed = switches.isRecentPress(2);
  bool sw4Pressed = switches.isRecentPress(3);

  // Center switches: toggle Manual/Bank mode
  if (sw2Pressed && sw3Pressed) {
    if (currentMode == MANUAL_MODE) {
      currentMode = BANK_MODE;
      displayState = SHOWING_BANK;
    } else if (currentMode == BANK_MODE) {
      currentMode = MANUAL_MODE;
      displayState = SHOWING_MANUAL;
      // Clear global preset state when leaving bank mode
      globalPresetActive = false;
      activePreset = -1;
    }
    switches.clearRecentPresses();
    return;
  }

  // Right switches: Bank up (only in bank mode)
  if (currentMode == BANK_MODE && sw3Pressed && sw4Pressed) {
    currentBank++;
    if (currentBank > 32) currentBank = 1;
    displayState = SHOWING_BANK;
    // Clear global preset when changing banks
    globalPresetActive = false;
    activePreset = -1;
    switches.clearRecentPresses();
    return;
  }

  // Left switches: Bank down (only in bank mode)
  if (currentMode == BANK_MODE && sw1Pressed && sw2Pressed) {
    if (currentBank == 1) currentBank = 32;
    else currentBank--;
    displayState = SHOWING_BANK;
    // Clear global preset when changing banks
    globalPresetActive = false;
    activePreset = -1;
    switches.clearRecentPresses();
    return;
  }

  // In channel set mode
  if (currentMode == CHANNEL_SET_MODE) {
    // Center switches: exit
    if (sw2Pressed && sw3Pressed) {
      exitChannelSetMode();
      switches.clearRecentPresses();
      return;
    }

    // Left switches: decrease channel
    if (sw1Pressed && sw2Pressed) {
      if (midiChannel == 1) midiChannel = 16;
      else midiChannel--;
      channelModeStartTime = now;
      switches.clearRecentPresses();
      return;
    }

    // Right switches: increase channel
    if (sw3Pressed && sw4Pressed) {
      if (midiChannel == 16) midiChannel = 1;
      else midiChannel++;
      channelModeStartTime = now;
      switches.clearRecentPresses();
      return;
    }

    // Timeout check
    if ((now - channelModeStartTime) > CHANNEL_TIMEOUT_MS) {
      exitChannelSetMode();
    }
    return;
  }

  // Individual switch presses
  for (int i = 0; i < 4; i++) {
    if (switches.isRecentPress(i)) {
      handleSingleSwitchPress(i);
      switches.clearRecentPresses();
      break;  // Only handle one at a time
    }
  }
}

// ===== EDIT MODE =====
void enterEditMode() {
  currentMode = EDIT_MODE;
  // Copy current loop states to edit buffer
  for (int i = 0; i < 4; i++) {
    editModeLoopStates[i] = loopStates[i];
  }
  displayState = EDIT_MODE_SHOWING;
  editModeFlashTime = millis();
  editModeFlashState = true;
}

void exitEditMode() {
  // Copy edited states back to main loop states
  for (int i = 0; i < 4; i++) {
    loopStates[i] = editModeLoopStates[i];
  }
  // Update relays immediately with new states
  relays.update(loopStates);

  // Show "SAVED" message
  displayState = SHOWING_SAVED;
  savedDisplayStartTime = millis();
  currentMode = BANK_MODE;
}

void handleSingleSwitchPress(uint8_t switchIndex) {
  if (currentMode == MANUAL_MODE) {
    // Toggle loop state
    loopStates[switchIndex] = !loopStates[switchIndex];
  }
  else if (currentMode == EDIT_MODE) {
    // Toggle loop state in edit buffer
    editModeLoopStates[switchIndex] = !editModeLoopStates[switchIndex];
  }
  else if (currentMode == BANK_MODE) {
    // Check if pressing the same switch as the active preset
    if (activePreset == switchIndex && !globalPresetActive) {
      // Activate global preset
      globalPresetActive = true;
      displayState = SHOWING_BANK;
      // Don't send MIDI, just activate global preset mode
    }
    else {
      // Exit global preset if active, or just send normal PC
      globalPresetActive = false;
      activePreset = switchIndex;

      // Send MIDI Program Change
      uint8_t pc = (currentBank * 4) + switchIndex + 1;
      sendMIDIProgramChange(pc, midiChannel);

      // Flash PC number on display
      flashingPC = pc;
      pcFlashStartTime = millis();
      displayState = FLASHING_PC;
    }
  }
}

// ===== MODE TRANSITIONS =====
void enterChannelSetMode() {
  currentMode = CHANNEL_SET_MODE;
  displayState = SHOWING_CHANNEL;
  channelModeStartTime = millis();
}

void exitChannelSetMode() {
  EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  currentMode = BANK_MODE;  // Return to bank mode
  displayState = SHOWING_BANK;
}

// ===== STATE MACHINE =====
void updateStateMachine() {
  unsigned long now = millis();

  // Handle PC flash timeout
  if (displayState == FLASHING_PC) {
    if ((now - pcFlashStartTime) > PC_FLASH_MS) {
      displayState = SHOWING_BANK;
    }
  }

  // Handle edit mode flashing
  if (currentMode == EDIT_MODE) {
    if ((now - editModeFlashTime) > EDIT_FLASH_INTERVAL_MS) {
      editModeFlashState = !editModeFlashState;
      displayState = editModeFlashState ? EDIT_MODE_SHOWING : EDIT_MODE_BLANK;
      editModeFlashTime = now;
    }
  }

  // Handle saved display timeout
  if (displayState == SHOWING_SAVED) {
    if ((now - savedDisplayStartTime) > SAVED_DISPLAY_MS) {
      displayState = SHOWING_BANK;
    }
  }

  // Handle edit mode flashing
  if (currentMode == EDIT_MODE) {
    if ((now - editModeFlashTime) > EDIT_FLASH_INTERVAL_MS) {
      editModeFlashState = !editModeFlashState;
      displayState = editModeFlashState ? EDIT_MODE_SHOWING : EDIT_MODE_BLANK;
      editModeFlashTime = now;
    }
  }
}