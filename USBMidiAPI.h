#ifndef USB_MIDI_API_H
#define USB_MIDI_API_H

#include <Arduino.h>
#include "MidiTypes.h"

// Simplified interface for USB MIDI communication with computer
class USBMidiAPI {
public:
    // Initialize USB MIDI
    static void begin() {
        usbMIDI.begin();
    }

    // Send note on message to computer
    static void sendNoteOn(byte note, byte velocity, byte channel) {
        usbMIDI.sendNoteOn(note, velocity, channel);
    }

    // Send note off message to computer
    static void sendNoteOff(byte note, byte velocity, byte channel) {
        usbMIDI.sendNoteOff(note, velocity, channel);
    }

    // Send control change message to computer
    static void sendControlChange(byte control, byte value, byte channel) {
        usbMIDI.sendControlChange(control, value, channel);
    }

    // Send real-time message to computer
    static void sendRealTime(byte message) {
        usbMIDI.sendRealTime(message);
    }

    // Read incoming USB MIDI messages, returns true if message was processed
    static bool read() {
        return usbMIDI.read();
    }

    // Set callbacks for message types
    static void setHandleNoteOn(void (*callback)(byte channel, byte note, byte velocity)) {
        usbMIDI.setHandleNoteOn(callback);
    }

    static void setHandleNoteOff(void (*callback)(byte channel, byte note, byte velocity)) {
        usbMIDI.setHandleNoteOff(callback);
    }

    static void setHandleControlChange(void (*callback)(byte channel, byte control, byte value)) {
        usbMIDI.setHandleControlChange(callback);
    }
};

#endif
