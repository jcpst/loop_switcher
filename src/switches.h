#ifndef SWITCHES_H
#define SWITCHES_H

#include <Arduino.h>

struct SwitchState {
  bool currentState;
  bool lastState;
  unsigned long lastDebounceTime;
  unsigned long pressStartTime;
  bool longPressTriggered;
};

class SwitchHandler {
public:
  SwitchHandler(const uint8_t pins[4], uint8_t debounceMs, uint8_t simultaneousWindowMs, uint16_t longPressMs);

  void begin();
  void readAndDebounce();
  bool isRecentPress(uint8_t switchIndex) const;
  void clearRecentPresses();
  bool isPressed(uint8_t switchIndex) const;
  bool isLongPress(uint8_t sw1Index, uint8_t sw2Index);
  bool isLongPress(uint8_t sw1Index, uint8_t sw2Index, uint16_t customLongPressMs);
  const SwitchState* getStates() const;

private:
  const uint8_t* switchPins;
  uint8_t debounceMs;
  uint8_t simultaneousWindowMs;
  uint16_t longPressMs;
  SwitchState switches[4];
};

#endif
