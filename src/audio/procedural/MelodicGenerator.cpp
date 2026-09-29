#include "procedural/MelodicGenerator.hpp"
#include <cmath>
#include <algorithm>

namespace procedural {

MelodicGenerator::MelodicGenerator() = default;

std::vector<uint8_t> MelodicGenerator::generate(
    const Scale& scale,
    uint32_t lengthTicks,
    uint32_t seed,
    MelodicStrategy strategy,
    float complexity,
    MelodicDirection direction) {

    uint32_t estimatedNotes = lengthTicks / 240;  // Rough estimate

    switch (strategy) {
        case MelodicStrategy::MARKOV_CHAIN:
            return generateMarkovMelody(scale, estimatedNotes, seed, complexity);

        case MelodicStrategy::RANDOM_WALK:
            return generateRandomWalk(scale, estimatedNotes, seed, 2, true);

        case MelodicStrategy::ASCENDING:
            return generateAscending(scale, estimatedNotes);

        case MelodicStrategy::DESCENDING:
            return generateDescending(scale, estimatedNotes);

        case MelodicStrategy::ARPEGGIO:
            return generateArpeggio(scale, estimatedNotes, seed);

        case MelodicStrategy::PENTATONIC:
            {
                auto pentatonic = Scale::Pentatonic(static_cast<Note>(scale.getRootNote()));
                return generateRandomWalk(pentatonic, estimatedNotes, seed, 1, false);
            }

        default:
        case MelodicStrategy::GENETIC:
            // Fallback to random walk
            return generateRandomWalk(scale, estimatedNotes, seed, 2, true);
    }
}

std::vector<uint8_t> MelodicGenerator::generateFromPattern(
    const Scale& scale,
    const std::vector<int>& intervals,
    uint8_t startPitch,
    uint32_t noteDuration,
    uint32_t seed) {

    std::vector<uint8_t> melody;
    melody.reserve(intervals.size());

    uint8_t currentPitch = startPitch;
    for (int interval : intervals) {
        int newPitch = static_cast<int>(currentPitch) + interval;
        if (newPitch >= 0 && newPitch <= 127) {
            currentPitch = static_cast<uint8_t>(newPitch);
            melody.push_back(currentPitch);
        }
    }

    return melody;
}

std::vector<uint8_t> MelodicGenerator::generateMarkovMelody(
    const Scale& scale,
    uint32_t lengthNotes,
    uint32_t seed,
    float complexity) {

    std::vector<uint8_t> melody;
    if (lengthNotes == 0) return melody;

    const auto& notes = scale.getNotes();
    if (notes.empty()) return melody;

    melody.reserve(lengthNotes);

    // Start on root note
    uint8_t current = notes[0];
    melody.push_back(current);

    // LCG random number generator seeded with user seed
    uint32_t rng = seed;

    for (uint32_t i = 1; i < lengthNotes; ++i) {
        rng = rng * 1103515245 + 12345;
        uint32_t r = (rng / 65536) % 32768;

        // Find current note in scale
        int currentIndex = scale.getScaleDegree(current);
        if (currentIndex < 0) currentIndex = 0;

        // Transition probabilities based on complexity
        // Higher complexity = more varied transitions
        int maxStep = static_cast<int>(1.0f + complexity * 3.0f);
        int step = static_cast<int>((r % (2 * maxStep + 1)) - maxStep);

        int nextIndex = currentIndex + step;
        nextIndex = std::max(0, std::min(static_cast<int>(notes.size() - 1), nextIndex));

        current = notes[nextIndex];
        melody.push_back(current);
    }

    return melody;
}

std::vector<uint8_t> MelodicGenerator::generateRandomWalk(
    const Scale& scale,
    uint32_t lengthNotes,
    uint32_t seed,
    int maxStep,
    bool tendencyTowardCenter) {

    std::vector<uint8_t> melody;
    if (lengthNotes == 0) return melody;

    const auto& notes = scale.getNotes();
    if (notes.empty()) return melody;

    melody.reserve(lengthNotes);

    // Start in middle of scale
    int currentIndex = notes.size() / 2;
    melody.push_back(notes[currentIndex]);

    uint32_t rng = seed;

    for (uint32_t i = 1; i < lengthNotes; ++i) {
        rng = rng * 1103515245 + 12345;
        uint32_t r = (rng / 65536) % 32768;

        // Random step
        int step = static_cast<int>((r % (2 * maxStep + 1)) - maxStep);

        // Tendency toward center (middle of scale)
        if (tendencyTowardCenter) {
            int centerIndex = notes.size() / 2;
            if (currentIndex < centerIndex && step < 0) step = -step;
            else if (currentIndex > centerIndex && step > 0) step = -step;
        }

        int nextIndex = currentIndex + step;
        nextIndex = std::max(0, std::min(static_cast<int>(notes.size() - 1), nextIndex));

        currentIndex = nextIndex;
        melody.push_back(notes[currentIndex]);
    }

    return melody;
}

std::vector<uint8_t> MelodicGenerator::generateAscending(const Scale& scale, uint32_t length) {
    std::vector<uint8_t> melody;
    const auto& notes = scale.getNotes();

    for (uint32_t i = 0; i < length; ++i) {
        melody.push_back(notes[i % notes.size()]);
    }

    return melody;
}

std::vector<uint8_t> MelodicGenerator::generateDescending(const Scale& scale, uint32_t length) {
    std::vector<uint8_t> melody;
    const auto& notes = scale.getNotes();

    for (uint32_t i = 0; i < length; ++i) {
        int index = notes.size() - 1 - (i % notes.size());
        melody.push_back(notes[index]);
    }

    return melody;
}

std::vector<uint8_t> MelodicGenerator::generateArpeggio(const Scale& scale, uint32_t length, uint32_t seed) {
    std::vector<uint8_t> melody;
    const auto& notes = scale.getNotes();

    uint32_t rng = seed;
    for (uint32_t i = 0; i < length; ++i) {
        rng = rng * 1103515245 + 12345;
        uint8_t index = (rng / 65536) % notes.size();
        melody.push_back(notes[index]);
    }

    return melody;
}

}  // namespace procedural
