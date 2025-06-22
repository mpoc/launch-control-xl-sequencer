#ifndef SEQUENCER_CHANNEL_H
#define SEQUENCER_CHANNEL_H

#include <Arduino.h>

// Structure to hold all parameters for a single sequencer step
struct StepData {
    bool active;          // Whether this step plays a note
    byte note;           // MIDI note number (0-127)
    byte velocity;       // Note velocity (0-127)

    StepData() : active(false), note(60), velocity(100) {}
};

// Class representing a single sequencer channel
class SequencerChannel {
private:
    static const int SEQUENCE_LENGTH = 16;
    StepData steps[SEQUENCE_LENGTH];
    byte channelNumber;
    byte midiChannel;
    bool muted;
    bool soloed;

public:
    SequencerChannel(byte channel = 0, byte midiCh = 1)
        : channelNumber(channel), midiChannel(midiCh), muted(false), soloed(false) {
        // Initialize with middle C for all steps
        for (int i = 0; i < SEQUENCE_LENGTH; i++) {
            steps[i].note = 60;
            steps[i].velocity = 100;
            steps[i].active = false;
        }
    }

    // Step state management
    void toggleStep(int stepIndex) {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            steps[stepIndex].active = !steps[stepIndex].active;
        }
    }

    void setStepActive(int stepIndex, bool active) {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            steps[stepIndex].active = active;
        }
    }

    bool isStepActive(int stepIndex) const {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            return steps[stepIndex].active;
        }
        return false;
    }

    // Note value management
    void setStepNote(int stepIndex, byte note) {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            steps[stepIndex].note = constrain(note, 0, 127);
        }
    }

    byte getStepNote(int stepIndex) const {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            return steps[stepIndex].note;
        }
        return 60; // Default middle C
    }

    // Velocity management
    void setStepVelocity(int stepIndex, byte velocity) {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            steps[stepIndex].velocity = constrain(velocity, 0, 127);
        }
    }

    byte getStepVelocity(int stepIndex) const {
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            return steps[stepIndex].velocity;
        }
        return 100;
    }

    // Pattern management
    void clearPattern() {
        for (int i = 0; i < SEQUENCE_LENGTH; i++) {
            steps[i].active = false;
        }
    }

    void fillPattern() {
        for (int i = 0; i < SEQUENCE_LENGTH; i++) {
            steps[i].active = true;
        }
    }

    // Channel properties
    byte getChannelNumber() const { return channelNumber; }
    byte getMidiChannel() const { return midiChannel; }
    void setMidiChannel(byte ch) { midiChannel = constrain(ch, 1, 16); }

    bool isMuted() const { return muted; }
    void setMuted(bool m) { muted = m; }
    void toggleMute() { muted = !muted; }

    bool isSoloed() const { return soloed; }
    void setSoloed(bool s) { soloed = s; }
    void toggleSolo() { soloed = !soloed; }

    // Get full step data
    const StepData& getStep(int stepIndex) const {
        static StepData defaultStep;
        if (stepIndex >= 0 && stepIndex < SEQUENCE_LENGTH) {
            return steps[stepIndex];
        }
        return defaultStep;
    }
};

#endif
