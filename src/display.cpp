#include "display.h"

Display::Display(uint8_t dinPin, uint8_t clkPin, uint8_t csPin)
  : lc(dinPin, clkPin, csPin, 1) {
}

void Display::begin() {
  lc.shutdown(0, false);  // Wake up display
  lc.setIntensity(0, 8);  // Medium brightness (0-15)
  lc.clearDisplay(0);
}

void Display::update(DisplayState state, uint8_t value, bool loopStates[4], bool globalPreset) {
  switch (state) {
    case SHOWING_MANUAL:
      displayManualStatus(loopStates);
      break;

    case SHOWING_BANK:
      displayBankNumber(value, globalPreset);
      break;

    case FLASHING_PC:
      displayBankNumber(value, false);
      break;

    case SHOWING_SAVED:
      displaySaved();
      break;

    case SHOWING_CHANNEL:
      displayChannel(value);
      break;

    case EDIT_MODE_SHOWING:
      displayEdit();
      break;

    case EDIT_MODE_BLANK:
      clear();
      break;
  }
}

void Display::displayBankNumber(uint8_t num, bool globalPreset) {
  // Display "bAnK XX" or "bAnK XX*" on 8 digits (positions 7-0, left to right)
  clear();

  setCharAt(7, 'b');
  setCharAt(6, 'A');
  setCharAt(5, 'n');
  setCharAt(4, 'K');
  // Position 3 is space

  uint8_t tens = num / 10;
  uint8_t ones = num % 10;

  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 0, ones, false);

  // Show asterisk symbol on position 2 if global preset is active
  if (globalPreset) {
    // Asterisk pattern: segments A, F, G, B, C (top, top-left, middle, top-right, bottom-right)
    // This creates a star-like appearance on 7-segment display
    lc.setRow(0, 2, 0b01110011);  // Custom asterisk pattern
  }
}

void Display::displayChannel(uint8_t ch) {
  // Display "Ch XX" on 8 digits
  clear();

  setCharAt(7, 'C');
  setCharAt(6, 'h');
  // Positions 5-2 are space

  uint8_t tens = ch / 10;
  uint8_t ones = ch % 10;

  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 0, ones, false);
}

void Display::displayEdit() {
  // Display "Edit" centered on 8 digits
  clear();

  setCharAt(5, 'E');
  setCharAt(4, 'd');
  setCharAt(3, 'i');
  setCharAt(2, 't');
}

void Display::displaySaved() {
  // Display "SAvEd" centered on 8 digits
  clear();

  setCharAt(6, 'S');
  setCharAt(5, 'A');
  setCharAt(4, 'v');
  setCharAt(3, 'E');
  setCharAt(2, 'd');
}

void Display::displayManualStatus(bool loopStates[4]) {
  // Show loop states on rightmost 4 digits
  clear();

  for (int i = 0; i < 4; i++) {
    if (loopStates[i]) {
      lc.setDigit(0, i, i + 1, false);
    } else {
      lc.setChar(0, i, '-', false);
    }
  }
}

void Display::clear() {
  lc.clearDisplay(0);
}

void Display::setCharAt(uint8_t position, char c) {
  lc.setChar(0, position, c, false);
}
