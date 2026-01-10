#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <stdint.h>
#include <stdlib.h>

// Pin modes
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// Digital pin values
#define LOW 0x0
#define HIGH 0x1

// Mock Arduino functions
extern "C" {
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);
    unsigned long millis(void);
    void delay(unsigned long ms);
}

// Test helper functions to control the mock
namespace ArduinoMock {
    void reset();
    void setMillis(unsigned long ms);
    void setPinState(uint8_t pin, uint8_t state);
    uint8_t getPinState(uint8_t pin);
    uint8_t getPinMode(uint8_t pin);
}

#endif // ARDUINO_MOCK_H
