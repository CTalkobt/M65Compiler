#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace procedural {

// Musical note enumeration (chromatic scale)
enum class Note : uint8_t {
    C = 0, Cs = 1, D = 2, Ds = 3, E = 4, F = 5,
    Fs = 6, G = 7, Gs = 8, A = 9, As = 10, B = 11
};

// Musical key (root note + major/minor)
enum class Key : uint8_t {
    C_MAJOR = 0,   C_MINOR = 12,
    Cs_MAJOR = 1,  Cs_MINOR = 13,
    D_MAJOR = 2,   D_MINOR = 14,
    Ds_MAJOR = 3,  Ds_MINOR = 15,
    E_MAJOR = 4,   E_MINOR = 16,
    F_MAJOR = 5,   F_MINOR = 17,
    Fs_MAJOR = 6,  Fs_MINOR = 18,
    G_MAJOR = 7,   G_MINOR = 19,
    Gs_MAJOR = 8,  Gs_MINOR = 20,
    A_MAJOR = 9,   A_MINOR = 21,
    As_MAJOR = 10, As_MINOR = 22,
    B_MAJOR = 11,  B_MINOR = 23
};

// Musical scale: collection of notes in a key
class Scale {
public:
    // Standard scale types
    static Scale Major(Note root);
    static Scale NaturalMinor(Note root);
    static Scale HarmonicMinor(Note root);
    static Scale MelodicMinor(Note root);
    static Scale Pentatonic(Note root, bool major = true);
    static Scale Blues(Note root);
    static Scale Dorian(Note root);
    static Scale Phrygian(Note root);
    static Scale Lydian(Note root);
    static Scale Mixolydian(Note root);

    // Query
    const std::vector<uint8_t>& getNotes() const { return notes_; }
    uint8_t getRootNote() const { return rootNote_; }
    const std::string& getName() const { return name_; }

    // Check if note is in scale
    bool contains(uint8_t pitch) const;

    // Get next/previous note in scale
    uint8_t getNextNote(uint8_t pitch) const;
    uint8_t getPreviousNote(uint8_t pitch) const;

    // Get scale degree (0-based)
    int getScaleDegree(uint8_t pitch) const;

private:
    Scale(uint8_t root, const std::vector<uint8_t>& intervals, const std::string& name);

    uint8_t rootNote_;
    std::vector<uint8_t> notes_;  // Chromatic pitches (0-11) in scale
    std::string name_;
};

}  // namespace procedural
