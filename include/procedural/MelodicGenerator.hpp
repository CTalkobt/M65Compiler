#pragma once

#include <vector>
#include <cstdint>
#include "Scale.hpp"

namespace procedural {

// Melodic generation strategies
enum class MelodicStrategy : uint8_t {
    MARKOV_CHAIN = 0,     // Markov chain following note transitions
    GENETIC = 1,          // Genetic algorithm
    RANDOM_WALK = 2,      // Random walk on scale
    ASCENDING = 3,        // Ascending pattern
    DESCENDING = 4,       // Descending pattern
    ARPEGGIO = 5,         // Arpeggio pattern
    PENTATONIC = 6        // Pentatonic melodies
};

// Melody direction hint
enum class MelodicDirection : uint8_t {
    UP = 0,
    DOWN = 1,
    NEUTRAL = 2,
    OSCILLATE = 3
};

// Generates melodies based on various algorithms
class MelodicGenerator {
public:
    MelodicGenerator();
    ~MelodicGenerator() = default;

    // Generate a melody sequence
    std::vector<uint8_t> generate(
        const Scale& scale,
        uint32_t lengthTicks,
        uint32_t seed,
        MelodicStrategy strategy = MelodicStrategy::MARKOV_CHAIN,
        float complexity = 0.5f,
        MelodicDirection direction = MelodicDirection::NEUTRAL
    );

    // Generate with specific parameters
    std::vector<uint8_t> generateFromPattern(
        const Scale& scale,
        const std::vector<int>& intervals,  // Scale degree changes
        uint8_t startPitch,
        uint32_t noteDuration,
        uint32_t seed
    );

    // Generate using Markov chain transitions
    std::vector<uint8_t> generateMarkovMelody(
        const Scale& scale,
        uint32_t lengthNotes,
        uint32_t seed,
        float complexity = 0.5f
    );

    // Generate random walk melody
    std::vector<uint8_t> generateRandomWalk(
        const Scale& scale,
        uint32_t lengthNotes,
        uint32_t seed,
        int maxStep = 2,
        bool tendencyTowardCenter = true
    );

private:
    // Helper methods for different strategies
    std::vector<uint8_t> generateAscending(const Scale& scale, uint32_t length);
    std::vector<uint8_t> generateDescending(const Scale& scale, uint32_t length);
    std::vector<uint8_t> generateArpeggio(const Scale& scale, uint32_t length, uint32_t seed);
};

}  // namespace procedural
