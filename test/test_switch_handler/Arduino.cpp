#include "Arduino.h"
#include <string.h>

// Mock state
static unsigned long mock_millis = 0;
static uint8_t pin_states[20] = {0};  // Support up to 20 pins
static uint8_t pin_modes[20] = {0};

// Arduino function implementations
void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 20) {
        pin_modes[pin] = mode;
        // Set initial state for INPUT_PULLUP
        if (mode == INPUT_PULLUP) {
            pin_states[pin] = HIGH;
        }
    }
}

void digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < 20) {
        pin_states[pin] = value;
    }
}

uint8_t digitalRead(uint8_t pin) {
    if (pin < 20) {
        return pin_states[pin];
    }
    return LOW;
}

unsigned long millis(void) {
    return mock_millis;
}

void delay(unsigned long ms) {
    mock_millis += ms;
}

// Test helper functions
namespace ArduinoMock {
    void reset() {
        mock_millis = 0;
        memset(pin_states, 0, sizeof(pin_states));
        memset(pin_modes, 0, sizeof(pin_modes));
    }

    void setMillis(unsigned long ms) {
        mock_millis = ms;
    }

    void setPinState(uint8_t pin, uint8_t state) {
        if (pin < 20) {
            pin_states[pin] = state;
        }
    }

    uint8_t getPinState(uint8_t pin) {
        if (pin < 20) {
            return pin_states[pin];
        }
        return LOW;
    }

    uint8_t getPinMode(uint8_t pin) {
        if (pin < 20) {
            return pin_modes[pin];
        }
        return 0;
    }
}
