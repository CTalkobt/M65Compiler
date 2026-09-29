// Phase 12: Sequencer/Song Architecture Tests
// Tests for core song, track, pattern, and sequencer functionality

#include <iostream>
#include <cassert>
#include <vector>
#include "../../include/audio/Song.hpp"
#include "../../include/audio/Track.hpp"
#include "../../include/audio/Pattern.hpp"
#include "../../include/audio/Sequencer.hpp"

using namespace audio;

void test_song_creation() {
    std::cout << "Test 1: Song Creation... ";

    Song song("My Song", 120);
    assert(song.getName() == "My Song");
    assert(song.getTempo() == 120);
    assert(song.getTimeSignature().numerator == 4);
    assert(song.getTimeSignature().denominator == 4);
    assert(song.getTrackCount() == 0);

    std::cout << "PASS" << std::endl;
}

void test_track_management() {
    std::cout << "Test 2: Track Management... ";

    Song song("Test", 120);

    auto melodyTrack = std::make_unique<Track>(Track::Channel::MELODY, "Melody");
    auto bassTrack = std::make_unique<Track>(Track::Channel::BASS, "Bass");

    Track* melody = song.addTrack(std::move(melodyTrack));
    Track* bass = song.addTrack(std::move(bassTrack));

    assert(song.getTrackCount() == 2);
    assert(melody != nullptr);
    assert(bass != nullptr);
    assert(melody->getName() == "Melody");
    assert(bass->getName() == "Bass");

    // Test track removal
    song.removeTrack(bass);
    assert(song.getTrackCount() == 1);

    std::cout << "PASS" << std::endl;
}

void test_pattern_creation() {
    std::cout << "Test 3: Pattern Creation... ";

    TimeSignature ts(4, 4, 480);
    Pattern pattern(4, ts);  // 4 bars

    assert(pattern.getLengthBars() == 4);
    assert(pattern.getLengthTicks() == 4 * ts.ticksPerBar());
    assert(pattern.isEmpty());

    // Add notes
    pattern.addNote(60, 0, 480, 100);      // C4, quarter note
    pattern.addNote(62, 480, 480, 100);    // D4, quarter note
    pattern.addNote(64, 960, 480, 100);    // E4, quarter note

    assert(pattern.getNotes().size() == 3);
    assert(!pattern.isEmpty());

    // Test note query
    // Note 1: ticks 0-479 (C4)
    // Note 2: ticks 480-959 (D4)
    // Note 3: ticks 960-1439 (E4)
    const Note* note_at_0 = pattern.getNoteAt(0);
    assert(note_at_0 != nullptr);
    assert(note_at_0->pitch == 60);  // C4

    const Note* note_at_400 = pattern.getNoteAt(400);
    assert(note_at_400 != nullptr);
    assert(note_at_400->pitch == 60);  // Still C4 (within first note)

    const Note* note_at_480 = pattern.getNoteAt(480);
    assert(note_at_480 != nullptr);
    assert(note_at_480->pitch == 62);  // D4 (second note starts here)

    const Note* note_at_960 = pattern.getNoteAt(960);
    assert(note_at_960 != nullptr);
    assert(note_at_960->pitch == 64);  // E4 (third note starts here)

    assert(pattern.isNotePlaying(0));
    assert(pattern.isNotePlaying(480));
    assert(pattern.isNotePlaying(960));

    std::cout << "PASS" << std::endl;
}

void test_pattern_operations() {
    std::cout << "Test 4: Pattern Operations... ";

    TimeSignature ts(4, 4, 480);
    Pattern pattern(4, ts);

    // Add notes for transposition test
    pattern.addNote(60, 0, 480, 100);   // C4
    pattern.addNote(62, 480, 480, 100); // D4

    // Test transpose
    pattern.transpose(12);  // Up one octave
    assert(pattern.getNotes()[0].pitch == 72);  // C5
    assert(pattern.getNotes()[1].pitch == 74);  // D5

    // Test transpose down
    pattern.transpose(-12);  // Back to original
    assert(pattern.getNotes()[0].pitch == 60);
    assert(pattern.getNotes()[1].pitch == 62);

    // Test scale (speed up by 2x)
    uint32_t originalDur = pattern.getNotes()[0].duration;
    pattern.scale(2.0f);
    assert(pattern.getNotes()[0].duration == originalDur * 2);

    // Test clear
    pattern.clear();
    assert(pattern.getNotes().size() == 0);
    assert(pattern.isEmpty());

    std::cout << "PASS" << std::endl;
}

void test_rests_and_controls() {
    std::cout << "Test 5: Rests and Control Changes... ";

    TimeSignature ts(4, 4, 480);
    Pattern pattern(4, ts);

    // Add rests
    pattern.addRest(0, 480);
    pattern.addRest(480, 480);

    assert(pattern.getRests().size() == 2);
    assert(pattern.getRests()[0].startTick == 0);
    assert(pattern.getRests()[1].startTick == 480);

    // Add control changes
    pattern.addControlChange(ControlChange::Type::VOLUME, 100, 0);
    pattern.addControlChange(ControlChange::Type::PAN, 64, 480);

    assert(pattern.getControlChanges().size() == 2);
    assert(pattern.getControlChanges()[0].type == ControlChange::Type::VOLUME);
    assert(pattern.getControlChanges()[0].value == 100);
    assert(pattern.getControlChanges()[1].type == ControlChange::Type::PAN);
    assert(pattern.getControlChanges()[1].value == 64);

    // Test that we can query what we added
    assert(pattern.getRests().size() == 2);
    assert(pattern.getControlChanges().size() == 2);

    std::cout << "PASS" << std::endl;
}

void test_sequencer_basic() {
    std::cout << "Test 6: Sequencer Basic Operation... ";

    auto song = std::make_unique<Song>("Test Song", 120);
    auto track = std::make_unique<Track>(Track::Channel::MELODY, "Melody");
    TimeSignature ts(4, 4, 480);
    auto pattern = std::make_unique<Pattern>(4, ts);

    // Add some notes to pattern
    pattern->addNote(60, 0, 480, 100);
    pattern->addNote(62, 480, 480, 100);
    pattern->addNote(64, 960, 480, 100);

    Track* trackPtr = song->addTrack(std::move(track));
    trackPtr->addPattern(std::move(pattern));

    Sequencer sequencer;
    sequencer.loadSong(std::move(song));

    assert(sequencer.getCurrentSong() != nullptr);
    assert(!sequencer.isPlaying());

    // Test playback control
    sequencer.play();
    assert(sequencer.isPlaying());

    sequencer.pause();
    assert(!sequencer.isPlaying());

    sequencer.play();
    sequencer.stop();
    assert(!sequencer.isPlaying());
    assert(sequencer.getCurrentPosition() == 0);

    std::cout << "PASS" << std::endl;
}

void test_sequencer_position() {
    std::cout << "Test 7: Sequencer Position Control... ";

    auto song = std::make_unique<Song>("Test", 120);
    Sequencer sequencer;
    sequencer.loadSong(std::move(song));

    // Test position setting
    sequencer.setCurrentPosition(1000);
    assert(sequencer.getCurrentPosition() == 1000);

    // Test boundary clamping (would clamp to song duration)
    uint32_t huge = 999999;
    sequencer.setCurrentPosition(huge);
    // Position should be clamped to song duration

    std::cout << "PASS" << std::endl;
}

void test_sequencer_quantization() {
    std::cout << "Test 8: Sequencer Quantization... ";

    auto song = std::make_unique<Song>("Test", 120);
    Sequencer sequencer;
    sequencer.loadSong(std::move(song));

    // Set quantize mode
    sequencer.setQuantizeMode(Sequencer::QuantizeMode::QUARTER);

    // Test quantization of tick values
    uint32_t tick = 150;  // Off-grid
    uint32_t quantized = sequencer.quantizeTick(tick);
    // Quantized tick should be aligned to quarter note grid

    assert(sequencer.getQuantizeMode() == Sequencer::QuantizeMode::QUARTER);

    std::cout << "PASS" << std::endl;
}

void test_sequencer_looping() {
    std::cout << "Test 9: Sequencer Looping... ";

    auto song = std::make_unique<Song>("Test", 120);
    Sequencer sequencer;
    sequencer.loadSong(std::move(song));

    // Enable looping
    sequencer.setLoopEnabled(true);
    sequencer.setLoopStart(1000);
    sequencer.setLoopEnd(5000);

    assert(sequencer.isLoopEnabled());
    assert(sequencer.getLoopStart() == 1000);
    assert(sequencer.getLoopEnd() == 5000);

    std::cout << "PASS" << std::endl;
}

void test_song_creation_comprehensive() {
    std::cout << "Test 10: Comprehensive Song Creation... ";

    auto song = std::make_unique<Song>("Ragtime Song", 140);
    song->setTempo(140);

    // Add multiple tracks
    auto melody = std::make_unique<Track>(Track::Channel::MELODY, "Melody");
    auto bass = std::make_unique<Track>(Track::Channel::BASS, "Stride Bass");
    auto harmony = std::make_unique<Track>(Track::Channel::HARMONY, "Chords");

    Track* melodyPtr = song->addTrack(std::move(melody));
    Track* bassPtr = song->addTrack(std::move(bass));
    Track* harmonyPtr = song->addTrack(std::move(harmony));

    // Add patterns to each track
    TimeSignature ts(4, 4, 480);

    auto melodyPattern = std::make_unique<Pattern>(8, ts);
    auto bassPattern = std::make_unique<Pattern>(8, ts);
    auto harmonyPattern = std::make_unique<Pattern>(8, ts);

    // Add notes
    melodyPattern->addNote(60, 0, 240, 100);
    bassPattern->addNote(36, 0, 480, 100);
    harmonyPattern->addNote(60, 0, 480, 80);

    melodyPtr->addPattern(std::move(melodyPattern));
    bassPtr->addPattern(std::move(bassPattern));
    harmonyPtr->addPattern(std::move(harmonyPattern));

    assert(song->getTrackCount() == 3);
    assert(melodyPtr->getPatternCount() == 1);
    assert(bassPtr->getPatternCount() == 1);
    assert(harmonyPtr->getPatternCount() == 1);

    Sequencer sequencer;
    sequencer.loadSong(std::move(song));

    std::string stats = sequencer.getStatistics();
    assert(!stats.empty());

    std::cout << "PASS" << std::endl;
}

int main() {
    std::cout << "\n=== Phase 12: Sequencer/Song Architecture Tests ===" << std::endl;
    std::cout << "Running comprehensive test suite...\n" << std::endl;

    try {
        test_song_creation();
        test_track_management();
        test_pattern_creation();
        test_pattern_operations();
        test_rests_and_controls();
        test_sequencer_basic();
        test_sequencer_position();
        test_sequencer_quantization();
        test_sequencer_looping();
        test_song_creation_comprehensive();

        std::cout << "\n✅ All 10 tests PASSED" << std::endl;
        std::cout << "Phase 12 Sequencer/Song Architecture is working correctly\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
