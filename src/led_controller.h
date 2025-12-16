#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

/**
 * LedController - Drives 8 status LEDs via 74HC595 shift register
 *
 * LED Assignment (Q0-Q7):
 * - Q0: Relay LED for Loop 1 (switch 0)
 * - Q1: Relay LED for Loop 2 (switch 1)
 * - Q2: Relay LED for Loop 3 (switch 2)
 * - Q3: Relay LED for Loop 4 (switch 3)
 * - Q4: Preset LED for Switch 1 / Preset 0
 * - Q5: Preset LED for Switch 2 / Preset 1
 * - Q6: Preset LED for Switch 3 / Preset 2
 * - Q7: Preset LED for Switch 4 / Preset 3
 *
 * Relay LEDs: Show currently applied loop states (what's driving the relays right now)
 * Preset LEDs: Show which preset is loaded (OFF in manual mode, ON for active preset otherwise)
 */
class LedController {
public:
  LedController(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, bool activeLow);

  void begin();

  /**
   * Update LED states based on current system state
   * @param appliedLoopStates The loop states currently applied to relays (4 bool array)
   * @param currentMode Current operating mode
   * @param activePreset Currently selected preset index (0-3, or -1 if none)
   * @param globalPresetActive True if global preset is active
   */
  void update(const bool* appliedLoopStates, Mode currentMode, int8_t activePreset, bool globalPresetActive);

private:
  uint8_t _dataPin;
  uint8_t _clockPin;
  uint8_t _latchPin;
  bool _activeLow;

  void shiftOut(uint8_t data);
};

#endif
