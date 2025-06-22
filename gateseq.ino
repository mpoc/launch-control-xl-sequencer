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
#include "SequencerChannel.h"

// Sequencer configuration
const int BPM = 100;
const unsigned long STEP_DURATION = 60000 / (BPM * 4); // 16th notes at given BPM
const int NUM_STEPS = 16;

// Sequencer state
SequencerChannel channel1(0, 1); // Channel 0, MIDI channel 1
int currentStep = 0;
unsigned long lastStepTime = 0;
bool isPlaying = false;
byte currentPlayingNote = 0;
bool noteIsPlaying = false;

// Knob to note mapping
// PAN_DEVICE knobs (CC 16-23) control notes for steps 1-8
// SEND_A knobs (CC 0-7) control notes for steps 9-16
const byte PAN_DEVICE_START_CC = 16;
const byte SEND_A_START_CC = 0;

// Button to step mapping
const byte TRACK_FOCUS_START_CC = 24;
const byte TRACK_CONTROL_START_CC = 32;

// LED indices
const byte TRACK_FOCUS_START_LED = 24;
const byte TRACK_CONTROL_START_LED = 32;

// Transport button CCs
const byte CC_UP = 44;
const byte CC_DOWN = 45;
const byte CC_LEFT = 46;
const byte CC_RIGHT = 47;

// LED colors for different states
const LedColor COLOR_STEP_OFF = {0, 0};      // Off
const LedColor COLOR_STEP_ON = {0, 1};       // Green low
const LedColor COLOR_CURRENT_OFF = {1, 0};   // Red low
const LedColor COLOR_CURRENT_ON = {3, 3};    // Yellow (current + active)

// Note display helpers
const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void printNoteInfo(int step, byte noteValue) {
    int octave = (noteValue / 12) - 1;
    int noteIndex = noteValue % 12;
    Serial.print("Step ");
    Serial.print(step + 1);
    Serial.print(" note: ");
    Serial.print(noteNames[noteIndex]);
    Serial.print(octave);
    Serial.print(" (");
    Serial.print(noteValue);
    Serial.println(")");
}

void updateStepLEDs() {
    for (int i = 0; i < NUM_STEPS; i++) {
        LedColor color;

        // Determine color based on step state and current position
        if (i == currentStep) {
            color = channel1.isStepActive(i) ? COLOR_CURRENT_ON : COLOR_CURRENT_OFF;
        } else {
            color = channel1.isStepActive(i) ? COLOR_STEP_ON : COLOR_STEP_OFF;
        }

        // Calculate LED index
        byte ledIndex;
        if (i < 8) {
            ledIndex = TRACK_FOCUS_START_LED + i;
        } else {
            ledIndex = TRACK_CONTROL_START_LED + (i - 8);
        }

        ControllerAPI::setLedColor(ledIndex, color);
    }
}

void updateTransportLEDs() {
    LedColor transportColor = isPlaying ? ControllerAPI::GREEN() : ControllerAPI::RED_LOW();
    ControllerAPI::setLedColor(44, transportColor);
}

void playNoteForStep(int stepIndex) {
    if (channel1.isMuted()) return;

    const StepData& step = channel1.getStep(stepIndex);
    if (step.active) {
        currentPlayingNote = step.note;
        USBMidiAPI::sendNoteOn(currentPlayingNote, step.velocity, channel1.getMidiChannel());
        HardwareMidiAPI::sendNoteOn(currentPlayingNote, step.velocity, channel1.getMidiChannel());
        noteIsPlaying = true;

        Serial.print("Step ");
        Serial.print(stepIndex + 1);
        Serial.print(" - NOTE ON: ");
        Serial.print(noteNames[currentPlayingNote % 12]);
        Serial.print((currentPlayingNote / 12) - 1);
        Serial.print(" vel:");
        Serial.println(step.velocity);
    }
}

void stopCurrentNote() {
    if (noteIsPlaying) {
        USBMidiAPI::sendNoteOff(currentPlayingNote, 0, channel1.getMidiChannel());
        HardwareMidiAPI::sendNoteOff(currentPlayingNote, 0, channel1.getMidiChannel());
        noteIsPlaying = false;
    }
}

void handleControlChange(byte channel, byte control, byte value) {
    // Handle knobs for note values
    if (control >= PAN_DEVICE_START_CC && control < PAN_DEVICE_START_CC + 8) {
        // PAN_DEVICE knobs control steps 1-8
        int stepIndex = control - PAN_DEVICE_START_CC;
        byte noteValue = map(value, 0, 127, 36, 84); // Map to C2-C6 range
        channel1.setStepNote(stepIndex, noteValue);
        printNoteInfo(stepIndex, noteValue);
    }
    else if (control >= SEND_A_START_CC && control < SEND_A_START_CC + 8) {
        // SEND_A knobs control steps 9-16
        int stepIndex = (control - SEND_A_START_CC) + 8;
        byte noteValue = map(value, 0, 127, 36, 84); // Map to C2-C6 range
        channel1.setStepNote(stepIndex, noteValue);
        printNoteInfo(stepIndex, noteValue);
    }
    // Handle buttons for step on/off
    else if (control >= TRACK_FOCUS_START_CC && control < TRACK_FOCUS_START_CC + 8) {
        if (value > 0) { // Only on button press
            int stepIndex = control - TRACK_FOCUS_START_CC;
            channel1.toggleStep(stepIndex);
            Serial.print("Toggle step ");
            Serial.print(stepIndex + 1);
            Serial.print(" to ");
            Serial.println(channel1.isStepActive(stepIndex) ? "ON" : "OFF");
            updateStepLEDs();
        }
    }
    else if (control >= TRACK_CONTROL_START_CC && control < TRACK_CONTROL_START_CC + 8) {
        if (value > 0) { // Only on button press
            int stepIndex = (control - TRACK_CONTROL_START_CC) + 8;
            channel1.toggleStep(stepIndex);
            Serial.print("Toggle step ");
            Serial.print(stepIndex + 1);
            Serial.print(" to ");
            Serial.println(channel1.isStepActive(stepIndex) ? "ON" : "OFF");
            updateStepLEDs();
        }
    }
    // Transport controls
    else if (control == CC_UP && value > 0) {
        // Play/Pause
        isPlaying = !isPlaying;
        Serial.println(isPlaying ? "Sequencer PLAYING" : "Sequencer STOPPED");

        if (isPlaying) {
            playNoteForStep(currentStep);
            lastStepTime = millis();
        } else {
            stopCurrentNote();
        }

        updateTransportLEDs();
    }
    else if (control == CC_DOWN && value > 0) {
        // Reset to step 0
        stopCurrentNote();
        currentStep = 0;
        Serial.println("Reset to step 1");

        if (isPlaying) {
            playNoteForStep(0);
            lastStepTime = millis();
        }

        updateStepLEDs();
    }
    else if (control == CC_LEFT && value > 0) {
        // Clear all steps
        channel1.clearPattern();
        Serial.println("Cleared all steps");
        updateStepLEDs();
    }
    else if (control == CC_RIGHT && value > 0) {
        // Fill all steps
        channel1.fillPattern();
        Serial.println("Filled all steps");
        updateStepLEDs();
    }
}

void processSequencer() {
    if (!isPlaying) return;

    unsigned long currentTime = millis();

    if (currentTime - lastStepTime >= STEP_DURATION) {
        lastStepTime = currentTime;

        // Stop current note
        stopCurrentNote();

        // Move to next step
        currentStep = (currentStep + 1) % NUM_STEPS;

        // Play new note if step is active
        playNoteForStep(currentStep);

        // Update display
        updateStepLEDs();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Modular 16-Step Sequencer");

    delay(2000);
    Serial.println("Starting in 2 seconds...");

    // Initialize all APIs
    ControllerAPI::begin();
    USBMidiAPI::begin();
    HardwareMidiAPI::begin();

    // Set up callbacks
    ControllerAPI::setHandleControlChange(handleControlChange);

    // Initialize display
    updateStepLEDs();
    updateTransportLEDs();

    lastStepTime = millis();

    Serial.println("Sequencer ready!");
    Serial.println("Controls:");
    Serial.println("- TRACK FOCUS/CONTROL buttons: Toggle steps on/off");
    Serial.println("- PAN/DEVICE knobs: Set note for steps 1-8");
    Serial.println("- SEND A knobs: Set note for steps 9-16");
    Serial.println("- UP: Play/Pause");
    Serial.println("- DOWN: Reset to step 1");
    Serial.println("- LEFT: Clear pattern");
    Serial.println("- RIGHT: Fill pattern");
}

void loop() {
    ControllerAPI::update();
    USBMidiAPI::read();
    HardwareMidiAPI::read();

    processSequencer();

    yield();
}
