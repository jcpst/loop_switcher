#ifndef SWITCH_HANDLER_H
#define SWITCH_HANDLER_H

#include <stdint.h>

/**
 * Switch state structure
 * Tracks the current and historical state of a single switch
 */
struct SwitchState {
    bool currentState;           // Current debounced state (LOW = pressed)
    bool lastState;              // Last raw reading
    unsigned long lastDebounceTime;  // Time of last state change
    unsigned long pressStartTime;    // Time when press was detected
    bool longPressTriggered;     // Has long press been triggered?
};

/**
 * SwitchHandler - Manages debouncing and pattern detection for 4 footswitches
 *
 * This class provides hardware-independent switch logic that can be unit tested.
 * It handles:
 * - Debouncing
 * - Recent press detection (for simultaneous presses)
 * - Long press detection
 */
class SwitchHandler {
public:
    /**
     * Constructor
     * @param debounceMs Time in ms for debounce delay
     * @param simultaneousWindowMs Time window for detecting simultaneous presses
     * @param longPressMs Time threshold for long press detection
     */
    SwitchHandler(uint8_t debounceMs = 30,
                  uint16_t simultaneousWindowMs = 400,
                  uint16_t longPressMs = 1000);

    /**
     * Update switch states based on raw readings
     * Call this regularly from your main loop
     * @param rawStates Array of 4 raw switch readings (LOW = pressed)
     * @param currentTime Current time in milliseconds
     */
    void update(const bool rawStates[4], unsigned long currentTime);

    /**
     * Clear all recent press timestamps
     * Call this after handling a simultaneous press pattern
     */
    void clearRecentPresses();

    /**
     * Check if a switch is currently pressed
     * @param switchIndex Switch to check (0-3)
     * @return true if switch is currently pressed
     */
    bool isPressed(uint8_t switchIndex) const;

    /**
     * Check if two switches have been held long enough for long press
     * @param sw1Index First switch (0-3)
     * @param sw2Index Second switch (0-3)
     * @param currentTime Current time in milliseconds
     * @return true if both switches held for longPressMs, false otherwise
     */
    bool isLongPress(uint8_t sw1Index, uint8_t sw2Index, unsigned long currentTime);

    /**
     * Check if two switches have been held long enough for long press (custom duration)
     * @param sw1Index First switch (0-3)
     * @param sw2Index Second switch (0-3)
     * @param currentTime Current time in milliseconds
     * @param customLongPressMs Custom long press threshold
     * @return true if both switches held for customLongPressMs
     */
    bool isLongPress(uint8_t sw1Index, uint8_t sw2Index, unsigned long currentTime, uint16_t customLongPressMs);

    /**
     * Check if a switch was pressed recently (within simultaneousWindowMs)
     * @param switchIndex Switch to check (0-3)
     * @param currentTime Current time in milliseconds
     * @return true if switch was pressed recently
     */
    bool isRecentPress(uint8_t switchIndex, unsigned long currentTime) const;

    /**
     * Get the current state of all switches
     * @return Pointer to array of 4 SwitchState structures
     */
    const SwitchState* getStates() const { return switches; }

    /**
     * Reset all switch states
     * Useful for testing
     */
    void reset();

private:
    uint8_t debounceMs;
    uint16_t simultaneousWindowMs;
    uint16_t longPressMs;
    SwitchState switches[4];
};

#endif // SWITCH_HANDLER_H
