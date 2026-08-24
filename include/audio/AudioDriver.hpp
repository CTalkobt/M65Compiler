#pragma once

#include <memory>
#include <array>
#include <cstdint>
#include "PlaybackEngine.hpp"
#include "SIDChip.hpp"

namespace audio {

// Audio output destination
enum class AudioOutput : uint8_t {
    SID_CHIP_0 = 0,      // First SID
    SID_CHIP_1 = 1,      // Second SID
    SID_CHIP_2 = 2,      // Third SID
    SID_CHIP_3 = 3,      // Fourth SID (MEGA65 has 4 SID chips)
    DIGI_AUDIO = 4       // DIGI audio channel
};

// Track-to-hardware routing
struct TrackRoute {
    AudioOutput output;      // Which SID chip or DIGI channel
    uint8_t voiceId;        // Voice ID (0-2 for SID, 0 for DIGI)
    uint8_t channel;        // MIDI channel (1-16) for routing
    bool enabled = true;    // Whether this route is active
};

// MEGA65 Audio Driver - bridges playback engine to hardware
class AudioDriver {
public:
    AudioDriver();
    ~AudioDriver() = default;

    // Initialize driver and hardware
    bool initialize();

    // Shutdown driver
    void shutdown();

    // Hardware access
    SIDChip& getSID(uint8_t chipId);
    const SIDChip& getSID(uint8_t chipId) const;

    // Configuration
    void configureTrackRoute(uint8_t trackId, const TrackRoute& route);
    const TrackRoute& getTrackRoute(uint8_t trackId) const;

    // Playback integration
    void attachPlaybackEngine(PlaybackEngine* engine);
    void detachPlaybackEngine();

    // Audio event handling
    void handleAudioEvent(const AudioEvent& event);

    // Hardware updates
    void updateHardware();

    // Status
    bool isInitialized() const { return initialized_; }
    uint32_t getActiveSIDChips() const;

    // Master control
    void setMasterVolume(uint8_t volume);  // 0-127
    void mute();
    void unmute();
    bool isMuted() const { return muted_; }

private:
    // Hardware state
    std::array<std::unique_ptr<SIDChip>, 4> sidChips_;
    std::array<TrackRoute, 16> trackRoutes_;  // Route for each track
    bool initialized_ = false;
    bool muted_ = false;
    uint8_t masterVolume_ = 127;

    // Playback engine reference
    PlaybackEngine* playbackEngine_ = nullptr;

    // Voice allocation
    struct VoiceAllocation {
        uint8_t sidChipId;
        uint8_t voiceId;
        bool allocated = false;
    };
    std::array<VoiceAllocation, 64> voicePool_;  // 16 SID voices pool

    // Helper methods
    uint8_t allocateVoice(uint8_t sidChipId);
    void deallocateVoice(uint8_t voiceAllocId);

    void handleNoteOn(const AudioEvent& event);
    void handleNoteOff(const AudioEvent& event);
    void handleControlChange(const AudioEvent& event);
    void handleTempoChange(const AudioEvent& event);

    // Hardware initialization
    void initializeSIDChips();
    void initializeRouting();

    // MEGA65 I/O
    void writeToMEGA65Memory(uint32_t address, uint8_t value);
    uint8_t readFromMEGA65Memory(uint32_t address) const;
};

}  // namespace audio
