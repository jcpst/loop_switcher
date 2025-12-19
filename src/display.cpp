#include "display.h"

// Sentinel values for display buffer management
#define BLANK_VALUE 0xFE   // Represents a blank/cleared display position
#define INIT_VALUE 0xFF    // Initial invalid value to force first update

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
  clearBuffered();

  setCharAtBuffered(7, 'b', false);
  setCharAtBuffered(6, 'A', false);
  setCharAtBuffered(5, 'n', false);
  setCharAtBuffered(4, 'K', false);

  const uint8_t tens = num / 10;
  const uint8_t ones = num % 10;

  setDigitAtBuffered(1, tens, false);
  setDigitAtBuffered(0, ones, false);

  if (globalPreset) {
    setCharAtBuffered(3, '-', false);
  }
}

void Display::displayChannel(uint8_t ch) {
  clearBuffered();

  setCharAtBuffered(7, 'C', false);
  setCharAtBuffered(6, 'h', false);
  setCharAtBuffered(5, 'a', false);
  setCharAtBuffered(4, 'n', false);

  const uint8_t tens = ch / 10;
  const uint8_t ones = ch % 10;

  setDigitAtBuffered(1, tens, false);
  setDigitAtBuffered(0, ones, false);
}

void Display::displayEdit(uint8_t animFrame) {
  // Display "Edit" with scrolling decimal animation (E->d->i->t)
  clearBuffered();

  // Show "Edit" text with scrolling decimal from left to right
  setCharAtBuffered(5, 'E', animFrame == 0);
  setCharAtBuffered(4, 'd', animFrame == 1);
  setCharAtBuffered(3, 'i', animFrame == 2);
  setCharAtBuffered(2, 't', animFrame == 3);

  // Add trailing decimals for scroll effect
  if (animFrame == 4) setCharAtBuffered(1, ' ', true);
  if (animFrame == 5) setCharAtBuffered(0, ' ', true);
}

void Display::displaySaved() {
  // Display "SAvEd" centered on 8 digits
  clearBuffered();

  setCharAtBuffered(6, 'S', false);
  setCharAtBuffered(5, 'A', false);
  setCharAtBuffered(4, 'v', false);
  setCharAtBuffered(3, 'E', false);
  setCharAtBuffered(2, 'd', false);
}

void Display::displayManualStatus(const bool loopStates[4]) {
  clearBuffered();

  setCharAtBuffered(6, loopStates[3] ? '4' : '_', false);
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
