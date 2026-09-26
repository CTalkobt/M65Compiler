#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "Song.hpp"

namespace audio {

// Musical note with pitch and duration
struct Note {
    uint8_t pitch;              // 0-127 (MIDI note number, 60 = middle C)
    uint32_t startTick;         // When note starts (in ticks)
    uint32_t duration;          // How long note lasts (in ticks)
    uint8_t velocity;           // 0-127 (note intensity)

    Note(uint8_t p, uint32_t start, uint32_t dur, uint8_t vel = 100)
        : pitch(p), startTick(start), duration(dur), velocity(vel) {}
};

// Rest (silence)
struct Rest {
    uint32_t startTick;         // When rest starts
    uint32_t duration;          // How long rest lasts

    Rest(uint32_t start, uint32_t dur)
        : startTick(start), duration(dur) {}
};

// Control change (volume, pan, effects, etc.)
struct ControlChange {
    enum class Type : uint8_t {
        VOLUME = 7,
        PAN = 10,
        MODULATION = 1,
        REVERB = 91,
        CHORUS = 93
    };

    Type type;
    uint8_t value;              // 0-127
    uint32_t tick;              // When this change occurs

    ControlChange(Type t, uint8_t val, uint32_t t_)
        : type(t), value(val), tick(t_) {}
};

// Pattern: a sequence of notes, rests, and control changes
class Pattern {
public:
    Pattern(uint32_t lengthBars, const TimeSignature& timeSig);
    ~Pattern() = default;

    // Metadata
    uint32_t getLengthBars() const { return lengthBars_; }
    const TimeSignature& getTimeSignature() const { return timeSig_; }
    uint32_t getLengthTicks() const {
        return lengthBars_ * timeSig_.ticksPerBar();
    }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    // Note management
    Note* addNote(uint8_t pitch, uint32_t startTick, uint32_t duration, uint8_t velocity = 100);
    void removeNote(Note* note);
    const std::vector<Note>& getNotes() const { return notes_; }
    std::vector<Note>& getNotes() { return notes_; }

    // Rest management
    Rest* addRest(uint32_t startTick, uint32_t duration);
    void removeRest(Rest* rest);
    const std::vector<Rest>& getRests() const { return rests_; }

    // Control change management
    ControlChange* addControlChange(ControlChange::Type type, uint8_t value, uint32_t tick);
    void removeControlChange(ControlChange* cc);
    const std::vector<ControlChange>& getControlChanges() const { return controlChanges_; }

    // Pattern operations
    void clear();
    void transpose(int8_t semitones);  // Shift all notes up/down
    void scale(float factor);           // Speed up/slow down
    void reverse();                     // Reverse order of notes

    // Query
    bool isEmpty() const {
        return notes_.empty() && rests_.empty();
    }

    // Find note at specific tick
    const Note* getNoteAt(uint32_t tick) const;
    bool isNotePlaying(uint32_t tick) const;

    // Visualization
    std::string visualize() const;  // ASCII art representation

private:
    uint32_t lengthBars_;
    TimeSignature timeSig_;
    std::string name_;

    std::vector<Note> notes_;
    std::vector<Rest> rests_;
    std::vector<ControlChange> controlChanges_;
};

}  // namespace audio
