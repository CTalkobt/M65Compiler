#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include "procedural/Scale.hpp"
#include "procedural/Chord.hpp"
#include "procedural/ChordProgression.hpp"
#include "procedural/MelodicGenerator.hpp"
#include "procedural/HarmonicGenerator.hpp"
#include "procedural/RhythmicGenerator.hpp"

using namespace procedural;

// Test 1: Scale Creation and Queries
void test_scale_creation() {
    Scale major = Scale::Major(Note::C);
    assert(major.getRootNote() == 0);  // C = 0
    assert(major.getScaleDegree(0) == 0);   // C is degree 0
    assert(major.getScaleDegree(2) == 1);   // D is degree 1
    assert(major.getScaleDegree(4) == 2);   // E is degree 2
    std::cout << "✓ Scale creation test passed\n";
}

// Test 2: Scale Contains Checks
void test_scale_contains() {
    Scale pentatonic = Scale::Pentatonic(Note::A, true);  // A major pentatonic
    assert(pentatonic.contains(9));   // A
    assert(pentatonic.contains(11));  // B
    assert(!pentatonic.contains(8));  // G# (not in A major pentatonic)
    std::cout << "✓ Scale contains test passed\n";
}

// Test 3: Scale Navigation
void test_scale_navigation() {
    Scale minor = Scale::NaturalMinor(Note::E);
    uint8_t current = 4;  // E
    uint8_t next = minor.getNextNote(current);
    assert(next > current || next == 4);  // Should advance or wrap

    uint8_t prev = minor.getPreviousNote(current);
    assert(prev < current || prev == 4);  // Should go backward or wrap
    std::cout << "✓ Scale navigation test passed\n";
}

// Test 4: Chord Creation
void test_chord_creation() {
    Chord major(Note::G, ChordQuality::MAJOR);
    assert(major.getRootNote() == Note::G);
    assert(major.getQuality() == ChordQuality::MAJOR);
    assert(!major.getName().empty());
    std::cout << "✓ Chord creation test passed: " << major.getName() << "\n";
}

// Test 5: Chord Intervals
void test_chord_intervals() {
    Chord dominant(Note::D, ChordQuality::DOMINANT7);
    const auto& intervals = dominant.getIntervals();
    assert(!intervals.empty());
    assert(intervals[0] == 0);  // Root always present
    std::cout << "✓ Chord intervals test passed (" << intervals.size() << " tones)\n";
}

// Test 6: Chord Voicing
void test_chord_voicing() {
    Chord chord(Note::F, ChordQuality::MAJOR);
    auto voicing = chord.getVoicing(0, 4);  // Root position, octave 4
    assert(!voicing.empty());

    uint8_t rootVoicing = chord.getRootVoicing(4);
    assert(rootVoicing == static_cast<uint8_t>(Note::F) + 4 * 12);
    std::cout << "✓ Chord voicing test passed\n";
}

// Test 7: Chord Progression Creation
void test_chord_progression_creation() {
    ChordProgression prog(Key::G_MAJOR);
    assert(prog.getKey() == Key::G_MAJOR);
    assert(prog.getChordCount() == 0);

    Chord i(Note::G, ChordQuality::MAJOR);
    prog.addChord(i, 1920);  // 4 beats
    assert(prog.getChordCount() == 1);
    std::cout << "✓ Chord progression creation test passed\n";
}

// Test 8: Chord Progression Queries
void test_chord_progression_queries() {
    ChordProgression prog(Key::C_MAJOR);

    Chord c_major(Note::C, ChordQuality::MAJOR);
    Chord g_major(Note::G, ChordQuality::MAJOR);

    prog.addChord(c_major, 1920);  // 0-1920 ticks
    prog.addChord(g_major, 1920);  // 1920-3840 ticks

    auto chord_at_100 = prog.getChordAt(100);
    assert(chord_at_100 != nullptr);
    assert(chord_at_100->chord.getRootNote() == Note::C);

    auto chord_at_2000 = prog.getChordAt(2000);
    assert(chord_at_2000 != nullptr);
    assert(chord_at_2000->chord.getRootNote() == Note::G);

    assert(prog.getTotalDuration() == 3840);
    std::cout << "✓ Chord progression queries test passed\n";
}

// Test 9: Melodic Generation
void test_melodic_generation() {
    MelodicGenerator gen;
    Scale blues = Scale::Blues(Note::E);

    auto melody = gen.generateMarkovMelody(blues, 16, 42, 0.5f);
    assert(melody.size() == 16);

    // Check that all notes are in scale
    for (uint8_t pitch : melody) {
        assert(blues.contains(pitch % 12));
    }
    std::cout << "✓ Melodic generation test passed (" << melody.size() << " notes)\n";
}

// Test 10: Random Walk Melody
void test_random_walk_melody() {
    MelodicGenerator gen;
    Scale major = Scale::Major(Note::D);

    auto walk = gen.generateRandomWalk(major, 20, 123, 2, true);
    assert(walk.size() == 20);

    // Check all notes in scale
    for (uint8_t pitch : walk) {
        assert(major.contains(pitch % 12));
    }
    std::cout << "✓ Random walk melody test passed\n";
}

// Test 11: Harmonic Generation
void test_harmonic_generation() {
    HarmonicGenerator gen;

    auto jazz_prog = gen.generateJazzProgression(Key::F_MAJOR, 4, 42);
    assert(jazz_prog.getChordCount() > 0);

    auto blues_prog = gen.generateBluesProgression(Key::As_MAJOR, 12, 42);
    assert(blues_prog.getChordCount() > 0);

    auto pop_prog = gen.generatePopProgression(Key::E_MAJOR, 8, 42);
    assert(pop_prog.getChordCount() > 0);
    std::cout << "✓ Harmonic generation test passed\n";
}

// Test 12: Rhythmic Generation - Swing
void test_rhythmic_swing() {
    RhythmicGenerator gen;

    auto swing = gen.generateSwingRhythm(8, 42);
    assert(swing.size() == 8);
    // Swing should have alternating long-short pattern
    for (size_t i = 0; i < swing.size(); ++i) {
        if (i % 2 == 0) {
            assert(swing[i] > swing[i + 1] || i + 1 >= swing.size());
        }
    }
    std::cout << "✓ Rhythmic swing test passed\n";
}

// Test 13: Rhythmic Generation - Triplet
void test_rhythmic_triplet() {
    RhythmicGenerator gen;

    auto triplet = gen.generateTripletRhythm(4, 42);
    assert(triplet.size() == 12);  // 4 beats * 3 notes per beat
    std::cout << "✓ Rhythmic triplet test passed\n";
}

// Test 14: Rhythmic Drum Pattern
void test_rhythmic_drum_pattern() {
    RhythmicGenerator gen;

    auto drums = gen.generateDrumPattern(4, 480, 42, 0);  // Basic drummer
    assert(!drums.empty());

    // Check all drum hits are within time bounds
    uint32_t totalTicks = 4 * 4 * 480;  // 4 bars, 4 beats, 480 ticks
    for (const auto& [tick, intensity] : drums) {
        assert(tick < totalTicks);
        assert(intensity > 0 && intensity <= 127);
    }
    std::cout << "✓ Rhythmic drum pattern test passed (" << drums.size() << " hits)\n";
}

// Test 15: Time Signature Rhythm
void test_time_signature_rhythm() {
    RhythmicGenerator gen;

    auto waltz = gen.generateInTimeSignature(4, 3, 4, 42, RhythmComplexity::SIMPLE);
    assert(waltz.size() == 12);  // 4 bars * 3 beats

    auto compound = gen.generateInTimeSignature(2, 6, 8, 42, RhythmComplexity::SIMPLE);
    assert(compound.size() == 12);  // 2 bars * 6 beats
    std::cout << "✓ Time signature rhythm test passed\n";
}

// Test 16: All Modal Scales
void test_modal_scales() {
    Scale dorian = Scale::Dorian(Note::D);
    Scale phrygian = Scale::Phrygian(Note::E);
    Scale lydian = Scale::Lydian(Note::F);
    Scale mixolydian = Scale::Mixolydian(Note::G);

    assert(dorian.getRootNote() == 2);
    assert(phrygian.getRootNote() == 4);
    assert(lydian.getRootNote() == 5);
    assert(mixolydian.getRootNote() == 7);
    std::cout << "✓ Modal scales test passed\n";
}

// Test 17: Harmonic and Melodic Minor
void test_harmonic_melodic_minor() {
    Scale harmonic = Scale::HarmonicMinor(Note::A);
    Scale melodic = Scale::MelodicMinor(Note::A);

    // Harmonic minor should have raised 7th (11 semitones from root)
    const auto& harmonic_notes = harmonic.getNotes();
    assert(harmonic_notes.size() == 7);

    const auto& melodic_notes = melodic.getNotes();
    assert(melodic_notes.size() == 7);
    std::cout << "✓ Harmonic/Melodic minor test passed\n";
}

// Test 18: Chord Quality Coverage
void test_chord_qualities() {
    Note root = Note::C;

    Chord major(root, ChordQuality::MAJOR);
    Chord minor(root, ChordQuality::MINOR);
    Chord dim(root, ChordQuality::DIMINISHED);
    Chord aug(root, ChordQuality::AUGMENTED);
    Chord maj7(root, ChordQuality::MAJOR7);
    Chord dom7(root, ChordQuality::DOMINANT7);
    Chord sus2(root, ChordQuality::SUSPENDED2);
    Chord sus4(root, ChordQuality::SUSPENDED4);

    assert(!major.getName().empty());
    assert(!minor.getName().empty());
    assert(!dim.getName().empty());
    assert(!aug.getName().empty());
    assert(!maj7.getName().empty());
    assert(!dom7.getName().empty());
    assert(!sus2.getName().empty());
    assert(!sus4.getName().empty());
    std::cout << "✓ Chord qualities test passed (8 types)\n";
}

// Test 19: Melodic Strategy Variations
void test_melodic_strategies() {
    MelodicGenerator gen;
    Scale major = Scale::Major(Note::G);
    uint32_t seed = 999;

    // Test different strategies
    auto markov = gen.generateMarkovMelody(major, 8, seed, 0.5f);
    auto walk = gen.generateRandomWalk(major, 8, seed, 2, true);
    auto ascending = gen.generate(major, 3840, seed, MelodicStrategy::ASCENDING);
    auto descending = gen.generate(major, 3840, seed, MelodicStrategy::DESCENDING);

    assert(markov.size() == 8);
    assert(walk.size() == 8);
    assert(!ascending.empty());
    assert(!descending.empty());
    std::cout << "✓ Melodic strategies test passed\n";
}

// Test 20: Comprehensive Progression
void test_comprehensive_progression() {
    ChordProgression prog(Key::As_MAJOR);

    Chord i(Note::As, ChordQuality::MAJOR);
    Chord iv(Note::Ds, ChordQuality::MAJOR);
    Chord v(Note::F, ChordQuality::MAJOR);

    prog.addChord(i, 1920);
    prog.addChord(iv, 1920);
    prog.addChord(v, 1920);
    prog.addChord(i, 1920);

    assert(prog.getChordCount() == 4);
    assert(prog.getTotalDuration() == 7680);
    assert(prog.getChordIndex(100) == 0);
    assert(prog.getChordIndex(2000) == 1);
    assert(prog.getChordIndex(4000) == 2);
    assert(prog.getChordIndex(6000) == 3);
    std::cout << "✓ Comprehensive progression test passed\n";
}

int main() {
    std::cout << "\n=== Phase 13: Procedural Generation Engine Tests ===\n";

    test_scale_creation();
    test_scale_contains();
    test_scale_navigation();
    test_chord_creation();
    test_chord_intervals();
    test_chord_voicing();
    test_chord_progression_creation();
    test_chord_progression_queries();
    test_melodic_generation();
    test_random_walk_melody();
    test_harmonic_generation();
    test_rhythmic_swing();
    test_rhythmic_triplet();
    test_rhythmic_drum_pattern();
    test_time_signature_rhythm();
    test_modal_scales();
    test_harmonic_melodic_minor();
    test_chord_qualities();
    test_melodic_strategies();
    test_comprehensive_progression();

    std::cout << "\n✅ All 20 Phase 13 tests passed!\n\n";
    return 0;
}
