#include "switch_handler.h"

SwitchHandler::SwitchHandler(uint8_t debounceMs,
                             uint16_t simultaneousWindowMs,
                             uint16_t longPressMs)
    : debounceMs(debounceMs),
      simultaneousWindowMs(simultaneousWindowMs),
      longPressMs(longPressMs) {
    reset();
}

void SwitchHandler::reset() {
    for (int i = 0; i < 4; i++) {
        switches[i].currentState = true;  // HIGH when not pressed (pullup)
        switches[i].lastState = true;
        switches[i].lastDebounceTime = 0;
        switches[i].pressStartTime = 0;
        switches[i].longPressTriggered = false;
    }
}

void SwitchHandler::update(const bool rawStates[4], unsigned long currentTime) {
    for (int i = 0; i < 4; i++) {
        bool reading = rawStates[i];

        // If the switch state changed (noise or actual press)
        if (reading != switches[i].lastState) {
            switches[i].lastDebounceTime = currentTime;
        }

        // If enough time has passed, accept the reading
        if ((currentTime - switches[i].lastDebounceTime) > debounceMs) {
            // If state actually changed
            if (reading != switches[i].currentState) {
                switches[i].currentState = reading;

                // On press (HIGH to LOW due to pullup)
                if (!reading) {
                    switches[i].pressStartTime = currentTime;
                    switches[i].longPressTriggered = false;
                }
            }
        }

        switches[i].lastState = reading;
    }
}

bool SwitchHandler::isRecentPress(uint8_t switchIndex, unsigned long currentTime) const {
    if (switchIndex >= 4) return false;

    // Check if button was pressed recently (within simultaneousWindowMs)
    if (switches[switchIndex].pressStartTime > 0) {
        const unsigned long timeSincePress = currentTime - switches[switchIndex].pressStartTime;
        return timeSincePress < simultaneousWindowMs;
    }

    return false;
}

void SwitchHandler::clearRecentPresses() {
    for (int i = 0; i < 4; i++) {
        switches[i].pressStartTime = 0;
    }
}

bool SwitchHandler::isPressed(uint8_t switchIndex) const {
    if (switchIndex >= 4) return false;
    return !switches[switchIndex].currentState;  // LOW = pressed
}

bool SwitchHandler::isLongPress(uint8_t sw1Index, uint8_t sw2Index, unsigned long currentTime) {
    return isLongPress(sw1Index, sw2Index, currentTime, longPressMs);
}

bool SwitchHandler::isLongPress(uint8_t sw1Index, uint8_t sw2Index, unsigned long currentTime, uint16_t customLongPressMs) {
    if (sw1Index >= 4 || sw2Index >= 4) return false;

    const bool switchesAreOn = !switches[sw1Index].currentState && !switches[sw2Index].currentState;
    const bool notTriggered = !switches[sw1Index].longPressTriggered && !switches[sw2Index].longPressTriggered;

    if (!switchesAreOn || !notTriggered) {
        return false;
    }

    // Use the LATER of the two press times to determine hold duration
    // This allows for a more natural press sequence
    const unsigned long laterPressTime = (switches[sw1Index].pressStartTime > switches[sw2Index].pressStartTime)
        ? switches[sw1Index].pressStartTime
        : switches[sw2Index].pressStartTime;

    const bool haveBeenHeldLongEnough = (currentTime - laterPressTime) > customLongPressMs;

    if (haveBeenHeldLongEnough) {
        // Mark as triggered to prevent repeated triggering
        switches[sw1Index].longPressTriggered = true;
        switches[sw2Index].longPressTriggered = true;
        return true;
    }

    return false;
}
