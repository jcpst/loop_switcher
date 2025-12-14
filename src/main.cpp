/*
 * 4-Loop MIDI Switcher with Bank Mode
 * ATmega328 (Arduino Uno/Nano compatible)
 *
 * Hardware:
 * - 4 momentary footswitches (active LOW with pullups)
 * - MAX7219 7-segment display driver (3 digits)
 * - 4 DPDT relays for audio switching
 * - MIDI output on hardware UART
 */
#include <Arduino.h>
#include <EEPROM.h>
#include <LedControl.h>

// ===== PIN DEFINITIONS =====
// Footswitches (active LOW, internal pullup)
const uint8_t SW1_PIN = 2;
const uint8_t SW2_PIN = 4;
const uint8_t SW3_PIN = 5;
const uint8_t SW4_PIN = 6;

// Relay drivers (active HIGH)
const uint8_t RELAY1_PIN = 7;
const uint8_t RELAY2_PIN = 8;
const uint8_t RELAY3_PIN = 9;
const uint8_t RELAY4_PIN = 10;

// MAX7219 SPI (using LedControl library)
const uint8_t MAX_DIN_PIN = 11;
const uint8_t MAX_CLK_PIN = 13;
const uint8_t MAX_CS_PIN = 12;

// MIDI uses hardware UART TX (pin 1 on Uno/Nano)

// ===== CONSTANTS =====
const uint32_t MIDI_BAUD = 31250;
const uint8_t DEBOUNCE_MS = 30;
const uint8_t SIMULTANEOUS_WINDOW_MS = 100;
const uint16_t LONG_PRESS_MS = 1000;
const uint16_t PC_FLASH_MS = 1000;
const uint16_t CHANNEL_TIMEOUT_MS = 5000;

const uint8_t EEPROM_CHANNEL_ADDR = 0;
const uint8_t DEFAULT_MIDI_CHANNEL = 1;

// ===== ENUMS =====
enum Mode {
  MANUAL_MODE,
  BANK_MODE,
  CHANNEL_SET_MODE
};

enum DisplayState {
  SHOWING_MANUAL,
  SHOWING_BANK,
  FLASHING_PC,
  SHOWING_CHANNEL
};

// ===== GLOBAL STATE =====
Mode currentMode = MANUAL_MODE;
DisplayState displayState = SHOWING_MANUAL;

uint8_t currentBank = 0;
uint8_t midiChannel = DEFAULT_MIDI_CHANNEL;
bool loopStates[4] = {false, false, false, false};

// Display control
LedControl lc = LedControl(MAX_DIN_PIN, MAX_CLK_PIN, MAX_CS_PIN, 1);
unsigned long pcFlashStartTime = 0;
uint8_t flashingPC = 0;
unsigned long channelModeStartTime = 0;

// Debug heartbeat
unsigned long lastHeartbeat = 0;
bool heartbeatState = false;

// Switch state tracking
struct SwitchState {
  bool currentState;
  bool lastState;
  unsigned long lastDebounceTime;
  unsigned long pressStartTime;
  bool longPressTriggered;
};

SwitchState switches[4];

// ===== FORWARD DECLARATIONS =====
void readAndDebounceAllSwitches();
void detectSwitchPatterns();
void updateStateMachine();
void updateRelays();
void updateDisplay();
bool isRecentPress(uint8_t switchIndex);
void clearRecentPresses();
void handleSingleSwitchPress(uint8_t switchIndex);
void enterChannelSetMode();
void exitChannelSetMode();
void displayNumber(uint8_t num);
void displayChannel(uint8_t ch);
void displayManualStatus();
void sendMIDIProgramChange(uint8_t program);

// ===== SETUP =====
void setup() {
  // Built-in LED for heartbeat (shares pin with SPI CLK, but that's OK for testing)
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize switch pins
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW3_PIN, INPUT_PULLUP);
  pinMode(SW4_PIN, INPUT_PULLUP);

  // Initialize relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // All relays off initially
  for (int i = 0; i < 4; i++) {
    digitalWrite(RELAY1_PIN + i, LOW);
  }

  // Initialize MAX7219
  lc.shutdown(0, false);  // Wake up display
  lc.setIntensity(0, 8);  // Medium brightness (0-15)
  lc.clearDisplay(0);

  // Initialize MIDI
  Serial.begin(MIDI_BAUD);

  // Load MIDI channel from EEPROM
  uint8_t storedChannel = EEPROM.read(EEPROM_CHANNEL_ADDR);
  if (storedChannel >= 1 && storedChannel <= 16) {
    midiChannel = storedChannel;
  } else {
    midiChannel = DEFAULT_MIDI_CHANNEL;
    EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  }

  // Initialize switch states
  for (int i = 0; i < 4; i++) {
    switches[i].currentState = true;  // Pullup = HIGH when not pressed
    switches[i].lastState = true;
    switches[i].lastDebounceTime = 0;
    switches[i].pressStartTime = 0;
    switches[i].longPressTriggered = false;
  }

  updateDisplay();
}

// ===== MAIN LOOP =====
void loop() {
  readAndDebounceAllSwitches();
  detectSwitchPatterns();
  updateStateMachine();
  updateRelays();
  updateDisplay();
}

// ===== SWITCH READING & DEBOUNCING =====
void readAndDebounceAllSwitches() {
  const uint8_t switchPins[4] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};
  unsigned long now = millis();

  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(switchPins[i]);

    // If the switch state changed (noise or actual press)
    if (reading != switches[i].lastState) {
      switches[i].lastDebounceTime = now;
    }

    // If enough time has passed, accept the reading
    if ((now - switches[i].lastDebounceTime) > DEBOUNCE_MS) {
      // If state actually changed
      if (reading != switches[i].currentState) {
        switches[i].currentState = reading;

        // On press (HIGH to LOW due to pullup)
        if (!reading) {
          switches[i].pressStartTime = now;
          switches[i].longPressTriggered = false;
        }
      }
    }

    switches[i].lastState = reading;
  }
}

// ===== SWITCH PATTERN DETECTION =====
void detectSwitchPatterns() {
  unsigned long now = millis();

  // Check for long press of outer switches (mode change to channel set)
  if (!switches[0].currentState && !switches[3].currentState) {
    if (!switches[0].longPressTriggered && !switches[3].longPressTriggered) {
      if ((now - switches[0].pressStartTime) > LONG_PRESS_MS &&
          (now - switches[3].pressStartTime) > LONG_PRESS_MS) {
        switches[0].longPressTriggered = true;
        switches[3].longPressTriggered = true;
        enterChannelSetMode();
      }
    }
    return;  // Don't process other patterns while holding these
  }

  // Check for simultaneous presses (within window)
  bool sw1Pressed = isRecentPress(0);
  bool sw2Pressed = isRecentPress(1);
  bool sw3Pressed = isRecentPress(2);
  bool sw4Pressed = isRecentPress(3);

  // Center switches: toggle Manual/Bank mode
  if (sw2Pressed && sw3Pressed) {
    if (currentMode == MANUAL_MODE) {
      currentMode = BANK_MODE;
      displayState = SHOWING_BANK;
    } else if (currentMode == BANK_MODE) {
      currentMode = MANUAL_MODE;
      displayState = SHOWING_MANUAL;
    }
    clearRecentPresses();
    return;
  }

  // Right switches: Bank up (only in bank mode)
  if (currentMode == BANK_MODE && sw3Pressed && sw4Pressed) {
    currentBank++;
    if (currentBank > 255) currentBank = 0;
    displayState = SHOWING_BANK;
    clearRecentPresses();
    return;
  }

  // Left switches: Bank down (only in bank mode)
  if (currentMode == BANK_MODE && sw1Pressed && sw2Pressed) {
    if (currentBank == 0) currentBank = 255;
    else currentBank--;
    displayState = SHOWING_BANK;
    clearRecentPresses();
    return;
  }

  // In channel set mode
  if (currentMode == CHANNEL_SET_MODE) {
    // Center switches: exit
    if (sw2Pressed && sw3Pressed) {
      exitChannelSetMode();
      clearRecentPresses();
      return;
    }

    // Left switches: decrease channel
    if (sw1Pressed && sw2Pressed) {
      if (midiChannel == 1) midiChannel = 16;
      else midiChannel--;
      channelModeStartTime = now;
      clearRecentPresses();
      return;
    }

    // Right switches: increase channel
    if (sw3Pressed && sw4Pressed) {
      if (midiChannel == 16) midiChannel = 1;
      else midiChannel++;
      channelModeStartTime = now;
      clearRecentPresses();
      return;
    }

    // Timeout check
    if ((now - channelModeStartTime) > CHANNEL_TIMEOUT_MS) {
      exitChannelSetMode();
    }
    return;
  }

  // Individual switch presses
  for (int i = 0; i < 4; i++) {
    if (isRecentPress(i)) {
      handleSingleSwitchPress(i);
      clearRecentPresses();
      break;  // Only handle one at a time
    }
  }
}

bool isRecentPress(uint8_t switchIndex) {
  if (!switches[switchIndex].currentState) {  // Currently pressed
    unsigned long pressDuration = millis() - switches[switchIndex].pressStartTime;
    return pressDuration < SIMULTANEOUS_WINDOW_MS;
  }
  return false;
}

void clearRecentPresses() {
  for (int i = 0; i < 4; i++) {
    switches[i].pressStartTime = 0;
  }
}

void handleSingleSwitchPress(uint8_t switchIndex) {
  if (currentMode == MANUAL_MODE) {
    // Toggle loop state
    loopStates[switchIndex] = !loopStates[switchIndex];
  }
  else if (currentMode == BANK_MODE) {
    // Send MIDI Program Change
    uint8_t pc = (currentBank * 4) + switchIndex + 1;
    sendMIDIProgramChange(pc);

    // Flash PC number on display
    flashingPC = pc;
    pcFlashStartTime = millis();
    displayState = FLASHING_PC;
  }
}

// ===== MODE TRANSITIONS =====
void enterChannelSetMode() {
  currentMode = CHANNEL_SET_MODE;
  displayState = SHOWING_CHANNEL;
  channelModeStartTime = millis();
}

void exitChannelSetMode() {
  EEPROM.write(EEPROM_CHANNEL_ADDR, midiChannel);
  currentMode = BANK_MODE;  // Return to bank mode
  displayState = SHOWING_BANK;
}

// ===== STATE MACHINE =====
void updateStateMachine() {
  unsigned long now = millis();

  // Handle PC flash timeout
  if (displayState == FLASHING_PC) {
    if ((now - pcFlashStartTime) > PC_FLASH_MS) {
      displayState = SHOWING_BANK;
    }
  }
}

// ===== RELAY CONTROL =====
void updateRelays() {
  if (currentMode == MANUAL_MODE) {
    digitalWrite(RELAY1_PIN, loopStates[0] ? HIGH : LOW);
    digitalWrite(RELAY2_PIN, loopStates[1] ? HIGH : LOW);
    digitalWrite(RELAY3_PIN, loopStates[2] ? HIGH : LOW);
    digitalWrite(RELAY4_PIN, loopStates[3] ? HIGH : LOW);
  }
  // In bank mode, relays stay off (or could be controlled by external MIDI gear)
}

// ===== DISPLAY CONTROL =====
void updateDisplay() {
  switch (displayState) {
    case SHOWING_MANUAL:
      // Show loop states as 4 segments or dashes
      displayManualStatus();
      break;

    case SHOWING_BANK:
      displayNumber(currentBank);
      break;

    case FLASHING_PC:
      displayNumber(flashingPC);
      break;

    case SHOWING_CHANNEL:
      displayChannel(midiChannel);
      break;
  }
}

void displayNumber(uint8_t num) {
  uint8_t hundreds = num / 100;
  uint8_t tens = (num / 10) % 10;
  uint8_t ones = num % 10;

  lc.setDigit(0, 0, ones, false);
  lc.setDigit(0, 1, tens, false);
  lc.setDigit(0, 2, hundreds, false);
}

void displayChannel(uint8_t ch) {
  // Show "C" and channel number (C 01 to C 16)
  uint8_t tens = ch / 10;
  uint8_t ones = ch % 10;

  lc.setDigit(0, 0, ones, false);
  lc.setDigit(0, 1, tens, false);
  lc.setChar(0, 2, 'C', false);
}

void displayManualStatus() {
  // Show dashes for inactive loops, numbers for active
  for (int i = 0; i < 3; i++) {
    if (i < 3) {  // We only have 3 digits
      if (loopStates[i]) {
        lc.setDigit(0, i, i + 1, false);
      } else {
        lc.setChar(0, i, '-', false);
      }
    }
  }
}

// ===== MIDI FUNCTIONS =====
void sendMIDIProgramChange(uint8_t program) {
  // Program Change: 0xC0 + channel (0-15), then program number (0-127)
  uint8_t statusByte = 0xC0 | ((midiChannel - 1) & 0x0F);
  uint8_t programByte = (program - 1) & 0x7F;  // PC 1-128 maps to 0-127

  Serial.write(statusByte);
  Serial.write(programByte);
}