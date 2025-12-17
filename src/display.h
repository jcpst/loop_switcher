#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LedControl.h>

enum DisplayState {
  SHOWING_MANUAL,
  SHOWING_BANK,
  FLASHING_PC,
  SHOWING_SAVED,
  EDIT_MODE_ANIMATED
};

class Display {
public:
  Display(uint8_t dinPin, uint8_t clkPin, uint8_t csPin);

  void begin();
  void update(DisplayState state, uint8_t value, bool loopStates[4], bool globalPreset = false, uint8_t animFrame = 0);
  void displayBankNumber(uint8_t num, bool globalPreset = false);
  void displayChannel(uint8_t ch);
  void displayManualStatus(bool loopStates[4]);
  void displayEdit(uint8_t animFrame);
  void displaySaved();
  void clear();

private:
  LedControl lc;
  void setCharAt(uint8_t position, char c);
};

#endif
