#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include "Song.hpp"
#include "Track.hpp"
#include "Pattern.hpp"

namespace audio {

// Callback for audio events (note on/off, control changes, etc.)
using AudioEventCallback = std::function<void(
    uint32_t tick,
    Track* track,
    const Note* note,
    bool isNoteOn
)>;

using ControlChangeCallback = std::function<void(
    uint32_t tick,
    Track* track,
    const ControlChange& cc
)>;

// Sequencer: manages playback of songs
class Sequencer {
public:
    Sequencer();
    ~Sequencer();  // Needs proper definition for unique_ptr<Song> cleanup

    // Song management
    void loadSong(std::unique_ptr<Song> song);
    Song* getCurrentSong() { return currentSong_.get(); }
    const Song* getCurrentSong() const { return currentSong_.get(); }
    void unloadSong();

    // Playback control
    void play();
    void pause();
    void stop();
    void reset();

    bool isPlaying() const { return isPlaying_; }
    bool isPaused() const { return isPaused_; }

    // Position control (in ticks)
    uint32_t getCurrentPosition() const { return currentPosition_; }
    void setCurrentPosition(uint32_t position);

    // Speed control
    float getPlaybackSpeed() const { return playbackSpeed_; }
    void setPlaybackSpeed(float speed) {
        if (speed > 0.1f && speed <= 2.0f) playbackSpeed_ = speed;
    }

    // Advance playback (called by timer/main loop)
    void advanceTick(uint32_t deltaTicks = 1);
    void advanceBar();  // Advance to next bar

    // Get all active notes at current position
    struct ActiveNotes {
        struct NoteInfo {
            Track* track;
            const Note* note;
        };
        std::vector<NoteInfo> notes;
    };
    ActiveNotes getActiveNotes() const;

    // Event callbacks
    void setNoteEventCallback(AudioEventCallback callback) {
        noteEventCallback_ = callback;
    }
    void setControlChangeCallback(ControlChangeCallback callback) {
        ccCallback_ = callback;
    }

    // Quantization (snap to grid)
    enum class QuantizeMode {
        NONE,
        SIXTEENTH,  // 1/16 note
        EIGHTH,     // 1/8 note
        QUARTER,    // 1/4 note
        HALF        // 1/2 note
    };

    void setQuantizeMode(QuantizeMode mode) { quantizeMode_ = mode; }
    QuantizeMode getQuantizeMode() const { return quantizeMode_; }
    uint32_t quantizeTick(uint32_t tick) const;

    // Looping
    void setLoopEnabled(bool enabled) { loopEnabled_ = enabled; }
    bool isLoopEnabled() const { return loopEnabled_; }

    void setLoopStart(uint32_t tick) { loopStart_ = tick; }
    void setLoopEnd(uint32_t tick) { loopEnd_ = tick; }
    uint32_t getLoopStart() const { return loopStart_; }
    uint32_t getLoopEnd() const { return loopEnd_; }

    // Metronome
    void setMetronomeEnabled(bool enabled) { metronomeEnabled_ = enabled; }
    bool isMetronomeEnabled() const { return metronomeEnabled_; }
    void setMetronomeTempo(uint32_t bpm) { metronomeTempo_ = bpm; }

    // Recording (for future use)
    void startRecording();
    void stopRecording();
    bool isRecording() const { return isRecording_; }

    // Statistics
    uint32_t getTotalNotes() const;
    uint32_t getTotalEvents() const;
    std::string getStatistics() const;

private:
    // Internal state
    std::unique_ptr<Song> currentSong_;
    bool isPlaying_ = false;
    bool isPaused_ = false;
    bool isRecording_ = false;
    uint32_t currentPosition_ = 0;
    float playbackSpeed_ = 1.0f;

    // Looping
    bool loopEnabled_ = false;
    uint32_t loopStart_ = 0;
    uint32_t loopEnd_ = 0;

    // Quantization
    QuantizeMode quantizeMode_ = QuantizeMode::NONE;

    // Metronome
    bool metronomeEnabled_ = false;
    uint32_t metronomeTempo_ = 120;
    uint32_t metronomeCounter_ = 0;

    // Callbacks
    AudioEventCallback noteEventCallback_;
    ControlChangeCallback ccCallback_;

    // Track note-on states (for detecting note-offs)
    std::vector<std::vector<bool>> noteStates_;  // [track][note] = is_on

    // Private helpers
    void updateNoteStates();
    void processNotesAtTick(uint32_t tick);
    void processControlChangesAtTick(uint32_t tick);
    void triggerMetronome();
};

}  // namespace audio
