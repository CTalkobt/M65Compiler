#pragma once

#include <vector>
#include <cstdint>
#include "Chord.hpp"
#include "ChordProgression.hpp"
#include "Scale.hpp"

namespace procedural {

// Harmonic progression styles
enum class ProgressionStyle : uint8_t {
    JAZZ = 0,            // Jazz changes (ii-V-I, turnarounds)
    BLUES = 1,           // Blues progression (I-IV-V)
    POP = 2,             // Pop harmony (I-V-vi-IV, etc.)
    CLASSICAL = 3,       // Classical sonata form
    FOLK = 4,            // Simple folk progressions
    MODAL = 5,           // Modal interchange progressions
    MINIMALIST = 6,      // Repetitive minimal chords
    CHROMATIC = 7        // Chromatic harmonic movement
};

// Generates harmonic progressions (chord sequences)
class HarmonicGenerator {
public:
    HarmonicGenerator();
    ~HarmonicGenerator() = default;

    // Generate a chord progression
    ChordProgression generateProgression(
        Key key,
        uint32_t lengthBars,
        uint32_t seed,
        ProgressionStyle style = ProgressionStyle::POP,
        float complexity = 0.5f,
        uint32_t ticksPerBeat = 480
    );

    // Generate specific style progressions
    ChordProgression generateJazzProgression(Key key, uint32_t bars, uint32_t seed);
    ChordProgression generateBluesProgression(Key key, uint32_t bars, uint32_t seed);
    ChordProgression generatePopProgression(Key key, uint32_t bars, uint32_t seed);
    ChordProgression generateClassicalProgression(Key key, uint32_t bars, uint32_t seed);
    ChordProgression generateFolkProgression(Key key, uint32_t bars, uint32_t seed);

    // Secondary dominants and modulation
    Chord getSecondaryDominant(const Chord& targetChord, Key key);
    ChordProgression addModulation(const ChordProgression& prog, Key newKey, uint32_t modBar);

    // Voicing strategies
    uint8_t selectVoicing(size_t chordIndex, size_t progressionLength, uint32_t seed);

private:
    std::vector<Chord> getScaleTriads(Key key);
    Chord romanToChord(int roman, Key key);
};

}  // namespace procedural
