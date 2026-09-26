#include "procedural/HarmonicGenerator.hpp"
#include <algorithm>

namespace procedural {

HarmonicGenerator::HarmonicGenerator() = default;

ChordProgression HarmonicGenerator::generateProgression(
    Key key,
    uint32_t lengthBars,
    uint32_t seed,
    ProgressionStyle style,
    float complexity,
    uint32_t ticksPerBeat) {

    switch (style) {
        case ProgressionStyle::JAZZ:
            return generateJazzProgression(key, lengthBars, seed);

        case ProgressionStyle::BLUES:
            return generateBluesProgression(key, lengthBars, seed);

        case ProgressionStyle::POP:
            return generatePopProgression(key, lengthBars, seed);

        case ProgressionStyle::CLASSICAL:
            return generateClassicalProgression(key, lengthBars, seed);

        case ProgressionStyle::FOLK:
            return generateFolkProgression(key, lengthBars, seed);

        case ProgressionStyle::MODAL:
        case ProgressionStyle::MINIMALIST:
        case ProgressionStyle::CHROMATIC:
        default:
            // Fallback to pop
            return generatePopProgression(key, lengthBars, seed);
    }
}

ChordProgression HarmonicGenerator::generateJazzProgression(Key key, uint32_t bars, uint32_t seed) {
    ChordProgression prog(key);

    // Extract root note from key
    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    // Common jazz progression: ii-V-I-vi
    Chord ii(static_cast<Note>((static_cast<uint8_t>(root) + 2) % 12), ChordQuality::MINOR7);
    Chord V(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::DOMINANT7);
    Chord I(root, ChordQuality::MAJOR7);
    Chord vi(static_cast<Note>((static_cast<uint8_t>(root) + 9) % 12), ChordQuality::MINOR7);

    uint32_t barsPerChord = std::max(1u, bars / 4);
    uint32_t ticksPerBeat = 480;  // Standard

    // Add chords
    for (uint32_t i = 0; i < bars; i += barsPerChord) {
        if (i == 0) prog.addChord(ii, barsPerChord * 4 * ticksPerBeat, selectVoicing(0, bars, seed));
        else if (i % 16 == 0) prog.addChord(vi, barsPerChord * 4 * ticksPerBeat, selectVoicing(i, bars, seed));
        else if (i % 8 == 0) prog.addChord(V, barsPerChord * 4 * ticksPerBeat, selectVoicing(i, bars, seed));
        else prog.addChord(I, barsPerChord * 4 * ticksPerBeat, selectVoicing(i, bars, seed));
    }

    return prog;
}

ChordProgression HarmonicGenerator::generateBluesProgression(Key key, uint32_t bars, uint32_t seed) {
    ChordProgression prog(key);

    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    Chord I(root, ChordQuality::DOMINANT7);
    Chord IV(static_cast<Note>((static_cast<uint8_t>(root) + 5) % 12), ChordQuality::DOMINANT7);
    Chord V(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::DOMINANT7);

    uint32_t ticksPerBeat = 480;

    // 12-bar blues: I(4) - IV(2) - I(2) - V(2) - IV(1) - I(2) - V(1)
    uint32_t barsDone = 0;
    if (barsDone + 4 <= bars) { prog.addChord(I, 4 * 4 * ticksPerBeat, 0); barsDone += 4; }
    if (barsDone + 2 <= bars) { prog.addChord(IV, 2 * 4 * ticksPerBeat, 0); barsDone += 2; }
    if (barsDone + 2 <= bars) { prog.addChord(I, 2 * 4 * ticksPerBeat, 0); barsDone += 2; }
    if (barsDone + 2 <= bars) { prog.addChord(V, 2 * 4 * ticksPerBeat, 0); barsDone += 2; }

    // Fill remaining bars with I
    while (barsDone < bars) {
        uint32_t remaining = bars - barsDone;
        uint32_t add = std::min(remaining, 2u);
        prog.addChord(I, add * 4 * ticksPerBeat, selectVoicing(barsDone, bars, seed));
        barsDone += add;
    }

    return prog;
}

ChordProgression HarmonicGenerator::generatePopProgression(Key key, uint32_t bars, uint32_t seed) {
    ChordProgression prog(key);

    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    // I-V-vi-IV (very popular)
    Chord I(root, ChordQuality::MAJOR);
    Chord V(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::MAJOR);
    Chord vi(static_cast<Note>((static_cast<uint8_t>(root) + 9) % 12), ChordQuality::MINOR);
    Chord IV(static_cast<Note>((static_cast<uint8_t>(root) + 5) % 12), ChordQuality::MAJOR);

    uint32_t barsPerChord = std::max(1u, bars / 4);
    uint32_t ticksPerBeat = 480;

    for (uint32_t i = 0; i < bars; i += barsPerChord) {
        uint32_t pos = i / barsPerChord;
        const Chord* chords[] = { &I, &V, &vi, &IV };
        const Chord* chord = chords[pos % 4];
        prog.addChord(*chord, barsPerChord * 4 * ticksPerBeat, selectVoicing(i, bars, seed));
    }

    return prog;
}

ChordProgression HarmonicGenerator::generateClassicalProgression(Key key, uint32_t bars, uint32_t seed) {
    ChordProgression prog(key);

    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    // Classical sonata form: I - IV - V - I (simplified)
    Chord I(root, ChordQuality::MAJOR);
    Chord IV(static_cast<Note>((static_cast<uint8_t>(root) + 5) % 12), ChordQuality::MAJOR);
    Chord V(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::MAJOR);

    uint32_t barsPerPhrase = std::max(2u, bars / 4);
    uint32_t ticksPerBeat = 480;

    // Exposition
    if (0 + barsPerPhrase <= bars) prog.addChord(I, barsPerPhrase * 4 * ticksPerBeat, 0);
    if (barsPerPhrase + barsPerPhrase <= bars) prog.addChord(V, barsPerPhrase * 4 * ticksPerBeat, 0);

    // Development
    if (2 * barsPerPhrase + barsPerPhrase <= bars) prog.addChord(IV, barsPerPhrase * 4 * ticksPerBeat, 0);

    // Recapitulation
    if (3 * barsPerPhrase <= bars) prog.addChord(I, (bars - 3 * barsPerPhrase) * 4 * ticksPerBeat, 0);

    return prog;
}

ChordProgression HarmonicGenerator::generateFolkProgression(Key key, uint32_t bars, uint32_t seed) {
    ChordProgression prog(key);

    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    // Simple folk: I-IV-I-V or I-V-I-V
    Chord I(root, ChordQuality::MAJOR);
    Chord IV(static_cast<Note>((static_cast<uint8_t>(root) + 5) % 12), ChordQuality::MAJOR);
    Chord V(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::MAJOR);

    uint32_t barsPerChord = std::max(1u, bars / 4);
    uint32_t ticksPerBeat = 480;

    for (uint32_t i = 0; i < bars; i += barsPerChord) {
        uint32_t pos = i / barsPerChord;
        const Chord* chords[] = { &I, &IV, &I, &V };
        const Chord* chord = chords[pos % 4];
        prog.addChord(*chord, barsPerChord * 4 * ticksPerBeat, 0);
    }

    return prog;
}

Chord HarmonicGenerator::getSecondaryDominant(const Chord& targetChord, Key key) {
    // Secondary dominant is a V7 chord a fifth above the target
    uint8_t targetRoot = static_cast<uint8_t>(targetChord.getRootNote());
    uint8_t dominantRoot = (targetRoot + 7) % 12;
    return Chord(static_cast<Note>(dominantRoot), ChordQuality::DOMINANT7);
}

ChordProgression HarmonicGenerator::addModulation(const ChordProgression& prog, Key newKey, uint32_t modBar) {
    // Create new progression with key change at specified bar
    ChordProgression result = prog;
    result.setKey(newKey);
    return result;
}

uint8_t HarmonicGenerator::selectVoicing(size_t chordIndex, size_t progressionLength, uint32_t seed) {
    // Simple voicing selection based on position
    uint32_t rng = seed ^ static_cast<uint32_t>(chordIndex);
    return (rng / 65536) % 4;  // 4 different voicing styles
}

std::vector<Chord> HarmonicGenerator::getScaleTriads(Key key) {
    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    // Major scale triads: I, ii, iii, IV, V, vi, vii°
    std::vector<Chord> triads;
    triads.emplace_back(root, ChordQuality::MAJOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 2) % 12), ChordQuality::MINOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 4) % 12), ChordQuality::MINOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 5) % 12), ChordQuality::MAJOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 7) % 12), ChordQuality::MAJOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 9) % 12), ChordQuality::MINOR);
    triads.emplace_back(static_cast<Note>((static_cast<uint8_t>(root) + 11) % 12), ChordQuality::DIMINISHED);

    return triads;
}

Chord HarmonicGenerator::romanToChord(int roman, Key key) {
    Note root = static_cast<Note>(static_cast<uint8_t>(key) % 12);

    int intervals[] = { 0, 2, 4, 5, 7, 9, 11 };
    int interval = intervals[roman % 7];
    Note chordRoot = static_cast<Note>((static_cast<uint8_t>(root) + interval) % 12);

    // Simplified: just use major for odd positions, minor for even
    ChordQuality quality = (roman % 2 == 0) ? ChordQuality::MAJOR : ChordQuality::MINOR;

    return Chord(chordRoot, quality);
}

}  // namespace procedural
