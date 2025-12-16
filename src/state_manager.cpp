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
    editModeAnimFrame(0),
    pcFlashStartTime(0),
    channelModeStartTime(0),
    editModeAnimTime(0),
    savedDisplayStartTime(0),
    flashingPC(0) {
}

void StateManager::initialize() {
  // Load MIDI channel from EEPROM
  uint8_t storedChannel = EEPROM.read(EEPROM_CHANNEL_ADDR);
  if (storedChannel >= 1 && storedChannel <= 16) {
    midiChannel = storedChannel;
  } else {
    // Invalid channel - set to default and write to EEPROM
    midiChannel = DEFAULT_MIDI_CHANNEL;
    EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  }

  // Check if EEPROM has been initialized
  uint8_t initFlag = EEPROM.read(EEPROM_INIT_FLAG_ADDR);
  if (initFlag != EEPROM_INIT_MAGIC) {
    // First boot - initialize all presets to 0 (all loops off)
    for (uint8_t i = 0; i < 128; i++) {
      EEPROM.write(EEPROM_PRESETS_START_ADDR + i, 0);
    }
    EEPROM.write(EEPROM_INIT_FLAG_ADDR, EEPROM_INIT_MAGIC);
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

void StateManager::savePreset(uint8_t presetNumber) {
  if (presetNumber < 1 || presetNumber > 128) return;

  // Pack 4 loop states into a single byte (bits 0-3)
  uint8_t packedState = 0;
  for (int i = 0; i < 4; i++) {
    if (loopStates[i]) {
      packedState |= (1 << i);
    }
  }

  // Only write to EEPROM if the value has changed (reduces wear)
  uint8_t currentValue = EEPROM.read(EEPROM_PRESETS_START_ADDR + presetNumber - 1);
  if (currentValue != packedState) {
    EEPROM.write(EEPROM_PRESETS_START_ADDR + presetNumber - 1, packedState);
  }
}

void StateManager::loadPreset(uint8_t presetNumber) {
  if (presetNumber < 1 || presetNumber > 128) return;

  // Read packed state from EEPROM
  uint8_t packedState = EEPROM.read(EEPROM_PRESETS_START_ADDR + presetNumber - 1);

  // Unpack into loop states
  for (int i = 0; i < 4; i++) {
    loopStates[i] = (packedState & (1 << i)) != 0;
  }
}
