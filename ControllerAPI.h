#ifndef CONTROLLER_API_H
#define CONTROLLER_API_H

#include <Arduino.h>
#include "USBHost_t36.h"
#include "MidiTypes.h"

// Launch Control XL LED capabilities:
//   LED indices 0-39  (knobs, TRACK FOCUS, TRACK CONTROL): red + green (can mix to yellow/amber)
//   LED indices 40-43 (DEVICE, MUTE, SOLO, RECORD ARM):   green only
//   LED indices 44-47 (UP, DOWN, LEFT, RIGHT):             red only
//
// Colors for Launch Control XL LEDs
struct LedColor {
    byte red;    // 0-3
    byte green;  // 0-3
};

// Define common colors
static const LedColor LED_OFF = {0, 0};
static const LedColor LED_RED_LOW = {1, 0};
static const LedColor LED_RED = {3, 0};
static const LedColor LED_GREEN_LOW = {0, 1};
static const LedColor LED_GREEN = {0, 3};
static const LedColor LED_YELLOW = {3, 3};
static const LedColor LED_AMBER = {3, 2};

// Global variables for USB host - must be in global scope
static USBHost controllerUsbHost;
static USBHub controllerHub(controllerUsbHost);
static MIDIDevice controllerMidi(controllerUsbHost);

// Simplified interface for Launch Control XL MIDI controller
class ControllerAPI {
private:
    static const int MAX_LEDS = 48;
    static byte ledCache[MAX_LEDS];  // Cached color bytes, 0xFF = uninitialized

public:
    // Initialize controller communication
    static void begin() {
        // Initialize USB host for controller
        controllerUsbHost.begin();
        // Initialize LED cache to uninitialized
        memset(ledCache, 0xFF, sizeof(ledCache));
    }

    // Process USB host tasks
    static void update() {
        // Process USB events
        controllerUsbHost.Task();
        controllerMidi.read();
    }

    // Set LED color on controller (0-63 for Launch Control XL)
    static void setLedColor(byte ledIndex, LedColor color) {
        if (ledIndex >= MAX_LEDS) return;
        byte colorByte = color.red + (color.green << 4);
        // Skip if LED already has this color
        if (ledCache[ledIndex] == colorByte) return;
        ledCache[ledIndex] = colorByte;
        byte templateIndex = 0;
        byte sysexData[] = {0xF0, 0x00, 0x20, 0x29, 0x02, 0x11, 0x78, templateIndex, ledIndex, colorByte, 0xF7};
        controllerMidi.sendSysEx(11, sysexData, true);
    }

    // Set callbacks for controller input
    static void setHandleNoteOn(void (*callback)(byte channel, byte note, byte velocity)) {
        controllerMidi.setHandleNoteOn(callback);
    }

    static void setHandleNoteOff(void (*callback)(byte channel, byte note, byte velocity)) {
        controllerMidi.setHandleNoteOff(callback);
    }

    static void setHandleControlChange(void (*callback)(byte channel, byte control, byte value)) {
        controllerMidi.setHandleControlChange(callback);
    }

    // For convenience, expose the color constants
    static const LedColor& OFF() { return LED_OFF; }
    static const LedColor& RED_LOW() { return LED_RED_LOW; }
    static const LedColor& RED() { return LED_RED; }
    static const LedColor& GREEN_LOW() { return LED_GREEN_LOW; }
    static const LedColor& GREEN() { return LED_GREEN; }
    static const LedColor& YELLOW() { return LED_YELLOW; }
    static const LedColor& AMBER() { return LED_AMBER; }
};

// Static member definition
byte ControllerAPI::ledCache[ControllerAPI::MAX_LEDS];

#endif
