#include "state_manager.h"
#include <EEPROM.h>

StateManager::StateManager() 
  : currentMode(MANUAL_MODE),
    displayState(SHOWING_MANUAL),
    currentBank(1),
    midiChannel(DEFAULT_MIDI_CHANNEL),
    loopStates{false, false, false, false},
    activePreset(-1),
    globalPresetActive(false),
    editModeLoopStates{false, false, false, false},
    editModeFlashState(false),
    pcFlashStartTime(0),
    channelModeStartTime(0),
    editModeFlashTime(0),
    savedDisplayStartTime(0),
    flashingPC(0) {
}

void StateManager::initialize() {
  // Load MIDI channel from EEPROM
  uint8_t storedChannel = EEPROM.read(EEPROM_CHANNEL_ADDR);
  if (storedChannel >= 1 && storedChannel <= 16) {
    midiChannel = storedChannel;
  } else {
    midiChannel = DEFAULT_MIDI_CHANNEL;
    EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  }
}

uint8_t StateManager::getDisplayValue() const {
  if (displayState == SHOWING_BANK || displayState == FLASHING_PC) {
    return (displayState == FLASHING_PC) ? flashingPC : currentBank;
  }
  return midiChannel;
}

bool* StateManager::getDisplayLoops() {
  return (currentMode == EDIT_MODE) ? editModeLoopStates : loopStates;
}
