#pragma once

#include <vector>
#include <cstdint>
#include "Chord.hpp"

namespace procedural {

// A single chord in a progression
struct ChordEntry {
    Chord chord;
    uint32_t durationTicks;  // How long this chord lasts
    uint8_t voicing;         // Voicing style (0=root position, 1=first inversion, etc.)

    ChordEntry(const Chord& c, uint32_t dur, uint8_t voic = 0)
        : chord(c), durationTicks(dur), voicing(voic) {}
};

// Chord progression: sequence of chords over time
class ChordProgression {
public:
    ChordProgression(Key key = Key::C_MAJOR);
    ~ChordProgression() = default;

    // Properties
    Key getKey() const { return key_; }
    void setKey(Key key) { key_ = key; }

    // Add/remove chords
    void addChord(const Chord& chord, uint32_t durationTicks, uint8_t voicing = 0);
    void insertChord(size_t position, const Chord& chord, uint32_t durationTicks, uint8_t voicing = 0);
    void removeChord(size_t position);
    void clear();

    // Query
    const std::vector<ChordEntry>& getChords() const { return chords_; }
    const ChordEntry* getChordAt(uint32_t tick) const;
    size_t getChordCount() const { return chords_.size(); }
    uint32_t getTotalDuration() const;

    // Navigation
    int getChordIndex(uint32_t tick) const;

private:
    Key key_;
    std::vector<ChordEntry> chords_;
};

}  // namespace procedural
