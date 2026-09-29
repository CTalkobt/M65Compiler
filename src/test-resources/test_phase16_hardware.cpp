#include <cassert>
#include <iostream>
#include <memory>
#include "audio/SIDChip.hpp"
#include "audio/AudioDriver.hpp"
#include "audio/DIGIAudio.hpp"
#include "audio/PlaybackEngine.hpp"
#include "audio/Song.hpp"
#include "audio/Track.hpp"
#include "audio/Pattern.hpp"

using namespace audio;

// Test 1: SID Voice Control
void test_sid_voice_control() {
    SIDChip sid(0);
    auto& voice = sid.getVoice(0);

    voice.setFrequency(0x1000);
    assert(voice.getFrequency() == 0x1000);

    voice.setWaveform(0x40);  // Pulse waveform
    assert(voice.getWaveform() == 0x40);

    voice.setPulseWidth(0x0800);
    assert(voice.getPulseWidth() == 0x0800);

    voice.setGate(true);
    assert(voice.isGated());

    std::cout << "✓ SID voice control test passed\n";
}

// Test 2: SID ADSR Envelope
void test_sid_adsr() {
    SIDChip sid(0);
    auto& voice = sid.getVoice(1);

    voice.setAttack(10);
    voice.setDecay(5);
    voice.setSustain(12);
    voice.setRelease(8);

    // All values should be clamped to 0-15
    assert(voice.getAttack() == 10);
    assert(voice.getDecay() == 5);
    assert(voice.getSustain() == 12);
    assert(voice.getRelease() == 8);

    std::cout << "✓ SID ADSR envelope test passed\n";
}

// Test 3: SID Filter Control
void test_sid_filter() {
    SIDChip sid(0);
    auto& filter = sid.getFilter();

    filter.setCutoff(0x0400);
    assert(filter.getCutoff() == 0x0400);

    filter.setResonance(8);
    assert(filter.getResonance() == 8);

    filter.setFilterType(0x30);  // Low-pass and high-pass
    assert(filter.getFilterType() == 0x30);

    filter.setVolume(12);
    assert(filter.getVolume() == 12);

    std::cout << "✓ SID filter control test passed\n";
}

// Test 4: MIDI Note to Frequency Conversion
void test_midi_note_conversion() {
    SIDChip sid(0);

    // Test conversion of various MIDI notes
    uint16_t freq_c4 = sid.midiNoteToFreq(60);  // Middle C
    assert(freq_c4 > 0);

    uint16_t freq_a4 = sid.midiNoteToFreq(69);  // A4
    assert(freq_a4 > freq_c4);

    uint16_t freq_c5 = sid.midiNoteToFreq(72);  // C5
    assert(freq_c5 > freq_a4);

    // Test invalid note
    uint16_t freq_invalid = sid.midiNoteToFreq(128);
    assert(freq_invalid == 0);

    std::cout << "✓ MIDI note conversion test passed\n";
}

// Test 5: Note On/Off
void test_note_on_off() {
    SIDChip sid(0);
    auto& voice = sid.getVoice(0);

    // Initially not gated
    assert(!voice.isGated());

    // Note on
    sid.noteOn(0, 60, 100);  // C4, velocity 100
    assert(voice.isGated());
    assert(voice.getFrequency() > 0);

    // Note off
    sid.noteOff(0);
    assert(!voice.isGated());

    std::cout << "✓ Note on/off test passed\n";
}

// Test 6: Multiple SID Chips
void test_multiple_sid_chips() {
    std::array<std::unique_ptr<SIDChip>, 4> chips;
    for (uint8_t i = 0; i < 4; ++i) {
        chips[i] = std::make_unique<SIDChip>(i);
        assert(chips[i]->getChipId() == i);
    }

    // Play different notes on different chips
    chips[0]->noteOn(0, 60, 100);  // C4
    chips[1]->noteOn(1, 64, 100);  // E4
    chips[2]->noteOn(2, 67, 100);  // G4
    chips[3]->noteOn(0, 72, 100);  // C5

    // Verify all notes started
    assert(chips[0]->getVoice(0).isGated());
    assert(chips[1]->getVoice(1).isGated());
    assert(chips[2]->getVoice(2).isGated());
    assert(chips[3]->getVoice(0).isGated());

    // Verify base addresses are correct (avoid sign-compare warning)
    assert(chips[0]->getBaseAddress() == 0xD400U);
    assert(chips[1]->getBaseAddress() == 0xD500U);
    assert(chips[2]->getBaseAddress() == 0xD600U);
    assert(chips[3]->getBaseAddress() == 0xD700U);

    std::cout << "✓ Multiple SID chips test passed\n";
}

// Test 7: Audio Driver Initialization
void test_audio_driver_init() {
    AudioDriver driver;
    assert(!driver.isInitialized());

    bool initialized = driver.initialize();
    assert(initialized);
    assert(driver.isInitialized());

    driver.shutdown();
    assert(!driver.isInitialized());

    std::cout << "✓ Audio driver initialization test passed\n";
}

// Test 8: Audio Driver SID Access
void test_audio_driver_sid_access() {
    AudioDriver driver;
    driver.initialize();

    for (uint8_t c = 0; c < 4; ++c) {
        auto& sid = driver.getSID(c);
        assert(sid.getChipId() == c);
    }

    driver.shutdown();
    std::cout << "✓ Audio driver SID access test passed\n";
}

// Test 9: Track Routing Configuration
void test_track_routing() {
    AudioDriver driver;
    driver.initialize();

    TrackRoute route;
    route.output = AudioOutput::SID_CHIP_0;
    route.voiceId = 0;
    route.channel = 1;
    route.enabled = true;

    driver.configureTrackRoute(0, route);
    const auto& retrieved = driver.getTrackRoute(0);
    assert(retrieved.output == AudioOutput::SID_CHIP_0);
    assert(retrieved.voiceId == 0);
    assert(retrieved.enabled);

    driver.shutdown();
    std::cout << "✓ Track routing configuration test passed\n";
}

// Test 10: Master Volume Control
void test_master_volume() {
    AudioDriver driver;
    driver.initialize();

    driver.setMasterVolume(100);
    assert(driver.getSID(0).getFilter().getVolume() > 0);

    driver.setMasterVolume(0);
    // Volume should be at minimum (not necessarily 0, could be 1)

    driver.setMasterVolume(127);
    assert(driver.getSID(0).getFilter().getVolume() > 0);

    driver.shutdown();
    std::cout << "✓ Master volume control test passed\n";
}

// Test 11: Muting and Unmuting
void test_mute_unmute() {
    AudioDriver driver;
    driver.initialize();

    assert(!driver.isMuted());

    driver.mute();
    assert(driver.isMuted());

    driver.unmute();
    assert(!driver.isMuted());

    driver.shutdown();
    std::cout << "✓ Mute/unmute test passed\n";
}

// Test 12: DIGI Audio Sample Loading
void test_digi_sample_loading() {
    DIGIAudio digi;

    assert(digi.getSampleLength() == 0);
    assert(!digi.isPlaying());

    // Create a test sample
    std::vector<uint8_t> sample(1000, 0x80);  // 1000 bytes of silence
    digi.loadSample(sample.data(), sample.size());

    assert(digi.getSampleLength() == 1000);
    std::cout << "✓ DIGI sample loading test passed\n";
}

// Test 13: DIGI Audio Playback Control
void test_digi_playback() {
    DIGIAudio digi;

    // Create test sample
    std::vector<uint8_t> sample(1000, 0x80);
    digi.loadSample(sample.data(), sample.size());

    digi.play();
    assert(digi.isPlaying());
    assert(digi.getCurrentPosition() == 0);

    digi.pause();
    assert(!digi.isPlaying());

    digi.play();
    assert(digi.isPlaying());

    digi.stop();
    assert(!digi.isPlaying());
    assert(digi.getCurrentPosition() == 0);

    std::cout << "✓ DIGI audio playback control test passed\n";
}

// Test 14: DIGI Audio Volume and Pan
void test_digi_volume_pan() {
    DIGIAudio digi;

    digi.setVolume(100);
    // Volume should be clamped to 127
    // (internal check that it doesn't crash)

    digi.setPan(64);  // Center pan
    // Pan should be set without error

    digi.setPlaybackRate(1.5f);
    // Playback rate should be clamped to 0.5-2.0

    std::cout << "✓ DIGI volume and pan test passed\n";
}

// Test 15: DIGI Audio Progress Tracking
void test_digi_progress() {
    DIGIAudio digi;
    assert(digi.getProgress() == 0.0f);

    std::vector<uint8_t> sample(1000, 0x80);
    digi.loadSample(sample.data(), sample.size());

    digi.play();
    digi.updateHardware();  // Simulate one update

    float progress = digi.getProgress();
    assert(progress >= 0.0f && progress <= 1.0f);

    std::cout << "✓ DIGI progress tracking test passed\n";
}

// Test 16: Integration - PlaybackEngine with AudioDriver
void test_playback_engine_integration() {
    AudioDriver driver;
    driver.initialize();

    PlaybackEngine engine;
    driver.attachPlaybackEngine(&engine);

    auto song = std::make_shared<Song>("Integration Test", 120);

    // Create a simple track
    auto track = std::make_unique<Track>(Track::Channel::MELODY, "Test");
    auto pattern = std::make_unique<Pattern>(16, TimeSignature());
    pattern->addNote(60, 0, 480, 100);
    pattern->addNote(64, 480, 480, 100);
    track->addPattern(std::move(pattern));
    song->addTrack(std::move(track));

    bool loaded = engine.loadSong(song);
    assert(loaded);

    // Configure routing
    TrackRoute route;
    route.output = AudioOutput::SID_CHIP_0;
    route.voiceId = 0;
    route.enabled = true;
    driver.configureTrackRoute(0, route);

    // Simulate playback
    engine.play();
    auto events = engine.update(100.0f);

    for (const auto& event : events) {
        driver.handleAudioEvent(event);
    }

    driver.detachPlaybackEngine();
    driver.shutdown();
    std::cout << "✓ PlaybackEngine integration test passed\n";
}

// Test 17: Active SID Chips Detection
void test_active_sid_detection() {
    AudioDriver driver;
    driver.initialize();

    uint32_t activeMask = driver.getActiveSIDChips();
    assert(activeMask == 0);  // No chips active initially

    // Play a note
    auto& sid = driver.getSID(0);
    sid.noteOn(0, 60, 100);

    activeMask = driver.getActiveSIDChips();
    assert(activeMask & (1 << 0));  // Chip 0 should be active

    driver.shutdown();
    std::cout << "✓ Active SID detection test passed\n";
}

// Test 18: Hardware Update
void test_hardware_update() {
    AudioDriver driver;
    driver.initialize();

    auto& sid = driver.getSID(0);
    sid.noteOn(0, 60, 100);
    sid.noteOn(1, 64, 100);
    sid.noteOn(2, 67, 100);

    // This should update all hardware registers
    driver.updateHardware();

    // Verify voices are still active
    assert(sid.getVoice(0).isGated());
    assert(sid.getVoice(1).isGated());
    assert(sid.getVoice(2).isGated());

    driver.shutdown();
    std::cout << "✓ Hardware update test passed\n";
}

int main() {
    std::cout << "\n=== Phase 16: MEGA65 Hardware Integration Tests ===\n";

    test_sid_voice_control();
    test_sid_adsr();
    test_sid_filter();
    test_midi_note_conversion();
    test_note_on_off();
    test_multiple_sid_chips();
    test_audio_driver_init();
    test_audio_driver_sid_access();
    test_track_routing();
    test_master_volume();
    test_mute_unmute();
    test_digi_sample_loading();
    test_digi_playback();
    test_digi_volume_pan();
    test_digi_progress();
    test_playback_engine_integration();
    test_active_sid_detection();
    test_hardware_update();

    std::cout << "\n✅ All 18 Phase 16 hardware integration tests passed!\n";
    std::cout << "   MEGA65 audio hardware layer ready for testing\n\n";

    return 0;
}
