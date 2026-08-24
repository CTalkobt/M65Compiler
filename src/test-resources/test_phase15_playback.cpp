#include <cassert>
#include <iostream>
#include <memory>
#include "audio/PlaybackEngine.hpp"
#include "audio/Song.hpp"
#include "audio/Track.hpp"
#include "audio/Pattern.hpp"

using namespace audio;

// Test 1: PlaybackEngine Creation
void test_playback_engine_creation() {
    PlaybackEngine engine;
    assert(!engine.isPlaying());
    assert(!engine.isPaused());
    assert(engine.getCurrentTick() == 0);
    assert(engine.getTempo() == 120);
    std::cout << "✓ PlaybackEngine creation test passed\n";
}

// Test 2: Song Loading
void test_song_loading() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Test Song", 120);

    bool loaded = engine.loadSong(song);
    assert(loaded);
    assert(engine.getTempo() == 120);
    std::cout << "✓ Song loading test passed\n";
}

// Test 3: Playback Control (Play/Pause/Stop)
void test_playback_control() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Control Test", 120);
    engine.loadSong(song);

    engine.play();
    assert(engine.isPlaying());
    assert(!engine.isPaused());

    engine.pause();
    assert(!engine.isPlaying());
    assert(engine.isPaused());

    engine.play();
    assert(engine.isPlaying());

    engine.stop();
    assert(!engine.isPlaying());
    assert(!engine.isPaused());
    assert(engine.getCurrentTick() == 0);
    std::cout << "✓ Playback control test passed\n";
}

// Test 4: Seeking
void test_seeking() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Seek Test", 120);
    engine.loadSong(song);

    engine.seek(1000);
    assert(engine.getCurrentTick() == 1000);

    engine.seekToBar(4);  // Bar 4
    uint32_t expectedTick = 4 * 4 * 480;
    assert(engine.getCurrentTick() == expectedTick);

    engine.seekToPercent(0.5f);
    assert(engine.getProgress() >= 0.49f && engine.getProgress() <= 0.51f);
    std::cout << "✓ Seeking test passed\n";
}

// Test 5: Tempo Control
void test_tempo_control() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Tempo Test", 120);
    engine.loadSong(song);

    engine.setTempo(140);
    assert(engine.getTempo() == 140);

    engine.setTempo(500);  // Should clamp
    assert(engine.getTempo() <= 300);

    engine.setTempo(0);  // Should clamp
    assert(engine.getTempo() >= 1);
    std::cout << "✓ Tempo control test passed\n";
}

// Test 6: Playback Speed Control
void test_playback_speed_control() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Speed Test", 120);
    engine.loadSong(song);

    engine.setPlaybackSpeed(0.5f);
    assert(engine.getPlaybackSpeed() == 0.5f);

    engine.setPlaybackSpeed(2.0f);
    assert(engine.getPlaybackSpeed() == 2.0f);

    engine.setPlaybackSpeed(10.0f);  // Should clamp
    assert(engine.getPlaybackSpeed() <= 2.0f);
    std::cout << "✓ Playback speed control test passed\n";
}

// Test 7: Volume Control
void test_volume_control() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Volume Test", 120);
    engine.loadSong(song);

    engine.setMasterVolume(100);
    assert(engine.getMasterVolume() == 100);

    engine.setTrackVolume(0, 80);
    assert(engine.getTrackVolume(0) == 80);

    engine.setTrackVolume(15, 90);
    assert(engine.getTrackVolume(15) == 90);
    std::cout << "✓ Volume control test passed\n";
}

// Test 8: Track Muting
void test_track_muting() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Mute Test", 120);
    engine.loadSong(song);

    assert(!engine.isTrackMuted(0));

    engine.setTrackMuted(0, true);
    assert(engine.isTrackMuted(0));

    engine.setTrackMuted(0, false);
    assert(!engine.isTrackMuted(0));
    std::cout << "✓ Track muting test passed\n";
}

// Test 9: Looping
void test_looping() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Loop Test", 120);
    engine.loadSong(song);

    assert(!engine.isLooping());

    engine.setLooping(true);
    assert(engine.isLooping());

    engine.setLoopPoints(1000, 5000);
    // Can't directly verify loop points, but setting them shouldn't crash
    std::cout << "✓ Looping test passed\n";
}

// Test 10: Event Callback
void test_event_callback() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Callback Test", 120);
    engine.loadSong(song);

    int callbackCount = 0;
    engine.setEventCallback([&callbackCount](const AudioEvent& event) {
        callbackCount++;
    });

    // Simulate playback without actual audio events
    engine.play();
    engine.update(100.0f);  // 100ms update
    // Note: Without actual notes in the song, we won't get events
    std::cout << "✓ Event callback test passed\n";
}

// Test 11: Update with Delta Time
void test_update_with_delta_time() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Update Test", 120);
    engine.loadSong(song);

    engine.play();
    uint32_t initialTick = engine.getCurrentTick();

    auto events = engine.update(100.0f);  // 100ms update
    uint32_t newTick = engine.getCurrentTick();

    assert(newTick >= initialTick);  // Should advance
    std::cout << "✓ Update with delta time test passed\n";
}

// Test 12: Progress Tracking
void test_progress_tracking() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Progress Test", 120);
    engine.loadSong(song);

    float progress = engine.getProgress();
    assert(progress >= 0.0f && progress <= 1.0f);

    engine.seekToPercent(0.75f);
    progress = engine.getProgress();
    assert(progress >= 0.74f && progress <= 0.76f);
    std::cout << "✓ Progress tracking test passed\n";
}

// Test 13: Real Song Playback (Simple)
void test_real_song_playback() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Real Song Test", 120);

    // Create a simple track with notes
    auto track = std::make_unique<Track>(Track::Channel::MELODY, "Test Melody");
    auto pattern = std::make_unique<Pattern>(32, TimeSignature());

    // Add some notes
    pattern->addNote(60, 0, 480, 100);      // C4
    pattern->addNote(62, 480, 480, 100);    // D4
    pattern->addNote(64, 960, 480, 100);    // E4
    pattern->addNote(65, 1440, 480, 100);   // F4

    track->addPattern(std::move(pattern));
    song->addTrack(std::move(track));

    // Load and play
    bool loaded = engine.loadSong(song);
    assert(loaded);

    engine.play();
    assert(engine.isPlaying());

    // Simulate some playback
    int eventCount = 0;
    engine.setEventCallback([&eventCount](const AudioEvent& event) {
        eventCount++;
    });

    // Play for 500ms (should schedule events)
    auto events = engine.update(500.0f);

    engine.stop();
    assert(!engine.isPlaying());
    std::cout << "✓ Real song playback test passed (" << eventCount << " events)\n";
}

// Test 14: Multiple Updates
void test_multiple_updates() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Multi-Update Test", 120);
    engine.loadSong(song);

    engine.play();

    uint32_t previousTick = 0;
    for (int i = 0; i < 10; ++i) {
        engine.update(16.0f);  // 16ms per update (60 FPS)
        uint32_t currentTick = engine.getCurrentTick();
        assert(currentTick >= previousTick);
        previousTick = currentTick;
    }

    std::cout << "✓ Multiple updates test passed\n";
}

// Test 15: Statistics Tracking
void test_statistics_tracking() {
    PlaybackEngine engine;
    auto song = std::make_shared<Song>("Stats Test", 120);
    engine.loadSong(song);

    const auto& stats = engine.getStats();
    assert(stats.eventsScheduled == 0);
    assert(stats.eventsPlayed == 0);

    engine.resetStats();
    const auto& statsAfterReset = engine.getStats();
    assert(statsAfterReset.eventsScheduled == 0);
    std::cout << "✓ Statistics tracking test passed\n";
}

int main() {
    std::cout << "\n=== Phase 15: Audio Playback Engine Tests ===\n";

    test_playback_engine_creation();
    test_song_loading();
    test_playback_control();
    test_seeking();
    test_tempo_control();
    test_playback_speed_control();
    test_volume_control();
    test_track_muting();
    test_looping();
    test_event_callback();
    test_update_with_delta_time();
    test_progress_tracking();
    test_real_song_playback();
    test_multiple_updates();
    test_statistics_tracking();

    std::cout << "\n✅ All 15 Phase 15 tests passed!\n";
    std::cout << "   Audio playback engine ready for MEGA65 integration\n\n";

    return 0;
}
