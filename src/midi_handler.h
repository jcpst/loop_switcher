#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Arduino.h>

void initMIDI();
void sendMIDIProgramChange(uint8_t program, uint8_t channel);

#endif
