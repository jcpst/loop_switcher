#include "midi_handler.h"
#include "config.h"

const uint32_t MIDI_BAUD = 31250;

void initMIDI() {
  Serial.begin(MIDI_BAUD);
  DEBUG_PRINTLN("MIDI initialized at 31250 baud");
}

void sendMIDIProgramChange(uint8_t program, uint8_t channel) {
  // Program Change: 0xC0 + channel (0-15), then program number (0-127)
  // channel: MIDI channel 0-15
  // program: Program number 1-128 (displayed), maps to MIDI 0-127
  const uint8_t statusByte = 0xC0 | (channel & 0x0F);
  const uint8_t programByte = (program - 1) & 0x7F;  // PC 1-128 maps to 0-127

  DEBUG_PRINT("MIDI PC: ");
  DEBUG_PRINT(program);
  DEBUG_PRINT(" on channel ");
  DEBUG_PRINTLN(channel + 1);

  Serial.write(statusByte);
  Serial.write(programByte);
}
