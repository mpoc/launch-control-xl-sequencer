#ifndef MIDI_TYPES_H
#define MIDI_TYPES_H

// Common MIDI message types and structures
struct MidiNote {
    byte channel;
    byte note;
    byte velocity;
};

struct MidiCC {
    byte channel;
    byte control;
    byte value;
};

// Common constants
const byte MIDI_NOTE_ON = 0x90;
const byte MIDI_NOTE_OFF = 0x80;
const byte MIDI_CC = 0xB0;

#endif
