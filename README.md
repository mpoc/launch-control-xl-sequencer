# Teensy 4.1 Multi-Channel MIDI Sequencer Project

## Project Overview

This project implements a multi-channel hardware MIDI sequencer using a Teensy 4.1 microcontroller as the brain and a Novation Launch Control XL as the primary user interface. The sequencer features 8 independent channels, each capable of generating 16-step patterns, with four distinct modes: a homepage view for channel overview, individual channel editing modes, a mixer mode for mute/solo control, and a live recording mode for real-time pattern input. The system acts as a bridge between hardware MIDI devices and computer software, capable of generating and routing MIDI messages through multiple interfaces simultaneously, with full MIDI clock sync capabilities.

## Hardware Setup

### Core Components
- **Teensy 4.1**: Main microcontroller running the sequencer logic
- **Novation Launch Control XL**: USB MIDI controller providing:
  - 24 knobs (3 rows of 8)
  - 8 faders
  - 16 buttons with LED feedback (2 rows of 8)
  - 4 mode buttons with LEDs
  - 4 transport buttons with LEDs

### Connections
1. **USB Host**: Teensy acts as USB host for the Launch Control XL
2. **USB Device**: Teensy connects to computer as a USB MIDI device
3. **Hardware MIDI**: Traditional 5-pin DIN MIDI output via Serial1
4. **Serial Debug**: USB serial connection for debugging output

## Software Architecture

### Modular API Design

The codebase is organized into clean, modular APIs for different communication interfaces:

1. **USBMidiAPI** (`USBMidiAPI.h`)
   - Handles USB MIDI communication with the computer
   - Sends/receives MIDI messages as a USB device
   - Uses Teensy's built-in `usbMIDI` library

2. **HardwareMidiAPI** (`HardwareMidiAPI.h`)
   - Manages traditional hardware MIDI I/O
   - Uses Serial1 for MIDI communication
   - Based on the FortySevenEffects MIDI library

3. **ControllerAPI** (`ControllerAPI.h`)
   - Interfaces with Launch Control XL via USB Host
   - Receives button/knob input as MIDI CC messages
   - Controls LED states via MIDI SysEx messages
   - Manages USB host tasks

4. **SequencerChannel** (`SequencerChannel.h`)
   - Encapsulates a single sequencer channel
   - Stores step data (active state, note values, velocity)
   - Provides pattern manipulation methods
   - Supports mute and solo states
   - Supports 8 independent channels

### Launch Control XL Mapping

Based on the provided mapping data, the controller layout is:

#### Knobs (CC Messages)
- **SEND A** (Top row): CC 0-7, LED indices 0-7
- **SEND B** (Middle row): CC 8-15, LED indices 8-15
- **PAN/DEVICE** (Bottom row): CC 16-23, LED indices 16-23

#### Faders
- **FADERS 1-8**: CC 48-55 (no LED control)

#### Buttons with LEDs
- **TRACK FOCUS** (Bottom row): CC 24-31, LED indices 24-31
- **TRACK CONTROL** (Top row): CC 32-39, LED indices 32-39
- **Mode buttons**: CC 40-43, LED indices 40-43 (DEVICE, MUTE, SOLO, RECORD_ARM)
- **Transport buttons**: CC 44-47, LED indices 44-47 (UP, DOWN, LEFT, RIGHT)

## Current Implementation

### Multi-Channel 16-Step Gate Sequencer with Mixer and Live Recording

The implementation provides 8 independent sequencer channels, each with 16 steps, featuring four display modes:

#### Display Modes

1. **Homepage Mode**
   - Overview of all 8 channels displayed on TRACK FOCUS buttons
   - LED states:
     - Green (low): Inactive channel
     - Green (bright): Currently selected channel
     - Yellow (blinking): Channel actively playing a note
   - Press any TRACK FOCUS button to enter that channel

2. **Channel Mode**
   - Detailed view of the selected channel's 16-step pattern
   - Same step control interface as original implementation
   - Full pattern editing capabilities per channel

3. **Mixer Mode**
   - Professional mixer-style mute and solo controls
   - TRACK FOCUS row: Mute buttons for channels 1-8
   - TRACK CONTROL row: Solo buttons for channels 1-8
   - LED states:
     - Mute LEDs: Off (unmuted) / Red (muted)
     - Solo LEDs: Off (not soloed) / Green (soloed)
   - Solo logic: If any channel is soloed, only soloed channels play

4. **Record Mode** (Live Recording)
   - Real-time pattern recording inspired by classic drum machines
   - While playing: Tap TRACK FOCUS buttons to record beats
   - Taps are time-quantized to the nearest step:
     - First half of step period: Records to current step
     - Second half of step period: Records to next step
   - Visual feedback:
     - Red: Armed/ready channels
     - Yellow flash: Tap confirmation (100ms)
     - Yellow blink: Note activity (same as homepage)
     - TRACK CONTROL row shows current step position (1-8)
   - While stopped: Tap to arm/disarm channels for recording
   - TRACK CONTROL buttons clear channel patterns

#### Mode Navigation
- **DEVICE button**: Toggle between Homepage and Channel modes
- **MUTE button**: Enter/exit Mixer mode
- **SOLO button**: Enter/exit Record mode
- DEVICE LED indicates homepage vs channel mode (bright = homepage, dim = channel)
- MUTE LED lights when in mixer mode
- SOLO LED lights red when in record mode

#### Step Control (Channel Mode)
- **Steps 1-8**: TRACK FOCUS buttons (bottom row)
- **Steps 9-16**: TRACK CONTROL buttons (top row)
- Press buttons to toggle steps on/off

#### Note Control (Channel Mode)
- **Steps 1-8 notes**: SEND A knobs (CC 0-7)
- **Steps 9-16 notes**: SEND B knobs (CC 8-15)
- Note range: C2 to C6 (MIDI notes 36-84)

#### Mixer Control (Mixer Mode)
- **Channels 1-8 mute**: TRACK FOCUS buttons (bottom row)
- **Channels 1-8 solo**: TRACK CONTROL buttons (top row)
- Mute: Prevents channel from playing
- Solo: Only soloed channels play (standard mixer behavior)

#### Transport Controls (All Modes)
- **UP button**: Play/Pause sequencer
  - Sends MIDI Start (0xFA) when starting from step 1
  - Sends MIDI Continue (0xFB) when resuming from other steps
  - Sends MIDI Stop (0xFC) when pausing
- **DOWN button**: Reset to step 1
  - Sends MIDI Stop followed by MIDI Start if playing
- **LEFT button**: Clear all steps (Channel mode only)
- **RIGHT button**: Fill all steps (Channel mode only)

#### Visual Feedback
- **Homepage Mode**:
  - Channel LEDs blink on note activity (100ms cycle)
  - Selected channel shows brighter green
- **Channel Mode**:
  - Inactive steps: LED off
  - Active steps: Green (low intensity)
  - Current step (inactive): Red (low intensity)
  - Current step (active): Yellow (bright)
- **Mixer Mode**:
  - Muted channels: Red LED
  - Soloed channels: Green LED
  - Unmuted/unsoloed: LED off
- **Record Mode**:
  - Armed channels: Red LED
  - Tap feedback: Yellow flash (100ms)
  - Note activity: Yellow blink (like homepage)
  - Step position: Green on TRACK CONTROL (steps 1-8)
- **Play indicator**: UP button LED (green when playing)

#### Multi-Channel Features
- **8 Independent Channels**: Each with unique patterns
- **Simultaneous Playback**: All channels play together
- **MIDI Channel Assignment**: Channels 1-8 use MIDI channels 1-8
- **Per-Channel Patterns**: Each channel maintains its own 16-step sequence
- **Mute/Solo System**: Professional mixer-style channel control
- **Live Recording**: Real-time pattern input with intelligent time quantization
- **MIDI Clock Master**: Acts as sync source for external devices

#### Timing
- Fixed tempo: 120 BPM
- Resolution: 16th notes
- Gate duration: 50% of step length
- Non-blocking timing using `millis()`

### MIDI Output & Sync
The sequencer sends MIDI messages simultaneously to:
- USB MIDI (for DAW/computer software)
- Hardware MIDI out (for synthesizers/drum machines)
- Each channel outputs on its own MIDI channel (1-8)
- Proper note-off handling prevents stuck notes
- Mute/solo states affect MIDI output in real-time

#### MIDI Clock Sync
- **Master Clock**: Outputs MIDI Clock at 24 ppq (pulses per quarter note)
- **Transport Sync**: Sends standard MIDI Start/Stop/Continue messages
- **Tempo**: Fixed at 120 BPM with accurate timing
- **External Sync**: DAWs and devices can follow the sequencer's transport

## Code Structure

```
├── USBMidiAPI.h          # USB MIDI to computer
├── HardwareMidiAPI.h     # Traditional MIDI I/O
├── ControllerAPI.h       # Launch Control XL interface
├── MidiTypes.h           # Common MIDI data structures
├── SequencerChannel.h    # Channel abstraction with mute/solo
├── MidiRouter.h          # Message routing (currently unused)
└── launch-control-xl-sequencer.ino  # Multi-channel sequencer implementation
```

## Design Principles

1. **Modularity**: Each component has a clean API interface
2. **Extensibility**: Channel-based architecture with 8 independent channels
3. **Decoupling**: Buttons, LEDs, and sequencer logic are independent
4. **Real-time Performance**: Non-blocking code suitable for tight timing
5. **Clear Abstractions**: Logical separation between transport, pattern, and note control
6. **Mode-Based UI**: Four distinct modes for different workflows
7. **Professional Features**: Mixer-style mute/solo follows industry standards
8. **MIDI Sync**: Full clock and transport sync for integration with other MIDI gear
9. **Performance-Oriented**: Live recording mode enables real-time pattern creation
10. **Intuitive Workflow**: Inspired by classic drum machines for familiar operation

## Usage Guide

### Getting Started
1. Power on - sequencer starts in Homepage mode
2. Press any TRACK FOCUS button to select a channel
3. In Channel mode, use buttons to create patterns and knobs to set notes
4. Press UP to start playback

### Channel Management
- **Homepage**: Quick overview and channel selection
- **Channel Mode**: Detailed pattern editing for selected channel
- **Pattern Creation**: Each channel stores its own 16-step pattern
- **Multi-Channel Playback**: All active channels play simultaneously

### Mixer Operations
- **Enter Mixer**: Press MUTE button from any mode
- **Mute Channels**: Press TRACK FOCUS buttons to toggle mute
- **Solo Channels**: Press TRACK CONTROL buttons to toggle solo
- **Exit Mixer**: Press MUTE again or DEVICE to return

### Live Recording
- **Enter Record Mode**: Press SOLO button (LED turns red)
- **While Playing**:
  - Tap TRACK FOCUS buttons in rhythm with the music
  - Each tap records a note at the nearest step (intelligently quantized)
  - Yellow flash confirms each tap
  - Overdubs onto existing patterns (doesn't erase)
- **While Stopped**:
  - Tap TRACK FOCUS to arm/disarm channels
  - Armed channels show red LED
- **Clear Patterns**: Use TRACK CONTROL buttons in record mode
- **Step Position**: TRACK CONTROL LEDs show current playback position

### Tips
- Homepage LEDs blink to show channel activity
- Each channel outputs on a different MIDI channel for multi-timbral use
- Transport controls work in all modes for quick access
- Solo overrides mute - when any channel is soloed, only soloed channels play
- Mixer changes take effect immediately, even during playback
- Use as MIDI master clock to sync external devices and DAWs
- MIDI Start/Stop/Continue messages allow seamless integration with other gear
- Record mode is perfect for finger drumming and creating natural rhythmic patterns
- Recorded notes use a default value (middle C) but can be changed in channel mode
- Time quantization ensures taps land on the nearest beat, making it easy to stay in time
