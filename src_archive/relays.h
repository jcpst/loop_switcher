#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>

class RelayController {
public:
  RelayController(const uint8_t pins[4]);

  void begin();
  void update(const bool loopStates[4]);
  void allOff();

private:
  const uint8_t* relayPins;
};

#endif
