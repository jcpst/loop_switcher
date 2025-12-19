#include "relays.h"

RelayController::RelayController(const uint8_t pins[4])
  : relayPins(pins) {
}

void RelayController::begin() {
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }
}

void RelayController::update(const bool loopStates[4]) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(relayPins[i], loopStates[i] ? HIGH : LOW);
  }
}

void RelayController::allOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(relayPins[i], LOW);
  }
}
