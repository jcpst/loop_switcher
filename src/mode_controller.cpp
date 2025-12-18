#include "mode_controller.h"
#include "config.h"

ModeController::ModeController(StateManager& state, SwitchHandler& switches, RelayController& relays)
  : state(state), switches(switches), relays(relays) {
}

void ModeController::detectSwitchPatterns() {
  // Check for 2-second long press of center switches (edit mode)
  // Only allow in BANK_MODE when a preset is active
  if (state.currentMode == BANK_MODE && state.activePreset != -1) {
    if (switches.isLongPress(1, 2, EDIT_MODE_LONG_PRESS_MS)) {
      enterEditMode();
      return;
    }
  }

  // In edit mode, check for exit
  if (state.currentMode == EDIT_MODE) {
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
    if (state.currentMode == MANUAL_MODE) {
      DEBUG_PRINTLN("Mode change: MANUAL -> BANK");
      state.currentMode = BANK_MODE;
      state.displayState = SHOWING_BANK;
    } else if (state.currentMode == BANK_MODE) {
      DEBUG_PRINTLN("Mode change: BANK -> MANUAL");
      state.currentMode = MANUAL_MODE;
      state.displayState = SHOWING_MANUAL;
      // Clear global preset state when leaving bank mode
      state.globalPresetActive = false;
      state.activePreset = -1;
    }

    switches.clearRecentPresses();

    return;
  }

  // Right switches: Bank up (only in bank mode)
  if (state.currentMode == BANK_MODE && sw3Pressed && sw4Pressed) {
    state.currentBank++;
    if (state.currentBank > NUM_BANKS) state.currentBank = 1;
    DEBUG_PRINT("Bank change: ");
    DEBUG_PRINTLN(state.currentBank);
    state.displayState = SHOWING_BANK;

    // Clear global preset when changing banks
    state.globalPresetActive = false;
    state.activePreset = -1;
    switches.clearRecentPresses();

    return;
  }

  // Left switches: Bank down (only in bank mode)
  if (state.currentMode == BANK_MODE && sw1Pressed && sw2Pressed) {
    if (state.currentBank == 1) state.currentBank = NUM_BANKS;
    else state.currentBank--;
    DEBUG_PRINT("Bank change: ");
    DEBUG_PRINTLN(state.currentBank);
    state.displayState = SHOWING_BANK;

    // Clear global preset when changing banks
    state.globalPresetActive = false;
    state.activePreset = -1;
    switches.clearRecentPresses();

    return;
  }

  // Individual switch presses
  for (int i = 0; i < NUM_LOOPS; i++) {
    if (switches.isRecentPress(i)) {
      handleSingleSwitchPress(i);
      switches.clearRecentPresses();
      break;  // Only handle one at a time
    }
  }
}

void ModeController::enterEditMode() {
  DEBUG_PRINTLN("Mode change: BANK -> EDIT");
  state.currentMode = EDIT_MODE;

  // Copy current loop states to edit buffer
  for (int i = 0; i < NUM_LOOPS; i++) {
    state.editModeLoopStates[i] = state.loopStates[i];
  }

  state.displayState = EDIT_MODE_ANIMATED;
  state.editModeAnimTime = millis();
  state.editModeAnimFrame = 0;
}

void ModeController::exitEditMode() {
  DEBUG_PRINTLN("Mode change: EDIT -> BANK (saving)");
  // Copy edited states back to main loop states
  for (int i = 0; i < NUM_LOOPS; i++) {
    state.loopStates[i] = state.editModeLoopStates[i];
  }

  // Update relays immediately with new states
  relays.update(state.loopStates);
  // Calculate preset number and save to EEPROM
  uint8_t presetNumber = ((state.currentBank - 1) * PRESETS_PER_BANK) + state.activePreset + 1;
  state.savePreset(presetNumber);
  // Show "SAVED" message
  state.displayState = SHOWING_SAVED;
  state.savedDisplayStartTime = millis();
  state.currentMode = BANK_MODE;
}

void ModeController::handleSingleSwitchPress(uint8_t switchIndex) {
  if (state.currentMode == MANUAL_MODE) {
    // Toggle loop state
    state.loopStates[switchIndex] = !state.loopStates[switchIndex];
  }
  else if (state.currentMode == EDIT_MODE) {
    // Toggle loop state in edit buffer
    state.editModeLoopStates[switchIndex] = !state.editModeLoopStates[switchIndex];
  }
  else if (state.currentMode == BANK_MODE) {
    // Check if pressing the same switch as the active preset
    if (state.activePreset == switchIndex && !state.globalPresetActive) {
      // Activate global preset - send PC TOTAL_PRESETS (calculated as NUM_BANKS * PRESETS_PER_BANK)
      state.globalPresetActive = true;
      sendMIDIProgramChange(TOTAL_PRESETS, state.midiChannel);

      // Flash PC number on display
      state.flashingPC = TOTAL_PRESETS;
      state.pcFlashStartTime = millis();
      state.displayState = FLASHING_PC;
    }
    else {
      // Exit global preset if active, or just send normal PC
      state.globalPresetActive = false;
      state.activePreset = switchIndex;

      // Send MIDI Program Change
      uint8_t pc = ((state.currentBank - 1) * PRESETS_PER_BANK) + switchIndex + 1;
      sendMIDIProgramChange(pc, state.midiChannel);
      // Load preset from EEPROM and apply to relays
      state.loadPreset(pc);
      relays.update(state.loopStates);
      // Flash PC number on display
      state.flashingPC = pc;
      state.pcFlashStartTime = millis();
      state.displayState = FLASHING_PC;
    }
  }
}

void ModeController::updateStateMachine() {
  unsigned long now = millis();

  // Handle PC flash timeout
  if (state.displayState == FLASHING_PC) {
    if ((now - state.pcFlashStartTime) > PC_FLASH_MS) {
      state.displayState = SHOWING_BANK;
    }
  }

  // Handle edit mode animation
  if (state.currentMode == EDIT_MODE) {
    if ((now - state.editModeAnimTime) > EDIT_ANIM_INTERVAL_MS) {
      state.editModeAnimFrame = (state.editModeAnimFrame + 1) % 6;
      state.editModeAnimTime = now;
    }
  }

  // Handle saved display timeout
  if (state.displayState == SHOWING_SAVED) {
    if ((now - state.savedDisplayStartTime) > SAVED_DISPLAY_MS) {
      state.displayState = SHOWING_BANK;
    }
  }
}
