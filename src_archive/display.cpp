#include "display.h"

// Sentinel values for display buffer management
#define BLANK_VALUE 0xFE   // Represents a blank/cleared display position
#define INIT_VALUE 0xFF    // Initial invalid value to force first update

const byte n = 0b00010101;
const byte t = 0b00001111;

/*
 * These are the printable characters:
 *  0 1 2 3 4 5 6 7 8 9
    A a (prints upper case)
    B b (prints lower case)
    C c (prints lower case)
    D d (prints lower case)
    E e (prints upper case)
    F f (prints upper case)
    H h (prints upper case)
    L l (prints upper case)
    P p (prints upper case)
    - (the minus sign)
    . , (lights up the decimal-point)
    _ (the underscore)
    <SPACE> (the blank or space char)
 */
Display::Display(uint8_t dinPin, uint8_t clkPin, uint8_t csPin)
  : lc(dinPin, clkPin, csPin, 1), bufferInitialized(false) {
  // Initialize buffers to force initial update
  for (uint8_t i = 0; i < DISPLAY_DIGITS; i++) {
    digitBuffer[i] = INIT_VALUE;  // Force update on first use
    isDigitBuffer[i] = false;
    decimalBuffer[i] = false;
  }
}

void Display::begin() {
  lc.shutdown(0, false);  // Wake up display
  lc.setIntensity(0, 8);  // Medium brightness (0-15)
  lc.clearDisplay(0);
  bufferInitialized = true;
}

// Buffered setChar - only updates if value changed
void Display::setCharAtBuffered(uint8_t position, char c, bool dp) {
  const uint8_t charValue = (uint8_t)c;
  
  // Update only if the character, type, or decimal point changed (or buffer not initialized)
  if (!bufferInitialized || digitBuffer[position] != charValue || isDigitBuffer[position] || decimalBuffer[position] != dp) {
    lc.setChar(0, position, c, dp);
    digitBuffer[position] = charValue;
    isDigitBuffer[position] = false;  // Mark as character, not digit
    decimalBuffer[position] = dp;
  }
}

// Buffered setDigit - only updates if value changed
void Display::setDigitAtBuffered(uint8_t position, uint8_t digit, bool dp) {
  // Update only if the digit, type, or decimal point changed (or buffer not initialized)
  if (!bufferInitialized || digitBuffer[position] != digit || !isDigitBuffer[position] || decimalBuffer[position] != dp) {
    lc.setDigit(0, position, digit, dp);
    digitBuffer[position] = digit;
    isDigitBuffer[position] = true;  // Mark as digit, not character
    decimalBuffer[position] = dp;
  }
}

// Buffered clear - only clears positions that aren't already blank
void Display::clearBuffered() {
  if (!bufferInitialized) {
    lc.clearDisplay(0);
    // Mark all positions as blank in buffer
    for (uint8_t i = 0; i < DISPLAY_DIGITS; i++) {
      digitBuffer[i] = BLANK_VALUE;
      isDigitBuffer[i] = false;
      decimalBuffer[i] = false;
    }
    return;
  }
  
  for (uint8_t i = 0; i < DISPLAY_DIGITS; i++) {
    if (digitBuffer[i] != BLANK_VALUE) {
      lc.setChar(0, i, ' ', false);
      digitBuffer[i] = BLANK_VALUE;
      isDigitBuffer[i] = false;
      decimalBuffer[i] = false;
    }
  }
}

void Display::update(DisplayState state, uint8_t value, const bool loopStates[4], bool globalPreset, uint8_t animFrame) {
  switch (state) {
    case SHOWING_MANUAL:
      displayManualStatus(loopStates);
      break;

    case SHOWING_BANK:
      displayBankNumber(value, globalPreset);
      break;

    case FLASHING_PC:
      displayProgramChange(value);
      break;

    case SHOWING_SAVED:
      displaySaved(animFrame);
      break;

    case EDIT_MODE_ANIMATED:
      displayEdit(animFrame);
      break;
  }
}

void Display::displayBankNumber(uint8_t num, bool globalPreset) {
  clearBuffered();

  setCharAtBuffered(7, 'b', false);
  setCharAtBuffered(6, 'A', false);
  setCharAtBuffered(4, ' ', false);

  const uint8_t tens = num / 10;
  const uint8_t ones = num % 10;

  setDigitAtBuffered(1, tens, false);
  setDigitAtBuffered(0, ones, false);

  if (globalPreset) {
    setCharAtBuffered(2, '-', false);
  }

  // Custom bit pattern for lowercase 'n' - set this LAST
  lc.setRow(0, 5, n);
}

void Display::displayProgramChange(uint8_t num) {
  clearBuffered();

  const uint8_t hundreds = num / 100;
  const uint8_t tens = (num / 10) % 10;
  const uint8_t ones = num % 10;

  setDigitAtBuffered(2, hundreds, false);
  setDigitAtBuffered(1, tens, false);
  setDigitAtBuffered(0, ones, false);
}

void Display::displayChannel(uint8_t ch) {
  // Directly write to hardware without buffering
  lc.clearDisplay(0);

  lc.setChar(0, 7, 'C', false);
  lc.setChar(0, 6, 'H', false);
  lc.setChar(0, 5, 'A', false);

  const uint8_t tens = ch / 10;
  const uint8_t ones = ch % 10;

  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 0, ones, false);

  // Custom bit pattern for lowercase 'n' - set this LAST
  lc.setRow(0, 4, n);
}

void Display::displayEdit(uint8_t animFrame) {
  // Display "Edit" with scrolling decimal animation (E->d->i->t)
  clearBuffered();

  // Show "Edit" text with scrolling decimal from left to right (Ed17)
  setCharAtBuffered(5, 'E', animFrame == 0);
  setCharAtBuffered(4, 'd', animFrame == 1);
  setDigitAtBuffered(3, 1, animFrame == 2);

  // Add trailing decimals for scroll effect
  if (animFrame == 4) setCharAtBuffered(1, ' ', true);
  if (animFrame == 5) setCharAtBuffered(0, ' ', true);

  lc.setRow(0, 2, t);
}

void Display::displaySaved(uint8_t animFrame) {
  // Flash all decimals 3 times: 200ms on, 200ms off
  // animFrame 0,2,4 = decimals on; animFrame 1,3,5 = decimals off
  clearBuffered();

  const bool decimalsOn = (animFrame % 2 == 0);

  // Set all positions to blank with decimal point controlled by animFrame
  for (uint8_t i = 0; i < DISPLAY_DIGITS; i++) {
    setCharAtBuffered(i, ' ', decimalsOn);
  }
}

void Display::displayManualStatus(const bool loopStates[4]) {
  clearBuffered();

  setCharAtBuffered(6, loopStates[3] ? '4' : '_', false);
  setCharAtBuffered(5, ' ', false);  // Explicitly clear position 5 (where 'n' from bank appears)
  setCharAtBuffered(4, loopStates[2] ? '3' : '_', false);
  setCharAtBuffered(2, loopStates[1] ? '2' : '_', false);
  setCharAtBuffered(0, loopStates[0] ? '1' : '_', false);
}

void Display::clear() {
  clearBuffered();
}

void Display::setCharAt(uint8_t position, char c) {
  setCharAtBuffered(position, c, false);
}
