#include <Arduino.h>
// Make sure Serial1 is defined before including MidiRouter.h
#if defined(TEENSYDUINO)
  // Teensy boards have Serial1 defined already
#else
  // For other boards that might not have Serial1 defined
  #error "This code requires a board with Serial1 defined, like Teensy"
#endif

#include "MidiRouter.h"
#include "MidiTypes.h"
#include "ControllerAPI.h"

// Flag to control note triggering - should be set by your actual logic
bool shouldPlayNote = false;
unsigned long noteTimer = 0;

// LED blink pattern variables
const unsigned long BLINK_INTERVAL = 1000; // 1 second between color changes
unsigned long lastBlinkTime = 0;
bool redPhase = true;  // Toggle between red and green

void handleControlChange(byte channel, byte control, byte value) {
    // Process control change from controller
    Serial.print("CC: ");
    Serial.print(control);
    Serial.print(", Value: ");
    Serial.println(value);
}

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("Testing Controller API");

    // Critical fix: Add a startup delay to allow time for programming
    delay(2000);  // 2 second delay before initializing USB host
    Serial.println("Starting controller in 2 seconds...");

    // Initialize the controller
    ControllerAPI::begin();

    // Set up callback for control changes
    ControllerAPI::setHandleControlChange(handleControlChange);

    // Initialize blink pattern timing
    lastBlinkTime = millis();
}

void updateBlinkPattern() {
    unsigned long currentTime = millis();

    // Check if it's time to update the blink pattern
    if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        lastBlinkTime = currentTime;

        // Toggle between red and green
        redPhase = !redPhase;
        LedColor color = redPhase ? ControllerAPI::RED() : ControllerAPI::GREEN();

        Serial.print("LED Phase: ");
        Serial.println(redPhase ? "RED" : "GREEN");

        // Update all controller LEDs - covers all 48 LEDs on the Launch Control XL

        // SEND_A knobs (top row) - LEDs 0-7
        for (int i = 0; i < 8; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // SEND_B knobs (row 3) - LEDs 8-15
        for (int i = 8; i < 16; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // PAN_DEVICE knobs (row 2) - LEDs 16-23
        for (int i = 16; i < 24; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // TRACK_FOCUS buttons (bottom row) - LEDs 24-31
        for (int i = 24; i < 32; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // TRACK_CONTROL buttons (row 4) - LEDs 32-39
        for (int i = 32; i < 40; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // Mode buttons - LEDs 40-43
        // DEVICE, MUTE, SOLO, RECORD_ARM
        for (int i = 40; i < 44; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // Transport buttons - LEDs 44-47
        // UP, DOWN, LEFT, RIGHT
        for (int i = 44; i < 48; i++) {
            ControllerAPI::setLedColor(i, color);
        }
    }
}

void loop() {
    // Process controller updates
    ControllerAPI::update();

    // Unified timing for both LED blinking and MIDI note test
    unsigned long currentTime = millis();
    if (currentTime > noteTimer) {
        // Toggle both note state and LED colors
        shouldPlayNote = !shouldPlayNote;
        redPhase = shouldPlayNote; // Synchronize color phase with note (red when note is on)

        // Set LED color based on note state
        LedColor color = shouldPlayNote ? ControllerAPI::RED() : ControllerAPI::GREEN();

        Serial.print("LED Phase: ");
        Serial.println(shouldPlayNote ? "RED (Note ON)" : "GREEN (Note OFF)");

        // Send MIDI note based on toggled state
        if (shouldPlayNote) {
            USBMidiAPI::sendNoteOn(60, 127, 1);  // Send middle C with full velocity on channel 1
        } else {
            USBMidiAPI::sendNoteOff(60, 0, 1);   // Turn off middle C
        }

        // Update all LEDs
        for (int i = 0; i < 48; i++) {
            ControllerAPI::setLedColor(i, color);
        }

        // Set next update time
        noteTimer = currentTime + BLINK_INTERVAL;
    }

    // Allow other system processes to run if needed
    yield();
}
