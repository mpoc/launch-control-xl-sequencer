#ifndef HARDWARE_MIDI_API_H
#define HARDWARE_MIDI_API_H

#include <Arduino.h>
#include <MIDI.h>
#include "MidiTypes.h"

// Create MIDI instance outside the class - this is necessary for the MIDI library
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiOut);

// Simplified interface for hardware MIDI communication
class HardwareMidiAPI {
public:
    // Initialize hardware MIDI with specified MIDI channel
    static void begin(byte channel = MIDI_CHANNEL_OMNI) {
        midiOut.begin(channel);
    }

    // Send note on message via hardware MIDI
    static void sendNoteOn(byte note, byte velocity, byte channel) {
        midiOut.sendNoteOn(note, velocity, channel);
    }

    // Send note off message via hardware MIDI
    static void sendNoteOff(byte note, byte velocity, byte channel) {
        midiOut.sendNoteOff(note, velocity, channel);
    }

    // Send control change message via hardware MIDI
    static void sendControlChange(byte control, byte value, byte channel) {
        midiOut.sendControlChange(control, value, channel);
    }

    // Send real-time message via hardware MIDI
    static void sendRealTime(byte message) {
        midiOut.sendRealTime((midi::MidiType)message);
    }

    // Read incoming hardware MIDI messages, returns true if message was processed
    static bool read() {
        return midiOut.read();
    }

    // Set callbacks for message types
    static void setHandleNoteOn(void (*callback)(byte channel, byte note, byte velocity)) {
        midiOut.setHandleNoteOn(callback);
    }

    static void setHandleNoteOff(void (*callback)(byte channel, byte note, byte velocity)) {
        midiOut.setHandleNoteOff(callback);
    }

    static void setHandleControlChange(void (*callback)(byte channel, byte control, byte value)) {
        midiOut.setHandleControlChange(callback);
    }
};

#endif
