#ifndef MODE_CONTROLLER_H
#define MODE_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "state_manager.h"
#include "switches.h"
#include "relays.h"
#include "midi_handler.h"

class ModeController {
public:
  ModeController(StateManager& state, SwitchHandler& switches, RelayController& relays);
  
  void detectSwitchPatterns();
  void updateStateMachine();
  void handleSingleSwitchPress(uint8_t switchIndex);
  
private:
  StateManager& state;
  SwitchHandler& switches;
  RelayController& relays;
  
  void enterEditMode();
  void exitEditMode();
};

#endif
