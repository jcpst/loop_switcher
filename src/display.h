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
  
  // Display buffers to track current state
  uint8_t digitBuffer[8];      // Current digit values on display
  bool decimalBuffer[8];       // Current decimal point states
  bool bufferInitialized;      // Track if buffer has been initialized
  
  void setCharAt(uint8_t position, char c);
  void setCharAtBuffered(uint8_t position, char c, bool dp);
  void setDigitAtBuffered(uint8_t position, uint8_t digit, bool dp);
  void clearBuffered();
};

#endif
