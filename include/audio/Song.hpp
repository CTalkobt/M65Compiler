#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace audio {

// Musical time signature
struct TimeSignature {
    uint8_t numerator;      // e.g., 4 in 4/4
    uint8_t denominator;    // e.g., 4 in 4/4
    uint32_t ticksPerBeat;  // e.g., 480 for PPQN

    TimeSignature(uint8_t num = 4, uint8_t denom = 4, uint32_t ticks = 480)
        : numerator(num), denominator(denom), ticksPerBeat(ticks) {}

    // Calculate ticks per bar
    uint32_t ticksPerBar() const {
        return (ticksPerBeat * 4 * numerator) / denominator;
    }

    // Calculate number of beats in a bar
    uint32_t beatsPerBar() const {
        return numerator;
    }
};

// Forward declarations
class Track;
class Sequencer;

// Song: top-level container for musical composition
class Song {
public:
    Song(const std::string& name = "Untitled", uint32_t tempo = 120);
    ~Song();  // Needs proper definition for unique_ptr<Track> cleanup

    // Metadata
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    uint32_t getTempo() const { return tempo_; }
    void setTempo(uint32_t tempo) {
        if (tempo > 0 && tempo < 1000) tempo_ = tempo;
    }

    const TimeSignature& getTimeSignature() const { return timeSig_; }
    void setTimeSignature(const TimeSignature& ts) { timeSig_ = ts; }

    // Track management
    Track* addTrack(std::unique_ptr<Track> track);
    void removeTrack(Track* track);
    std::vector<Track*> getTracks();
    const std::vector<Track*> getTracks() const;
    size_t getTrackCount() const { return tracks_.size(); }

    // Playback control
    void play();
    void pause();
    void stop();
    void reset();

    bool isPlaying() const { return isPlaying_; }
    bool isPaused() const { return isPaused_; }

    // Position tracking (in ticks)
    uint32_t getCurrentPosition() const { return currentPosition_; }
    void setCurrentPosition(uint32_t position) { currentPosition_ = position; }

    // Duration calculation (in ticks)
    uint32_t getDurationTicks() const { return durationTicks_; }
    void setDurationTicks(uint32_t ticks) { durationTicks_ = ticks; }

    // Duration in bars
    uint32_t getDurationBars() const {
        return (durationTicks_ + timeSig_.ticksPerBar() - 1) / timeSig_.ticksPerBar();
    }

    // Serialization
    bool saveTo(const std::string& filename);
    bool loadFrom(const std::string& filename);

private:
    friend class Sequencer;

    std::string name_;
    uint32_t tempo_;           // BPM (beats per minute)
    TimeSignature timeSig_;

    std::vector<std::unique_ptr<Track>> tracks_;
    std::vector<Track*> trackPtrs_;  // Non-owning pointers for easy access

    bool isPlaying_ = false;
    bool isPaused_ = false;
    uint32_t currentPosition_ = 0;  // Current playback position in ticks
    uint32_t durationTicks_ = 0;    // Total length of song in ticks
};

}  // namespace audio
