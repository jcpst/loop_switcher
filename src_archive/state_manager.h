#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "display.h"

class StateManager {
public:
  // Current mode and display
  Mode currentMode;
  DisplayState displayState;
  
  // Bank and MIDI
  uint8_t currentBank;
  uint8_t midiChannel;  // MIDI channel 0-15 (displayed as 1-16)
  
  // Loop states
  bool loopStates[4];
  
  // Preset tracking
  int8_t activePreset;
  bool globalPresetActive;
  
  // Edit mode
  bool editModeLoopStates[4];
  uint8_t editModeAnimFrame;

  // Saved display animation
  uint8_t savedDisplayAnimFrame;

  // Timing
  unsigned long pcFlashStartTime;
  unsigned long editModeAnimTime;
  unsigned long savedDisplayStartTime;
  unsigned long savedDisplayAnimTime;
  
  // Display state
  uint8_t flashingPC;
  
  StateManager();
  void initialize();
  uint8_t getDisplayValue() const;
  bool* getDisplayLoops();

  // EEPROM preset storage
  void savePreset(uint8_t presetNumber);
  void loadPreset(uint8_t presetNumber);

  // Read MIDI channel from DIP switches on footswitch pins
  uint8_t readMidiChannelFromHardware() const;
};

#endif
