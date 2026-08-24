#include "audio/KeyboardController.hpp"
#include <cstring>

namespace audio {

KeyboardController::KeyboardController() {
    keyStates_.fill(false);
    initializeKeyMappings();
}

void KeyboardController::initialize() {
    // Hardware initialization would go here for actual MEGA65 keyboard
    keyStates_.fill(false);
}

void KeyboardController::shutdown() {
    keyStates_.fill(false);
}

void KeyboardController::setKeyboardMode(KeyboardMode mode) {
    currentMode_ = mode;
}

void KeyboardController::setOctave(uint8_t octave) {
    if (octave <= 8) {
        currentOctave_ = octave;
    }
}

void KeyboardController::incrementOctave() {
    if (currentOctave_ < 8) {
        currentOctave_++;
    }
}

void KeyboardController::decrementOctave() {
    if (currentOctave_ > 0) {
        currentOctave_--;
    }
}

void KeyboardController::update() {
    // Poll MEGA65 keyboard matrix (stub for now)
    // This would scan the keyboard hardware and generate key events
    // for each key press/release transition
}

KeyEvent KeyboardController::pollKey() {
    // Return any pending key event
    // This would be called repeatedly to drain the keyboard buffer
    return KeyEvent::KEY_NONE;
}

uint8_t KeyboardController::keyToMIDINote(KeyEvent key) const {
    uint8_t noteIndex = mapKeyToScaleNote(key, currentMode_);
    if (noteIndex == 0xFF) return 0;  // Invalid key

    // Map note index to MIDI note within current octave
    // C0 = MIDI note 12, C1 = 24, etc.
    uint8_t baseNote = 12 + (currentOctave_ * 12);
    return baseNote + noteIndex;
}

void KeyboardController::initializeKeyMappings() {
    // Row 0: 1-8 → C C# D D# E F F# G (first 8 chromatic notes)
    rowToChromatic_[0] = 0;  // C

    // Row 1: Q-I → C D E F G A B C (C major scale)
    rowToMajor_[1] = 0;  // C

    // Row 2: A-K → C D E G A B C D (pentatonic extended)
    rowToMajor_[2] = 0;  // C

    // Row 3: Z-M → remaining notes or drums
    rowToMajor_[3] = 0;

    // Pentatonic notes: C(0), D(2), E(4), G(7), A(9)
    pentatonicNotes_[0] = 0;   // C
    pentatonicNotes_[1] = 2;   // D
    pentatonicNotes_[2] = 4;   // E
    pentatonicNotes_[3] = 7;   // G
    pentatonicNotes_[4] = 9;   // A
}

uint8_t KeyboardController::mapKeyToScaleNote(KeyEvent key, KeyboardMode mode) const {
    switch (mode) {
        case KeyboardMode::CHROMATIC:
            // Map keyboard rows to chromatic scale
            switch (key) {
                // Row 0: C C# D D# E F F# G
                case KeyEvent::KEY_1: return 0;   // C
                case KeyEvent::KEY_2: return 1;   // C#
                case KeyEvent::KEY_3: return 2;   // D
                case KeyEvent::KEY_4: return 3;   // D#
                case KeyEvent::KEY_5: return 4;   // E
                case KeyEvent::KEY_6: return 5;   // F
                case KeyEvent::KEY_7: return 6;   // F#
                case KeyEvent::KEY_8: return 7;   // G
                // Row 1: G# A A# B (and more)
                case KeyEvent::KEY_Q: return 8;   // G#
                case KeyEvent::KEY_W: return 9;   // A
                case KeyEvent::KEY_E: return 10;  // A#
                case KeyEvent::KEY_R: return 11;  // B
                case KeyEvent::KEY_T: return 0;   // C (next octave)
                case KeyEvent::KEY_Y: return 1;   // C#
                case KeyEvent::KEY_U: return 2;   // D
                case KeyEvent::KEY_I: return 3;   // D#
                // Row 2: A-K (more chromatic)
                case KeyEvent::KEY_A: return 4;   // E
                case KeyEvent::KEY_S: return 5;   // F
                case KeyEvent::KEY_D: return 6;   // F#
                case KeyEvent::KEY_F: return 7;   // G
                case KeyEvent::KEY_G: return 8;   // G#
                case KeyEvent::KEY_H: return 9;   // A
                case KeyEvent::KEY_J: return 10;  // A#
                case KeyEvent::KEY_K: return 11;  // B
                default: return 0xFF;
            }
            break;

        case KeyboardMode::MAJOR_SCALE:
            // Map keyboard rows to major scale (C D E F G A B)
            switch (key) {
                case KeyEvent::KEY_1: return 0;   // C
                case KeyEvent::KEY_2: return 2;   // D
                case KeyEvent::KEY_3: return 4;   // E
                case KeyEvent::KEY_4: return 5;   // F
                case KeyEvent::KEY_5: return 7;   // G
                case KeyEvent::KEY_6: return 9;   // A
                case KeyEvent::KEY_7: return 11;  // B
                case KeyEvent::KEY_8: return 12;  // C (octave)
                case KeyEvent::KEY_Q: return 0;   // C
                case KeyEvent::KEY_W: return 2;   // D
                case KeyEvent::KEY_E: return 4;   // E
                case KeyEvent::KEY_R: return 5;   // F
                case KeyEvent::KEY_T: return 7;   // G
                case KeyEvent::KEY_Y: return 9;   // A
                case KeyEvent::KEY_U: return 11;  // B
                case KeyEvent::KEY_I: return 12;  // C
                default: return 0xFF;
            }
            break;

        case KeyboardMode::PENTATONIC:
            // Map keyboard rows to pentatonic (C D E G A)
            switch (key) {
                case KeyEvent::KEY_1: return 0;   // C
                case KeyEvent::KEY_2: return 2;   // D
                case KeyEvent::KEY_3: return 4;   // E
                case KeyEvent::KEY_4: return 7;   // G
                case KeyEvent::KEY_5: return 9;   // A
                case KeyEvent::KEY_6: return 12;  // C (octave)
                case KeyEvent::KEY_Q: return 0;   // C
                case KeyEvent::KEY_W: return 2;   // D
                case KeyEvent::KEY_E: return 4;   // E
                case KeyEvent::KEY_R: return 7;   // G
                case KeyEvent::KEY_T: return 9;   // A
                case KeyEvent::KEY_Y: return 12;  // C
                default: return 0xFF;
            }
            break;

        case KeyboardMode::DRUMS:
            // Map to drum kit MIDI notes (36-87)
            switch (key) {
                case KeyEvent::KEY_1: return 36;  // Kick
                case KeyEvent::KEY_2: return 38;  // Snare
                case KeyEvent::KEY_3: return 42;  // Hi-hat closed
                case KeyEvent::KEY_4: return 46;  // Hi-hat open
                case KeyEvent::KEY_5: return 43;  // Tom high
                case KeyEvent::KEY_6: return 47;  // Tom mid
                case KeyEvent::KEY_7: return 50;  // Tom low
                default: return 0xFF;
            }
            break;

        default:
            return 0xFF;
    }
}

}  // namespace audio
