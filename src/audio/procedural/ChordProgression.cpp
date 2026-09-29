#include "procedural/ChordProgression.hpp"
#include <algorithm>

namespace procedural {

ChordProgression::ChordProgression(Key key)
    : key_(key) {}

void ChordProgression::addChord(const Chord& chord, uint32_t durationTicks, uint8_t voicing) {
    chords_.emplace_back(chord, durationTicks, voicing);
}

void ChordProgression::insertChord(size_t position, const Chord& chord, uint32_t durationTicks, uint8_t voicing) {
    if (position <= chords_.size()) {
        chords_.insert(chords_.begin() + position, ChordEntry(chord, durationTicks, voicing));
    }
}

void ChordProgression::removeChord(size_t position) {
    if (position < chords_.size()) {
        chords_.erase(chords_.begin() + position);
    }
}

void ChordProgression::clear() {
    chords_.clear();
}

const ChordEntry* ChordProgression::getChordAt(uint32_t tick) const {
    uint32_t currentTick = 0;
    for (const auto& entry : chords_) {
        if (tick >= currentTick && tick < currentTick + entry.durationTicks) {
            return &entry;
        }
        currentTick += entry.durationTicks;
    }
    return nullptr;
}

uint32_t ChordProgression::getTotalDuration() const {
    uint32_t total = 0;
    for (const auto& entry : chords_) {
        total += entry.durationTicks;
    }
    return total;
}

int ChordProgression::getChordIndex(uint32_t tick) const {
    uint32_t currentTick = 0;
    for (size_t i = 0; i < chords_.size(); ++i) {
        if (tick >= currentTick && tick < currentTick + chords_[i].durationTicks) {
            return static_cast<int>(i);
        }
        currentTick += chords_[i].durationTicks;
    }
    return -1;
}

}  // namespace procedural
