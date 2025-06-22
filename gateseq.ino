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
const int BPM = 120;
const unsigned long STEP_DURATION = 60000 / (BPM * 4); // 16th notes at given BPM
const int NUM_STEPS = 16;
const int NUM_CHANNELS = 8; // Using TRACK FOCUS buttons for channels

// Display modes
enum DisplayMode {
    MODE_HOMEPAGE,
    MODE_CHANNEL,
    MODE_MIXER
};

// Sequencer state
SequencerChannel channels[NUM_CHANNELS]; // Array of channels
int currentChannel = 0;
DisplayMode currentMode = MODE_HOMEPAGE;
int currentStep = 0;
unsigned long lastStepTime = 0;
bool isPlaying = false;

// Note tracking for all channels (for LED blinking)
struct ChannelNoteState {
    bool isPlaying;
    unsigned long noteOnTime;
    byte currentNote;
};
ChannelNoteState channelNotes[NUM_CHANNELS];

// Knob to note mapping
// SEND A knobs (CC 0-7) control notes for steps 1-8
// SEND B knobs (CC 8-15) control notes for steps 9-16
const byte SEND_A_START_CC = 0;    // For steps 1-8
const byte SEND_B_START_CC = 8;    // For steps 9-16

// Button to step mapping
const byte TRACK_FOCUS_START_CC = 24;
const byte TRACK_CONTROL_START_CC = 32;

// LED indices
const byte TRACK_FOCUS_START_LED = 24;
const byte TRACK_CONTROL_START_LED = 32;

// Mode button CCs
const byte CC_DEVICE = 40;
const byte CC_MUTE = 41;
const byte CC_SOLO = 42;
const byte CC_RECORD_ARM = 43;

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
const LedColor COLOR_CHANNEL_INACTIVE = {0, 1}; // Green low
const LedColor COLOR_CHANNEL_ACTIVE = {0, 3};   // Green bright
const LedColor COLOR_CHANNEL_PLAYING = {3, 3};  // Yellow for note activity
const LedColor COLOR_MUTED = {3, 0};          // Red for muted channels
const LedColor COLOR_SOLOED = {0, 3};         // Green for soloed channels

// Note display helpers
const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void printNoteInfo(int step, byte noteValue) {
    int octave = (noteValue / 12) - 1;
    int noteIndex = noteValue % 12;
    Serial.print("Ch");
    Serial.print(currentChannel + 1);
    Serial.print(" Step ");
    Serial.print(step + 1);
    Serial.print(" note: ");
    Serial.print(noteNames[noteIndex]);
    Serial.print(octave);
    Serial.print(" (");
    Serial.print(noteValue);
    Serial.println(")");
}

// Check if any channel is soloed
bool hasAnySolo() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i].isSoloed()) {
            return true;
        }
    }
    return false;
}

// Check if a channel should play based on mute/solo states
bool shouldChannelPlay(int channelIndex) {
    bool anySolo = hasAnySolo();

    if (anySolo) {
        // If any channel is soloed, only soloed channels play
        return channels[channelIndex].isSoloed();
    } else {
        // If no channels are soloed, all non-muted channels play
        return !channels[channelIndex].isMuted();
    }
}

void updateModeLEDs() {
    // DEVICE LED shows if we're in homepage or channel mode
    LedColor deviceColor = (currentMode == MODE_HOMEPAGE) ? ControllerAPI::GREEN() : ControllerAPI::GREEN_LOW();
    ControllerAPI::setLedColor(CC_DEVICE, deviceColor);

    // MUTE LED shows if we're in mixer mode
    LedColor muteColor = (currentMode == MODE_MIXER) ? ControllerAPI::GREEN() : ControllerAPI::OFF();
    ControllerAPI::setLedColor(CC_MUTE, muteColor);
}

void updateHomepageLEDs() {
    // Show channel states on TRACK FOCUS buttons
    for (int i = 0; i < NUM_CHANNELS; i++) {
        LedColor color;

        // Check if this channel has a note playing
        if (channelNotes[i].isPlaying) {
            // Blink the LED - simple on/off based on time
            unsigned long timeSinceNote = millis() - channelNotes[i].noteOnTime;
            if ((timeSinceNote / 100) % 2 == 0) { // Blink every 100ms
                color = COLOR_CHANNEL_PLAYING;
            } else {
                color = (i == currentChannel) ? COLOR_CHANNEL_ACTIVE : COLOR_CHANNEL_INACTIVE;
            }
        } else {
            // Show if this is the current channel
            color = (i == currentChannel) ? COLOR_CHANNEL_ACTIVE : COLOR_CHANNEL_INACTIVE;
        }

        ControllerAPI::setLedColor(TRACK_FOCUS_START_LED + i, color);
    }

    // Turn off TRACK CONTROL LEDs in homepage mode
    for (int i = 0; i < 8; i++) {
        ControllerAPI::setLedColor(TRACK_CONTROL_START_LED + i, COLOR_STEP_OFF);
    }
}

void updateChannelLEDs() {
    // Original step LED display for current channel
    for (int i = 0; i < NUM_STEPS; i++) {
        LedColor color;

        // Determine color based on step state and current position
        if (i == currentStep) {
            color = channels[currentChannel].isStepActive(i) ? COLOR_CURRENT_ON : COLOR_CURRENT_OFF;
        } else {
            color = channels[currentChannel].isStepActive(i) ? COLOR_STEP_ON : COLOR_STEP_OFF;
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

void updateMixerLEDs() {
    // TRACK FOCUS row shows mute states
    for (int i = 0; i < NUM_CHANNELS; i++) {
        LedColor color = channels[i].isMuted() ? COLOR_MUTED : ControllerAPI::OFF();
        ControllerAPI::setLedColor(TRACK_FOCUS_START_LED + i, color);
    }

    // TRACK CONTROL row shows solo states
    for (int i = 0; i < NUM_CHANNELS; i++) {
        LedColor color = channels[i].isSoloed() ? COLOR_SOLOED : ControllerAPI::OFF();
        ControllerAPI::setLedColor(TRACK_CONTROL_START_LED + i, color);
    }
}

void updateLEDs() {
    updateModeLEDs();

    switch (currentMode) {
        case MODE_HOMEPAGE:
            updateHomepageLEDs();
            break;
        case MODE_CHANNEL:
            updateChannelLEDs();
            break;
        case MODE_MIXER:
            updateMixerLEDs();
            break;
    }
}

void updateTransportLEDs() {
    LedColor transportColor = isPlaying ? ControllerAPI::GREEN() : ControllerAPI::RED_LOW();
    ControllerAPI::setLedColor(CC_UP, transportColor);
}

void playNoteForStep(int channelIndex, int stepIndex) {
    if (!shouldChannelPlay(channelIndex)) return;

    const StepData& step = channels[channelIndex].getStep(stepIndex);
    if (step.active) {
        byte note = step.note;
        byte velocity = step.velocity;
        byte midiChannel = channels[channelIndex].getMidiChannel();

        // Send MIDI
        USBMidiAPI::sendNoteOn(note, velocity, midiChannel);
        HardwareMidiAPI::sendNoteOn(note, velocity, midiChannel);

        // Track note state for LED blinking
        channelNotes[channelIndex].isPlaying = true;
        channelNotes[channelIndex].noteOnTime = millis();
        channelNotes[channelIndex].currentNote = note;

        Serial.print("Ch");
        Serial.print(channelIndex + 1);
        Serial.print(" Step ");
        Serial.print(stepIndex + 1);
        Serial.print(" - NOTE ON: ");
        Serial.print(noteNames[note % 12]);
        Serial.print((note / 12) - 1);
        Serial.print(" vel:");
        Serial.println(velocity);
    }
}

void stopNoteForChannel(int channelIndex) {
    if (channelNotes[channelIndex].isPlaying) {
        byte note = channelNotes[channelIndex].currentNote;
        byte midiChannel = channels[channelIndex].getMidiChannel();

        USBMidiAPI::sendNoteOff(note, 0, midiChannel);
        HardwareMidiAPI::sendNoteOff(note, 0, midiChannel);

        channelNotes[channelIndex].isPlaying = false;
    }
}

void stopAllNotes() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        stopNoteForChannel(i);
    }
}

void switchToChannel(int channelIndex) {
    if (channelIndex >= 0 && channelIndex < NUM_CHANNELS) {
        currentChannel = channelIndex;
        currentMode = MODE_CHANNEL;
        Serial.print("Switched to channel ");
        Serial.println(channelIndex + 1);
        updateLEDs();
    }
}

void handleControlChange(byte channel, byte control, byte value) {
    // Mode buttons
    if (control == CC_DEVICE && value > 0) {
        // Toggle between homepage and channel mode (not mixer mode)
        if (currentMode == MODE_MIXER) {
            // Exit mixer mode to homepage
            currentMode = MODE_HOMEPAGE;
        } else {
            currentMode = (currentMode == MODE_HOMEPAGE) ? MODE_CHANNEL : MODE_HOMEPAGE;
        }
        Serial.print("Mode: ");
        Serial.println(currentMode == MODE_HOMEPAGE ? "HOMEPAGE" :
                      (currentMode == MODE_CHANNEL ? "CHANNEL" : "MIXER"));
        updateLEDs();
    }
    else if (control == CC_MUTE && value > 0) {
        // Toggle mixer mode
        currentMode = (currentMode == MODE_MIXER) ? MODE_HOMEPAGE : MODE_MIXER;
        Serial.print("Mode: ");
        Serial.println(currentMode == MODE_MIXER ? "MIXER" :
                      (currentMode == MODE_HOMEPAGE ? "HOMEPAGE" : "CHANNEL"));
        updateLEDs();
    }
    // Handle mode-specific controls
    else if (currentMode == MODE_CHANNEL) {
        // Channel mode - existing sequencer controls

        // Knobs for note values
        if (control >= SEND_A_START_CC && control < SEND_A_START_CC + 8) {
            // SEND A knobs control steps 1-8
            int stepIndex = control - SEND_A_START_CC;
            byte noteValue = map(value, 0, 127, 36, 84);
            channels[currentChannel].setStepNote(stepIndex, noteValue);
            printNoteInfo(stepIndex, noteValue);
        }
        else if (control >= SEND_B_START_CC && control < SEND_B_START_CC + 8) {
            // SEND B knobs control steps 9-16
            int stepIndex = (control - SEND_B_START_CC) + 8;
            byte noteValue = map(value, 0, 127, 36, 84);
            channels[currentChannel].setStepNote(stepIndex, noteValue);
            printNoteInfo(stepIndex, noteValue);
        }
        // Buttons for step on/off
        else if (control >= TRACK_FOCUS_START_CC && control < TRACK_FOCUS_START_CC + 8) {
            if (value > 0) {
                int stepIndex = control - TRACK_FOCUS_START_CC;
                channels[currentChannel].toggleStep(stepIndex);
                Serial.print("Ch");
                Serial.print(currentChannel + 1);
                Serial.print(" Step ");
                Serial.print(stepIndex + 1);
                Serial.print(" -> ");
                Serial.println(channels[currentChannel].isStepActive(stepIndex) ? "ON" : "OFF");
                updateLEDs();
            }
        }
        else if (control >= TRACK_CONTROL_START_CC && control < TRACK_CONTROL_START_CC + 8) {
            if (value > 0) {
                int stepIndex = (control - TRACK_CONTROL_START_CC) + 8;
                channels[currentChannel].toggleStep(stepIndex);
                Serial.print("Ch");
                Serial.print(currentChannel + 1);
                Serial.print(" Step ");
                Serial.print(stepIndex + 1);
                Serial.print(" -> ");
                Serial.println(channels[currentChannel].isStepActive(stepIndex) ? "ON" : "OFF");
                updateLEDs();
            }
        }
    }
    else if (currentMode == MODE_HOMEPAGE) {
        // Homepage mode - channel selection
        if (control >= TRACK_FOCUS_START_CC && control < TRACK_FOCUS_START_CC + 8) {
            if (value > 0) {
                int channelIndex = control - TRACK_FOCUS_START_CC;
                switchToChannel(channelIndex);
            }
        }
    }
    else if (currentMode == MODE_MIXER) {
        // Mixer mode - mute/solo controls

        // TRACK FOCUS buttons control mute
        if (control >= TRACK_FOCUS_START_CC && control < TRACK_FOCUS_START_CC + 8) {
            if (value > 0) {
                int channelIndex = control - TRACK_FOCUS_START_CC;
                channels[channelIndex].toggleMute();
                Serial.print("Channel ");
                Serial.print(channelIndex + 1);
                Serial.print(" mute: ");
                Serial.println(channels[channelIndex].isMuted() ? "ON" : "OFF");
                updateLEDs();
            }
        }
        // TRACK CONTROL buttons control solo
        else if (control >= TRACK_CONTROL_START_CC && control < TRACK_CONTROL_START_CC + 8) {
            if (value > 0) {
                int channelIndex = control - TRACK_CONTROL_START_CC;
                channels[channelIndex].toggleSolo();
                Serial.print("Channel ");
                Serial.print(channelIndex + 1);
                Serial.print(" solo: ");
                Serial.println(channels[channelIndex].isSoloed() ? "ON" : "OFF");
                updateLEDs();
            }
        }
    }

    // Transport controls work in all modes
    if (control == CC_UP && value > 0) {
        isPlaying = !isPlaying;
        Serial.println(isPlaying ? "Sequencer PLAYING" : "Sequencer STOPPED");

        if (isPlaying) {
            lastStepTime = millis();
            // Play current step for all channels
            for (int i = 0; i < NUM_CHANNELS; i++) {
                playNoteForStep(i, currentStep);
            }
        } else {
            stopAllNotes();
        }

        updateTransportLEDs();
    }
    else if (control == CC_DOWN && value > 0) {
        stopAllNotes();
        currentStep = 0;
        Serial.println("Reset to step 1");

        if (isPlaying) {
            lastStepTime = millis();
            for (int i = 0; i < NUM_CHANNELS; i++) {
                playNoteForStep(i, 0);
            }
        }

        updateLEDs();
    }
    else if (control == CC_LEFT && value > 0 && currentMode == MODE_CHANNEL) {
        channels[currentChannel].clearPattern();
        Serial.print("Cleared channel ");
        Serial.println(currentChannel + 1);
        updateLEDs();
    }
    else if (control == CC_RIGHT && value > 0 && currentMode == MODE_CHANNEL) {
        channels[currentChannel].fillPattern();
        Serial.print("Filled channel ");
        Serial.println(currentChannel + 1);
        updateLEDs();
    }
}

void processSequencer() {
    if (!isPlaying) return;

    unsigned long currentTime = millis();

    // Handle note off timing (simple gate - half step duration)
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channelNotes[i].isPlaying &&
            (currentTime - channelNotes[i].noteOnTime) > (STEP_DURATION / 2)) {
            stopNoteForChannel(i);
        }
    }

    // Handle step advancement
    if (currentTime - lastStepTime >= STEP_DURATION) {
        lastStepTime = currentTime;

        // Stop any remaining notes
        stopAllNotes();

        // Move to next step
        currentStep = (currentStep + 1) % NUM_STEPS;

        // Play notes for all channels
        for (int i = 0; i < NUM_CHANNELS; i++) {
            playNoteForStep(i, currentStep);
        }

        // Update display
        updateLEDs();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Multi-Channel 16-Step Sequencer with Mixer");

    delay(2000);
    Serial.println("Starting in 2 seconds...");

    // Initialize channels with unique MIDI channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channels[i] = SequencerChannel(i, i + 1); // MIDI channels 1-8

        // Initialize note states
        channelNotes[i].isPlaying = false;
        channelNotes[i].noteOnTime = 0;
        channelNotes[i].currentNote = 0;
    }

    // Initialize all APIs
    ControllerAPI::begin();
    USBMidiAPI::begin();
    HardwareMidiAPI::begin();

    // Set up callbacks
    ControllerAPI::setHandleControlChange(handleControlChange);

    // Initialize display
    updateLEDs();
    updateTransportLEDs();

    lastStepTime = millis();

    Serial.println("Multi-channel sequencer ready!");
    Serial.println("\nControls:");
    Serial.println("- DEVICE: Toggle between Homepage and Channel view");
    Serial.println("- MUTE: Enter/exit Mixer mode");
    Serial.println("- Homepage: Press TRACK FOCUS buttons to select channels");
    Serial.println("- Mixer mode:");
    Serial.println("  - TRACK FOCUS: Mute channels");
    Serial.println("  - TRACK CONTROL: Solo channels");
    Serial.println("- Channel view: Edit patterns as before");
    Serial.println("- Transport controls work in all modes");
    Serial.println("\nStarting in Homepage mode");
}

void loop() {
    ControllerAPI::update();
    USBMidiAPI::read();
    HardwareMidiAPI::read();

    processSequencer();

    // Update homepage LEDs for blinking effect
    if (currentMode == MODE_HOMEPAGE) {
        updateHomepageLEDs();
    }

    yield();
}
