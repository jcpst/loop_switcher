#include "state_manager.h"
#include "config.h"
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
    editModeAnimTime(0),
    savedDisplayStartTime(0),
    flashingPC(0) {
}

uint8_t StateManager::readMidiChannelFromHardware() const {
  // Read 4-bit binary value from footswitch pins (used as DIP switch inputs during setup)
  // SW1=bit0, SW2=bit1, SW3=bit2, SW4=bit3
  // Binary value 0-15 represents MIDI channels 0-15 (displayed as 1-16)
  uint8_t binaryValue = 0;
  
  // Read each pin - switches are active LOW with pullups
  // So if DIP switch is ON (connecting to ground), pin reads LOW (0)
  // If DIP switch is OFF (open), pin reads HIGH (1) due to pullup
  // We want ON switch to contribute to binary value, so invert the reading
  if (digitalRead(SW1_PIN) == LOW) binaryValue |= (1 << 0);  // Bit 0
  if (digitalRead(SW2_PIN) == LOW) binaryValue |= (1 << 1);  // Bit 1
  if (digitalRead(SW3_PIN) == LOW) binaryValue |= (1 << 2);  // Bit 2
  if (digitalRead(SW4_PIN) == LOW) binaryValue |= (1 << 3);  // Bit 3
  
  // Return MIDI channel 0-15
  return binaryValue;
}

void StateManager::initialize() {
  // Read MIDI channel from DIP switches on footswitch pins
  midiChannel = readMidiChannelFromHardware();
  DEBUG_PRINT("MIDI channel set to: ");
  DEBUG_PRINTLN(midiChannel + 1);

  // Check if EEPROM has been initialized
  const uint8_t initFlag = EEPROM.read(EEPROM_INIT_FLAG_ADDR);
  if (initFlag != EEPROM_INIT_MAGIC) {
    DEBUG_PRINTLN("First boot - initializing EEPROM");
    // First boot - initialize all presets to 0 (all loops off)
    for (uint8_t i = 0; i < TOTAL_PRESETS; i++) {
      EEPROM.write(EEPROM_PRESETS_START_ADDR + i, 0);
    }
    EEPROM.write(EEPROM_INIT_FLAG_ADDR, EEPROM_INIT_MAGIC);
  } else {
    DEBUG_PRINTLN("EEPROM already initialized");
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
  if (presetNumber < 1 || presetNumber > TOTAL_PRESETS) return;

  // Pack loop states into a single byte (bits 0-(NUM_LOOPS-1))
  uint8_t packedState = 0;
  for (int i = 0; i < NUM_LOOPS; i++) {
    if (loopStates[i]) {
      packedState |= (1 << i);
    }
  }

  // Only write to EEPROM if the value has changed (reduces wear)
  const uint8_t currentValue = EEPROM.read(EEPROM_PRESETS_START_ADDR + presetNumber - 1);
  if (currentValue != packedState) {
    DEBUG_PRINT("Saving preset ");
    DEBUG_PRINT(presetNumber);
    DEBUG_PRINT(" with state: 0x");
    DEBUG_PRINTLN(packedState, HEX);
    EEPROM.write(EEPROM_PRESETS_START_ADDR + presetNumber - 1, packedState);
  }
}

void StateManager::loadPreset(uint8_t presetNumber) {
  if (presetNumber < 1 || presetNumber > TOTAL_PRESETS) return;

  // Read packed state from EEPROM
  const uint8_t packedState = EEPROM.read(EEPROM_PRESETS_START_ADDR + presetNumber - 1);

  DEBUG_PRINT("Loading preset ");
  DEBUG_PRINT(presetNumber);
  DEBUG_PRINT(" with state: 0x");
  DEBUG_PRINTLN(packedState, HEX);

  // Unpack into loop states
  for (int i = 0; i < NUM_LOOPS; i++) {
    loopStates[i] = (packedState & (1 << i)) != 0;
  }
}
