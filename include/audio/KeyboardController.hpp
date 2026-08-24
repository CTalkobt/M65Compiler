#pragma once

#include <cstdint>
#include <functional>
#include <array>

namespace audio {

// Keyboard note mapping modes
enum class KeyboardMode : uint8_t {
    CHROMATIC = 0,      // C-D-E-F-G-A-B-C chromatic scale
    MAJOR_SCALE = 1,    // C major scale (C-D-E-F-G-A-B)
    PENTATONIC = 2,     // C pentatonic (C-D-E-G-A)
    DRUMS = 3           // Drum kit mode
};

// MEGA65 keyboard scancode mapping
enum class KeyEvent : uint8_t {
    KEY_NONE = 0xFF,
    // Row 0: Escape, 1, 2, 3, 4, 5, 6, 7, 8
    KEY_ESC = 0x00,
    KEY_1 = 0x01,
    KEY_2 = 0x02,
    KEY_3 = 0x03,
    KEY_4 = 0x04,
    KEY_5 = 0x05,
    KEY_6 = 0x06,
    KEY_7 = 0x07,
    KEY_8 = 0x08,
    // Row 1: Q, W, E, R, T, Y, U, I
    KEY_Q = 0x10,
    KEY_W = 0x11,
    KEY_E = 0x12,
    KEY_R = 0x13,
    KEY_T = 0x14,
    KEY_Y = 0x15,
    KEY_U = 0x16,
    KEY_I = 0x17,
    // Row 2: A, S, D, F, G, H, J, K
    KEY_A = 0x20,
    KEY_S = 0x21,
    KEY_D = 0x22,
    KEY_F = 0x23,
    KEY_G = 0x24,
    KEY_H = 0x25,
    KEY_J = 0x26,
    KEY_K = 0x27,
    // Row 3: Z, X, C, V, B, N, M, ,
    KEY_Z = 0x30,
    KEY_X = 0x31,
    KEY_C = 0x32,
    KEY_V = 0x33,
    KEY_B = 0x34,
    KEY_N = 0x35,
    KEY_M = 0x36,
    // Special keys
    KEY_SPACE = 0x40,
    KEY_LSHIFT = 0x41,
    KEY_RSHIFT = 0x42,
    KEY_LCTRL = 0x43,
    KEY_RCTRL = 0x44,
    KEY_UP = 0x50,
    KEY_DOWN = 0x51,
    KEY_LEFT = 0x52,
    KEY_RIGHT = 0x53
};

// Keyboard controller for real-time synthesizer control
class KeyboardController {
public:
    KeyboardController();
    ~KeyboardController() = default;

    // Initialization
    void initialize();
    void shutdown();

    // Keyboard mode control
    void setKeyboardMode(KeyboardMode mode);
    KeyboardMode getKeyboardMode() const { return currentMode_; }

    // Octave control
    void setOctave(uint8_t octave);  // 0-8
    uint8_t getOctave() const { return currentOctave_; }
    void incrementOctave();
    void decrementOctave();

    // Polling and event handling
    void update();
    KeyEvent pollKey();

    // Key-to-MIDI note conversion
    uint8_t keyToMIDINote(KeyEvent key) const;

    // Modifiers
    bool isShiftPressed() const { return shiftPressed_; }
    bool isCtrlPressed() const { return ctrlPressed_; }

    // Callbacks
    using KeyPressCallback = std::function<void(uint8_t midiNote, uint8_t velocity)>;
    using KeyReleaseCallback = std::function<void(uint8_t midiNote)>;
    using ControlChangeCallback = std::function<void(uint8_t controlNumber, uint8_t value)>;

    void setKeyPressCallback(KeyPressCallback cb) { keyPressCallback_ = cb; }
    void setKeyReleaseCallback(KeyReleaseCallback cb) { keyReleaseCallback_ = cb; }
    void setControlChangeCallback(ControlChangeCallback cb) { controlChangeCallback_ = cb; }

private:
    KeyboardMode currentMode_ = KeyboardMode::CHROMATIC;
    uint8_t currentOctave_ = 4;  // Middle octave
    bool shiftPressed_ = false;
    bool ctrlPressed_ = false;

    // Key state tracking
    std::array<bool, 128> keyStates_;

    // Scale note mappings (indices into chromatic scale within octave)
    // Chromatic: C C# D D# E F F# G G# A A# B (12 notes)
    // Major: C D E F G A B (7 notes)
    // Pentatonic: C D E G A (5 notes)

    // Keyboard row to note index mappings
    std::array<uint8_t, 8> rowToChromatic_;   // Chromatic mapping per row
    std::array<uint8_t, 8> rowToMajor_;       // Major scale mapping per row
    std::array<uint8_t, 5> pentatonicNotes_;  // Pentatonic note indices

    // Callbacks
    KeyPressCallback keyPressCallback_;
    KeyReleaseCallback keyReleaseCallback_;
    ControlChangeCallback controlChangeCallback_;

    // Helper methods
    void initializeKeyMappings();
    uint8_t mapKeyToScaleNote(KeyEvent key, KeyboardMode mode) const;
};

}  // namespace audio
