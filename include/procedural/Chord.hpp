#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "Scale.hpp"

namespace procedural {

// Chord quality (triad, extended, etc.)
enum class ChordQuality : uint8_t {
    MAJOR = 0,           // 1-3-5
    MINOR = 1,           // 1-b3-5
    DIMINISHED = 2,      // 1-b3-b5
    AUGMENTED = 3,       // 1-3-#5
    MAJOR7 = 4,          // 1-3-5-7
    MINOR7 = 5,          // 1-b3-5-b7
    DOMINANT7 = 6,       // 1-3-5-b7
    MINOR_MAJOR7 = 7,    // 1-b3-5-7
    HALF_DIM7 = 8,       // 1-b3-b5-b7
    SUSPENDED2 = 9,      // 1-2-5
    SUSPENDED4 = 10,     // 1-4-5
    MAJOR6 = 11,         // 1-3-5-6
    MINOR6 = 12,         // 1-b3-5-6
    ADD9 = 13,           // 1-3-5-9
    MAJOR9 = 14,         // 1-3-5-7-9
    MINOR9 = 15          // 1-b3-5-b7-9
};

// Musical chord: root + quality + voicing
class Chord {
public:
    Chord(Note root, ChordQuality quality);
    ~Chord() = default;

    // Properties
    Note getRootNote() const { return root_; }
    ChordQuality getQuality() const { return quality_; }
    const std::string& getName() const { return name_; }

    // Get chord tones (semitones from root, in octave)
    const std::vector<uint8_t>& getIntervals() const { return intervals_; }

    // Get voicing (specific pitches)
    // voicingStyle: 0=root position, 1=first inversion, 2=second inversion, 3=drop-2, etc.
    std::vector<uint8_t> getVoicing(uint8_t voicingStyle = 0, uint8_t octave = 4) const;

    // Get root voicing (just the root note)
    uint8_t getRootVoicing(uint8_t octave = 4) const;

    // Get bass note for inversion
    uint8_t getBassNote(uint8_t inversion = 0, uint8_t octave = 2) const;

private:
    Note root_;
    ChordQuality quality_;
    std::string name_;
    std::vector<uint8_t> intervals_;  // Semitones from root (sorted)

    void initializeIntervals();
};

}  // namespace procedural
