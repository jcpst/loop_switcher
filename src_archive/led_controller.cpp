#include "led_controller.h"

LedController::LedController(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, bool activeLow)
  : _dataPin(dataPin), _clockPin(clockPin), _latchPin(latchPin), _activeLow(activeLow) {
}

void LedController::begin() {
  pinMode(_dataPin, OUTPUT);
  pinMode(_clockPin, OUTPUT);
  pinMode(_latchPin, OUTPUT);

  digitalWrite(_dataPin, LOW);
  digitalWrite(_clockPin, LOW);
  digitalWrite(_latchPin, LOW);

  // Initialize all LEDs to off
  shiftOut(0x00);
}

void LedController::update(const bool* appliedLoopStates, Mode currentMode, int8_t activePreset, bool globalPresetActive) {
  uint8_t outputByte = 0;

  // Bits 0-3: Relay LEDs (show currently applied loop states)
  for (uint8_t i = 0; i < 4; i++) {
    if (appliedLoopStates[i]) {
      outputByte |= (1 << i);
    }
  }

  // Bits 4-7: Preset LEDs
  // - In MANUAL_MODE: all preset LEDs OFF
  // - When global preset is active: all preset LEDs OFF
  // - In other modes: light the LED for activePreset (if valid 0-3)
  if (currentMode != MANUAL_MODE && !globalPresetActive && activePreset >= 0 && activePreset <= 3) {
    outputByte |= (1 << (4 + activePreset));
  }

  // Apply polarity inversion if LEDs are wired active-low
  if (_activeLow) {
    outputByte = ~outputByte;
  }

  shiftOut(outputByte);
}

void LedController::shiftOut(uint8_t data) {
  // Set latch low to begin data transfer
  digitalWrite(_latchPin, LOW);

  // Shift out 8 bits, MSB first
  for (int8_t i = 7; i >= 0; i--) {
    digitalWrite(_clockPin, LOW);
    digitalWrite(_dataPin, (data >> i) & 0x01);
    digitalWrite(_clockPin, HIGH);
  }

  // Set latch high to update outputs
  digitalWrite(_latchPin, HIGH);
}
