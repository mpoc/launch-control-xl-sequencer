# Teensy 4.1 MIDI Sequencer Project

## Project Overview

This project implements a hardware MIDI sequencer using a Teensy 4.1 microcontroller as the brain and a Novation Launch Control XL as the primary user interface. The sequencer acts as a bridge between hardware MIDI devices and computer software, capable of generating and routing MIDI messages through multiple interfaces simultaneously.

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
   - Designed for easy multi-channel expansion

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

### Basic 16-Step Gate Sequencer

The current implementation provides a single-channel, 16-step gate sequencer with the following features:

#### Step Control
- **Steps 1-8**: TRACK FOCUS buttons (bottom row)
- **Steps 9-16**: TRACK CONTROL buttons (top row)
- Press buttons to toggle steps on/off

#### Note Control
- **Steps 1-8 notes**: PAN/DEVICE knobs (CC 16-23)
- **Steps 9-16 notes**: SEND A knobs (CC 0-7)
- Note range: C2 to C6 (MIDI notes 36-84)

#### Transport Controls
- **UP button**: Play/Pause sequencer
- **DOWN button**: Reset to step 1
- **LEFT button**: Clear all steps
- **RIGHT button**: Fill all steps

#### Visual Feedback
- **Inactive steps**: LED off
- **Active steps**: Green (low intensity)
- **Current step (inactive)**: Red (low intensity)
- **Current step (active)**: Yellow (bright)
- **Play indicator**: UP button LED (green when playing)

#### Timing
- Fixed tempo: 120 BPM
- Resolution: 16th notes
- Non-blocking timing using `millis()`

### MIDI Output
The sequencer sends MIDI note messages simultaneously to:
- USB MIDI (for DAW/computer software)
- Hardware MIDI out (for synthesizers/drum machines)
- Proper note-off handling prevents stuck notes

## Code Structure

```
├── USBMidiAPI.h          # USB MIDI to computer
├── HardwareMidiAPI.h     # Traditional MIDI I/O
├── ControllerAPI.h       # Launch Control XL interface
├── MidiTypes.h           # Common MIDI data structures
├── SequencerChannel.h    # Channel abstraction
├── MidiRouter.h          # Message routing (currently unused)
└── basic-sequencer.ino   # Main sequencer implementation
```

## Design Principles

1. **Modularity**: Each component has a clean API interface
2. **Extensibility**: Channel-based architecture supports future multi-channel implementation
3. **Decoupling**: Buttons, LEDs, and sequencer logic are independent
4. **Real-time Performance**: Non-blocking code suitable for tight timing
5. **Clear Abstractions**: Logical separation between transport, pattern, and note control

## Future Development Paths

The modular architecture supports many potential enhancements:

### Multi-Channel Support
- Multiple independent sequencer channels
- Channel selection/viewing modes
- Per-channel mute/solo
- Channel mixing/routing

### Enhanced Sequencing
- Variable sequence lengths
- Pattern banks/storage
- Swing/shuffle timing
- Note velocity control (using SEND B knobs)
- Gate length control
- Ratcheting/subdivisions

### Advanced Features
- Scale quantization
- Probability/random steps
- Parameter locks
- Pattern chaining
- MIDI clock sync
- External MIDI input processing

### UI Enhancements
- Multiple view modes
- Visual metronome
- Pattern visualization across all LEDs
- Settings/configuration mode

## Technical Notes

- The project requires Teensy-specific features (USB Host, multiple serial ports)
- A 2-second startup delay prevents programming issues
- LED control uses manufacturer-specific SysEx messages
- The USB Host library creates global objects that must be in the main scope
- Serial1 must be defined before including certain headers

## Development Environment

- Platform: Arduino IDE with Teensyduino
- Board: Teensy 4.1
- Required Libraries:
  - USBHost_t36 (Teensy USB Host)
  - MIDI Library (FortySevenEffects)
  - Core Teensy libraries

This documentation represents the project state as of the basic 16-step sequencer implementation. The architecture has been designed with future expansion in mind, making it straightforward to add channels, features, and complexity while maintaining clean, understandable code.
