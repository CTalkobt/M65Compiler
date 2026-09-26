#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include "Song.hpp"
#include "Sequencer.hpp"

namespace audio {

// Audio event types that can be scheduled and played
enum class AudioEventType : uint8_t {
    NOTE_ON = 0,      // Start playing a note
    NOTE_OFF = 1,     // Stop playing a note
    CONTROL_CHANGE = 2, // Change a parameter (volume, pan, etc.)
    TEMPO_CHANGE = 3, // Change playback tempo
    TRACK_MUTE = 4,   // Mute/unmute a track
    EFFECT_PARAM = 5  // Adjust effect parameter
};

// Scheduled audio event
struct AudioEvent {
    uint32_t tick;           // When to play (in ticks)
    AudioEventType type;     // What kind of event
    uint8_t trackId;         // Which track (0-15)
    uint8_t param1;          // Parameter 1 (note pitch, control number, etc.)
    uint8_t param2;          // Parameter 2 (velocity, value, etc.)
    uint16_t param3;         // Parameter 3 (extended info)

    AudioEvent(uint32_t t, AudioEventType ty, uint8_t track, uint8_t p1, uint8_t p2, uint16_t p3 = 0)
        : tick(t), type(ty), trackId(track), param1(p1), param2(p2), param3(p3) {}
};

// Playback statistics
struct PlaybackStats {
    uint32_t eventsScheduled = 0;
    uint32_t eventsPlayed = 0;
    uint32_t notesPlaying = 0;
    float cpuLoad = 0.0f;  // Estimated CPU usage (0.0-1.0)
};

// Real-time audio playback engine
class PlaybackEngine {
public:
    PlaybackEngine();
    ~PlaybackEngine() = default;

    // Load and prepare a song for playback
    bool loadSong(const std::shared_ptr<Song>& song);

    // Playback control
    void play();
    void pause();
    void stop();
    void reset();

    // Playback state queries
    bool isPlaying() const { return isPlaying_; }
    bool isPaused() const { return isPaused_; }
    uint32_t getCurrentTick() const { return currentTick_; }
    uint32_t getTotalTicks() const { return totalTicks_; }
    float getProgress() const;  // 0.0 - 1.0

    // Seek to a specific position
    void seek(uint32_t tick);
    void seekToBar(uint32_t bar);
    void seekToPercent(float percent);

    // Tempo control
    void setTempo(uint32_t beatsPerMinute);
    uint32_t getTempo() const { return tempoBeatsPerMinute_; }
    void setPlaybackSpeed(float speed);  // 0.5x - 2.0x
    float getPlaybackSpeed() const { return playbackSpeed_; }

    // Volume control (master and per-track)
    void setMasterVolume(uint8_t volume);  // 0-127
    uint8_t getMasterVolume() const { return masterVolume_; }

    void setTrackVolume(uint8_t trackId, uint8_t volume);
    uint8_t getTrackVolume(uint8_t trackId) const;

    // Track muting
    void setTrackMuted(uint8_t trackId, bool muted);
    bool isTrackMuted(uint8_t trackId) const;

    // Looping control
    void setLooping(bool enabled);
    bool isLooping() const { return looping_; }

    void setLoopPoints(uint32_t startTick, uint32_t endTick);

    // Audio event callback (called when audio events occur)
    using EventCallback = std::function<void(const AudioEvent&)>;
    void setEventCallback(EventCallback callback);

    // Schedule events for a given time range (called periodically)
    void scheduleEvents(uint32_t fromTick, uint32_t toTick);

    // Process playback for a time delta (in milliseconds)
    // Returns the next batch of audio events to process
    std::vector<AudioEvent> update(float deltaTimeMs);

    // Get playback statistics
    const PlaybackStats& getStats() const { return stats_; }

    // Performance monitoring
    void resetStats();

private:
    std::shared_ptr<Song> song_;
    std::unique_ptr<Sequencer> sequencer_;

    // Playback state
    bool isPlaying_ = false;
    bool isPaused_ = false;
    uint32_t currentTick_ = 0;
    uint32_t totalTicks_ = 0;

    // Tempo and timing
    uint32_t tempoBeatsPerMinute_ = 120;
    float playbackSpeed_ = 1.0f;
    float msPerTick_ = 0.5f;  // Milliseconds per tick (depends on tempo)

    // Volume control
    uint8_t masterVolume_ = 127;
    std::vector<uint8_t> trackVolumes_;  // 0-127 per track
    std::vector<bool> trackMuted_;

    // Looping
    bool looping_ = false;
    uint32_t loopStartTick_ = 0;
    uint32_t loopEndTick_ = 0;

    // Active notes tracking (for note-off events)
    struct ActiveNote {
        uint8_t trackId;
        uint8_t pitch;
        uint32_t startTick;
        uint32_t duration;

        ActiveNote(uint8_t t, uint8_t p, uint32_t s, uint32_t d)
            : trackId(t), pitch(p), startTick(s), duration(d) {}
    };
    std::vector<ActiveNote> activeNotes_;

    // Event scheduling
    std::vector<AudioEvent> pendingEvents_;
    EventCallback eventCallback_;

    // Statistics
    PlaybackStats stats_;

    // Helper methods
    void updateMsPerTick();
    void generateNoteEvents(uint32_t fromTick, uint32_t toTick);
    void generateControlEvents(uint32_t fromTick, uint32_t toTick);
    std::vector<AudioEvent> getPendingEvents();
};

}  // namespace audio
