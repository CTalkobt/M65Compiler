#pragma once

#include <vector>
#include <cstdint>

namespace procedural {

// Rhythm patterns (note durations in ticks relative to beat)
enum class RhythmPattern : uint8_t {
    STEADY = 0,         // All notes same duration
    SWING = 1,          // Swung rhythm (long-short-long-short)
    TRIPLET = 2,        // Triplet feel (three notes per beat)
    SYNCOPATED = 3,     // Syncopated rhythm
    POLYRHYTHMIC = 4,   // Multiple simultaneous patterns
    OFFBEAT = 5,        // Notes on offbeats
    BROKEN = 6,         // Irregular/broken rhythm
    OSTINATO = 7        // Repeating pattern
};

// Rhythmic complexity levels
enum class RhythmComplexity : uint8_t {
    SIMPLE = 0,      // Whole, half, quarter notes
    MODERATE = 1,    // Add eighths, sixteenths
    COMPLEX = 2,     // Add triplets, syncopation
    POLYRHYTHMIC = 3 // Multiple overlapping patterns
};

// Generates rhythm patterns and note durations
class RhythmicGenerator {
public:
    RhythmicGenerator();
    ~RhythmicGenerator() = default;

    // Generate rhythm pattern (note durations in ticks)
    std::vector<uint32_t> generatePattern(
        uint32_t lengthTicks,
        uint32_t ticksPerBeat,
        uint32_t seed,
        RhythmPattern pattern = RhythmPattern::STEADY,
        RhythmComplexity complexity = RhythmComplexity::SIMPLE
    );

    // Generate specific rhythm types
    std::vector<uint32_t> generateSwingRhythm(uint32_t length, uint32_t seed);
    std::vector<uint32_t> generateTripletRhythm(uint32_t length, uint32_t seed);
    std::vector<uint32_t> generateSyncopatedRhythm(uint32_t length, uint32_t seed);
    std::vector<uint32_t> generatePolyrhythmicPattern(uint32_t length, uint32_t seed);

    // Generate a drum pattern (sequence of drum hits with positions and intensities)
    std::vector<std::pair<uint32_t, uint8_t>> generateDrumPattern(
        uint32_t lengthBars,
        uint32_t ticksPerBeat,
        uint32_t seed,
        uint8_t drummer = 0  // 0=basic, 1=jazz, 2=funk, 3=metal
    );

    // Time signature support
    std::vector<uint32_t> generateInTimeSignature(
        uint32_t lengthBars,
        uint8_t numerator,    // 2, 3, 4, 5, 7, etc.
        uint8_t denominator,  // 4, 8, 16
        uint32_t seed,
        RhythmComplexity complexity = RhythmComplexity::SIMPLE
    );

private:
    // Helper methods
    std::vector<uint32_t> generateRandomDurations(uint32_t length, uint32_t seed);
    std::vector<uint32_t> generateEvenDurations(uint32_t length, uint32_t evenness);
};

}  // namespace procedural
