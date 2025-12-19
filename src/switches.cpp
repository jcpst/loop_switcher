#include "switches.h"

SwitchHandler::SwitchHandler(const uint8_t pins[4], uint8_t debounceMs, uint8_t simultaneousWindowMs,
                             uint16_t longPressMs)
  : switchPins(pins), debounceMs(debounceMs), simultaneousWindowMs(simultaneousWindowMs), longPressMs(longPressMs) {
}

void SwitchHandler::begin() {
  for (int i = 0; i < 4; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
    switches[i].currentState = true; // Pullup = HIGH when not pressed
    switches[i].lastState = true;
    switches[i].lastDebounceTime = 0;
    switches[i].pressStartTime = 0;
    switches[i].longPressTriggered = false;
  }
}

void SwitchHandler::readAndDebounce() {
  unsigned long now = millis();

  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(switchPins[i]);

    // If the switch state changed (noise or actual press)
    if (reading != switches[i].lastState) {
      switches[i].lastDebounceTime = now;
    }

    // If enough time has passed, accept the reading
    if ((now - switches[i].lastDebounceTime) > debounceMs) {
      // If state actually changed
      if (reading != switches[i].currentState) {
        switches[i].currentState = reading;

        // On press (HIGH to LOW due to pullup)
        if (!reading) {
          switches[i].pressStartTime = now;
          switches[i].longPressTriggered = false;
        }
      }
    }

    switches[i].lastState = reading;
  }
}

bool SwitchHandler::isRecentPress(uint8_t switchIndex) const {
  if (!switches[switchIndex].currentState) {
    // Currently pressed
    const unsigned long pressDuration = millis() - switches[switchIndex].pressStartTime;
    return pressDuration < simultaneousWindowMs;
  }

  return false;
}

void SwitchHandler::clearRecentPresses() {
  for (int i = 0; i < 4; i++) {
    switches[i].pressStartTime = 0;
  }
}

bool SwitchHandler::isPressed(uint8_t switchIndex) const {
  return !switches[switchIndex].currentState;
}

bool SwitchHandler::isLongPress(uint8_t sw1Index, uint8_t sw2Index) {
  return isLongPress(sw1Index, sw2Index, longPressMs);
}

bool SwitchHandler::isLongPress(uint8_t sw1Index, uint8_t sw2Index, uint16_t customLongPressMs) {
  const unsigned long now = millis();

  const bool switchesAreOn = !switches[sw1Index].currentState && !switches[sw2Index].currentState;
  const bool notTriggered = !switches[sw1Index].longPressTriggered && !switches[sw2Index].longPressTriggered;
  const bool haveBeenHeldLongEnough = now - switches[sw1Index].pressStartTime > customLongPressMs &&
                                now - switches[sw2Index].pressStartTime > customLongPressMs;

  if (switchesAreOn && notTriggered && haveBeenHeldLongEnough) {
    switches[sw1Index].longPressTriggered = true;
    switches[sw2Index].longPressTriggered = true;

    return true;
  }

  return false;
}

const SwitchState *SwitchHandler::getStates() const {
  return switches;
}
