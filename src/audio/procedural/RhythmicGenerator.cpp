#include "procedural/RhythmicGenerator.hpp"
#include <algorithm>
#include <cmath>

namespace procedural {

RhythmicGenerator::RhythmicGenerator() = default;

std::vector<uint32_t> RhythmicGenerator::generatePattern(
    uint32_t lengthTicks,
    uint32_t ticksPerBeat,
    uint32_t seed,
    RhythmPattern pattern,
    RhythmComplexity complexity) {

    switch (pattern) {
        case RhythmPattern::SWING:
            return generateSwingRhythm(lengthTicks / ticksPerBeat, seed);

        case RhythmPattern::TRIPLET:
            return generateTripletRhythm(lengthTicks / ticksPerBeat, seed);

        case RhythmPattern::SYNCOPATED:
            return generateSyncopatedRhythm(lengthTicks / ticksPerBeat, seed);

        case RhythmPattern::POLYRHYTHMIC:
            return generatePolyrhythmicPattern(lengthTicks / ticksPerBeat, seed);

        case RhythmPattern::STEADY:
        case RhythmPattern::OFFBEAT:
        case RhythmPattern::BROKEN:
        case RhythmPattern::OSTINATO:
        default:
            return generateRandomDurations(lengthTicks / ticksPerBeat, seed);
    }
}

std::vector<uint32_t> RhythmicGenerator::generateSwingRhythm(uint32_t length, uint32_t seed) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    uint32_t rng = seed;

    // Swing: alternating long-short pattern
    bool isLong = true;
    for (uint32_t i = 0; i < length; ++i) {
        if (isLong) {
            pattern.push_back(320);  // Long note (2/3 of beat)
        } else {
            pattern.push_back(160);  // Short note (1/3 of beat)
        }
        isLong = !isLong;
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generateTripletRhythm(uint32_t length, uint32_t seed) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    // Triplet: three notes per beat
    uint32_t ticksPerTriplet = 160;  // 480 / 3

    for (uint32_t i = 0; i < length; ++i) {
        pattern.push_back(ticksPerTriplet);
        pattern.push_back(ticksPerTriplet);
        pattern.push_back(ticksPerTriplet);
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generateSyncopatedRhythm(uint32_t length, uint32_t seed) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    uint32_t rng = seed;

    // Syncopation: off-beat emphasis with varied durations
    for (uint32_t i = 0; i < length; ++i) {
        rng = rng * 1103515245 + 12345;
        uint32_t r = (rng / 65536) % 32768;

        // Vary note duration: 120 (sixteenth), 240 (eighth), 480 (quarter)
        std::vector<uint32_t> durations = { 120, 240, 240, 480 };
        pattern.push_back(durations[r % durations.size()]);
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generatePolyrhythmicPattern(uint32_t length, uint32_t seed) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    uint32_t rng = seed;

    // Polyrhythm: overlapping rhythm cycles
    for (uint32_t i = 0; i < length; ++i) {
        rng = rng * 1103515245 + 12345;

        // Mix of duple (2) and triple (3) subdivisions
        if (i % 6 == 0) {
            pattern.push_back(240);  // Every 3rd beat (duple)
        } else if (i % 3 == 0) {
            pattern.push_back(160);  // Every beat (triple)
        } else {
            pattern.push_back(240);  // Quarter
        }
    }

    return pattern;
}

std::vector<std::pair<uint32_t, uint8_t>> RhythmicGenerator::generateDrumPattern(
    uint32_t lengthBars,
    uint32_t ticksPerBeat,
    uint32_t seed,
    uint8_t drummer) {

    std::vector<std::pair<uint32_t, uint8_t>> pattern;

    uint32_t rng = seed;
    uint32_t beatCount = lengthBars * 4;  // 4 beats per bar

    for (uint32_t beat = 0; beat < beatCount; ++beat) {
        uint32_t tick = beat * ticksPerBeat;

        rng = rng * 1103515245 + 12345;
        uint32_t r = (rng / 65536) % 32768;

        // Drummer styles
        switch (drummer) {
            case 0: {  // Basic: kick on 1,3, snare on 2,4
                if (beat % 4 == 0 || beat % 4 == 2) {  // Kick drum
                    pattern.emplace_back(tick, 100);
                }
                if (beat % 4 == 1 || beat % 4 == 3) {  // Snare
                    pattern.emplace_back(tick, 80);
                }
                break;
            }

            case 1: {  // Jazz: more complex hi-hat and kick patterns
                if (beat % 8 == 0 || beat % 8 == 2) {
                    pattern.emplace_back(tick, 100);  // Kick
                }
                if ((beat + 1) % 4 == 0) {
                    pattern.emplace_back(tick, 85);  // Snare
                }
                // Hi-hats on eighth notes
                for (int i = 0; i < 2; ++i) {
                    pattern.emplace_back(tick + i * ticksPerBeat / 2, 60);
                }
                break;
            }

            case 2: {  // Funk: syncopated kick and snare
                if (beat % 4 == 0) {
                    pattern.emplace_back(tick, 110);  // Kick
                }
                if (beat % 4 == 1) {
                    pattern.emplace_back(tick + ticksPerBeat / 2, 90);  // Syncopated snare
                }
                break;
            }

            case 3: {  // Metal: double bass drum and kick
                if (beat % 2 == 0) {
                    pattern.emplace_back(tick, 120);  // Double kick
                }
                if ((beat + 1) % 4 == 0) {
                    pattern.emplace_back(tick, 100);  // Kick
                }
                break;
            }

            default:
                // Basic fallback
                if (beat % 4 == 0) {
                    pattern.emplace_back(tick, 100);
                }
                break;
        }
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generateInTimeSignature(
    uint32_t lengthBars,
    uint8_t numerator,
    uint8_t denominator,
    uint32_t seed,
    RhythmComplexity complexity) {

    std::vector<uint32_t> pattern;

    uint32_t rng = seed;
    uint32_t ticksPerBeat = 480;
    uint32_t beatsPerBar = numerator;

    if (denominator == 8) beatsPerBar = numerator;  // Eighth note gets the beat
    else if (denominator == 16) beatsPerBar = numerator * 2;  // Sixteenth note gets beat

    for (uint32_t bar = 0; bar < lengthBars; ++bar) {
        for (uint32_t beat = 0; beat < beatsPerBar; ++beat) {
            rng = rng * 1103515245 + 12345;
            uint32_t r = (rng / 65536) % 32768;

            uint32_t duration = ticksPerBeat;

            if (complexity == RhythmComplexity::SIMPLE) {
                duration = ticksPerBeat;
            } else if (complexity == RhythmComplexity::MODERATE) {
                std::vector<uint32_t> durations = { ticksPerBeat / 2, ticksPerBeat, ticksPerBeat * 2 };
                duration = durations[r % durations.size()];
            } else if (complexity == RhythmComplexity::COMPLEX) {
                std::vector<uint32_t> durations = {
                    ticksPerBeat / 4, ticksPerBeat / 3, ticksPerBeat / 2,
                    ticksPerBeat, ticksPerBeat * 2, ticksPerBeat * 3
                };
                duration = durations[r % durations.size()];
            } else {
                // POLYRHYTHMIC: advanced patterns
                duration = ticksPerBeat / 2 + (r % (ticksPerBeat / 2));
            }

            pattern.push_back(duration);
        }
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generateRandomDurations(uint32_t length, uint32_t seed) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    uint32_t rng = seed;

    // Common note durations: sixteenth (120), eighth (240), quarter (480), half (960)
    std::vector<uint32_t> durations = { 120, 240, 480, 960 };

    for (uint32_t i = 0; i < length; ++i) {
        rng = rng * 1103515245 + 12345;
        uint32_t r = (rng / 65536) % 32768;
        pattern.push_back(durations[r % durations.size()]);
    }

    return pattern;
}

std::vector<uint32_t> RhythmicGenerator::generateEvenDurations(uint32_t length, uint32_t evenness) {
    std::vector<uint32_t> pattern;
    pattern.reserve(length);

    // Evenness: 0 = very varied, 100 = all same
    // For simplicity, just return steady quarter notes when evenness is high
    uint32_t baseDuration = 480;  // Quarter note

    for (uint32_t i = 0; i < length; ++i) {
        pattern.push_back(baseDuration);
    }

    return pattern;
}

}  // namespace procedural
