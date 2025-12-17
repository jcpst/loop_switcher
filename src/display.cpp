#include "display.h"

Display::Display(uint8_t dinPin, uint8_t clkPin, uint8_t csPin)
  : lc(dinPin, clkPin, csPin, 1) {
}

void Display::begin() {
  lc.shutdown(0, false);  // Wake up display
  lc.setIntensity(0, 8);  // Medium brightness (0-15)
  lc.clearDisplay(0);
}

void Display::update(DisplayState state, uint8_t value, bool loopStates[4], bool globalPreset, uint8_t animFrame) {
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

    case EDIT_MODE_ANIMATED:
      displayEdit(animFrame);
      break;
  }
}

void Display::displayBankNumber(uint8_t num, bool globalPreset) {
  clear();

  setCharAt(7, 'b');
  setCharAt(6, 'A');
  setCharAt(5, 'n');
  setCharAt(4, 'K');

  uint8_t tens = num / 10;
  uint8_t ones = num % 10;

  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 0, ones, false);

  if (globalPreset) {
    lc.setChar(0, 3, '-', false);
  }
}

void Display::displayChannel(uint8_t ch) {
  clear();

  setCharAt(7, 'C');
  setCharAt(6, 'h');
  setCharAt(5, 'a');
  setCharAt(4, 'n');

  uint8_t tens = ch / 10;
  uint8_t ones = ch % 10;

  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 0, ones, false);
}

void Display::displayEdit(uint8_t animFrame) {
  // Display "Edit" with scrolling decimal animation (E->d->i->t)
  clear();

  // Show "Edit" text with scrolling decimal from left to right
  lc.setChar(0, 5, 'E', animFrame == 0);
  lc.setChar(0, 4, 'd', animFrame == 1);
  lc.setChar(0, 3, 'i', animFrame == 2);
  lc.setChar(0, 2, 't', animFrame == 3);

  // Add trailing decimals for scroll effect
  if (animFrame == 4) lc.setChar(0, 1, ' ', true);
  if (animFrame == 5) lc.setChar(0, 0, ' ', true);
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
  clear();

  setCharAt(6, loopStates[3] ? '4' : '_');
  setCharAt(4, loopStates[2] ? '3' : '_');
  setCharAt(2, loopStates[1] ? '2' : '_');
  setCharAt(0, loopStates[0] ? '1' : '_');
}

void Display::clear() {
  lc.clearDisplay(0);
}

void Display::setCharAt(uint8_t position, char c) {
  lc.setChar(0, position, c, false);
}
