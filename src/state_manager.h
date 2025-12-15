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
  uint8_t midiChannel;
  
  // Loop states
  bool loopStates[4];
  
  // Preset tracking
  int8_t activePreset;
  bool globalPresetActive;
  
  // Edit mode
  bool editModeLoopStates[4];
  bool editModeFlashState;
  
  // Timing
  unsigned long pcFlashStartTime;
  unsigned long channelModeStartTime;
  unsigned long editModeFlashTime;
  unsigned long savedDisplayStartTime;
  
  // Display state
  uint8_t flashingPC;
  
  StateManager();
  void initialize();
  uint8_t getDisplayValue() const;
  bool* getDisplayLoops();
};

#endif
