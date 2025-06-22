#ifndef MIDI_ROUTER_H
#define MIDI_ROUTER_H

#include <Arduino.h>
#include "MidiTypes.h"
#include "USBMidiAPI.h"
#include "HardwareMidiAPI.h"
#include "ControllerAPI.h"

// Simplified router for MIDI messages between all interfaces
class MidiRouter {
private:
    static void onControllerNoteOn(byte channel, byte note, byte velocity) {
        // Forward controller notes to both USB and hardware MIDI
        USBMidiAPI::sendNoteOn(note, velocity, channel);
        HardwareMidiAPI::sendNoteOn(note, velocity, channel);

        // Call user's callback if set
        if (noteOnCallback) noteOnCallback(channel, note, velocity);
    }

    static void onControllerNoteOff(byte channel, byte note, byte velocity) {
        // Forward controller notes to both USB and hardware MIDI
        USBMidiAPI::sendNoteOff(note, velocity, channel);
        HardwareMidiAPI::sendNoteOff(note, velocity, channel);

        // Call user's callback if set
        if (noteOffCallback) noteOffCallback(channel, note, velocity);
    }

    static void onControllerCC(byte channel, byte control, byte value) {
        // Call user's callback if set
        if (ccCallback) ccCallback(channel, control, value);
    }

    // User callbacks
    static void (*noteOnCallback)(byte channel, byte note, byte velocity);
    static void (*noteOffCallback)(byte channel, byte note, byte velocity);
    static void (*ccCallback)(byte channel, byte control, byte value);

public:
    // Initialize all MIDI interfaces
    static void begin() {
        USBMidiAPI::begin();
        HardwareMidiAPI::begin();
        ControllerAPI::begin();

        // Set up controller callbacks to route messages
        ControllerAPI::setHandleNoteOn(onControllerNoteOn);
        ControllerAPI::setHandleNoteOff(onControllerNoteOff);
        ControllerAPI::setHandleControlChange(onControllerCC);
    }

    // Process all MIDI interfaces
    static void update() {
        USBMidiAPI::read();
        HardwareMidiAPI::read();
        ControllerAPI::update();
    }

    // Set user callbacks
    static void setHandleNoteOn(void (*callback)(byte channel, byte note, byte velocity)) {
        noteOnCallback = callback;
    }

    static void setHandleNoteOff(void (*callback)(byte channel, byte note, byte velocity)) {
        noteOffCallback = callback;
    }

    static void setHandleControlChange(void (*callback)(byte channel, byte control, byte value)) {
        ccCallback = callback;
    }
};

// Initialize static callback members
void (*MidiRouter::noteOnCallback)(byte channel, byte note, byte velocity) = nullptr;
void (*MidiRouter::noteOffCallback)(byte channel, byte note, byte velocity) = nullptr;
void (*MidiRouter::ccCallback)(byte channel, byte control, byte value) = nullptr;

#endif
