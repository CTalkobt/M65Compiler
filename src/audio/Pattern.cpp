#include "audio/Pattern.hpp"
#include <algorithm>
#include <sstream>

namespace audio {

Pattern::Pattern(uint32_t lengthBars, const TimeSignature& timeSig)
    : lengthBars_(lengthBars), timeSig_(timeSig), name_("Pattern") {
}

Note* Pattern::addNote(uint8_t pitch, uint32_t startTick, uint32_t duration, uint8_t velocity) {
    if (pitch > 127) pitch = 127;
    if (velocity > 127) velocity = 127;
    if (duration == 0) duration = 1;

    notes_.emplace_back(pitch, startTick, duration, velocity);
    return &notes_.back();
}

void Pattern::removeNote(Note* note) {
    if (!note) return;
    for (auto it = notes_.begin(); it != notes_.end(); ++it) {
        if (&(*it) == note) {
            notes_.erase(it);
            break;
        }
    }
}

Rest* Pattern::addRest(uint32_t startTick, uint32_t duration) {
    if (duration == 0) duration = 1;
    rests_.emplace_back(startTick, duration);
    return &rests_.back();
}

void Pattern::removeRest(Rest* rest) {
    if (!rest) return;
    for (auto it = rests_.begin(); it != rests_.end(); ++it) {
        if (&(*it) == rest) {
            rests_.erase(it);
            break;
        }
    }
}

ControlChange* Pattern::addControlChange(ControlChange::Type type, uint8_t value, uint32_t tick) {
    if (value > 127) value = 127;
    controlChanges_.emplace_back(type, value, tick);
    return &controlChanges_.back();
}

void Pattern::removeControlChange(ControlChange* cc) {
    if (!cc) return;
    for (auto it = controlChanges_.begin(); it != controlChanges_.end(); ++it) {
        if (&(*it) == cc) {
            controlChanges_.erase(it);
            break;
        }
    }
}

void Pattern::clear() {
    notes_.clear();
    rests_.clear();
    controlChanges_.clear();
}

void Pattern::transpose(int8_t semitones) {
    for (auto& note : notes_) {
        int newPitch = static_cast<int>(note.pitch) + semitones;
        if (newPitch < 0) newPitch = 0;
        if (newPitch > 127) newPitch = 127;
        note.pitch = static_cast<uint8_t>(newPitch);
    }
}

void Pattern::scale(float factor) {
    if (factor <= 0.0f) return;

    for (auto& note : notes_) {
        note.duration = static_cast<uint32_t>(note.duration * factor);
        if (note.duration < 1) note.duration = 1;
    }

    for (auto& rest : rests_) {
        rest.duration = static_cast<uint32_t>(rest.duration * factor);
        if (rest.duration < 1) rest.duration = 1;
    }

    for (auto& cc : controlChanges_) {
        cc.tick = static_cast<uint32_t>(cc.tick * factor);
    }
}

void Pattern::reverse() {
    // Reverse order of notes
    std::reverse(notes_.begin(), notes_.end());
    std::reverse(rests_.begin(), rests_.end());
    std::reverse(controlChanges_.begin(), controlChanges_.end());

    // Recalculate positions based on pattern length
    uint32_t patternLen = getLengthTicks();
    for (auto& note : notes_) {
        note.startTick = patternLen - note.startTick - note.duration;
    }
    for (auto& rest : rests_) {
        rest.startTick = patternLen - rest.startTick - rest.duration;
    }
    for (auto& cc : controlChanges_) {
        cc.tick = patternLen - cc.tick;
    }
}

const Note* Pattern::getNoteAt(uint32_t tick) const {
    for (const auto& note : notes_) {
        if (tick >= note.startTick && tick < note.startTick + note.duration) {
            return &note;
        }
    }
    return nullptr;
}

bool Pattern::isNotePlaying(uint32_t tick) const {
    return getNoteAt(tick) != nullptr;
}

std::string Pattern::visualize() const {
    std::ostringstream oss;
    oss << "Pattern: " << name_ << " (" << lengthBars_ << " bars)\n";
    oss << "Notes: " << notes_.size() << "\n";

    if (!notes_.empty()) {
        oss << "  Pitches: ";
        for (size_t i = 0; i < std::min(size_t(10), notes_.size()); ++i) {
            oss << static_cast<int>(notes_[i].pitch) << " ";
        }
        if (notes_.size() > 10) oss << "...";
        oss << "\n";
    }

    oss << "Rests: " << rests_.size() << "\n";
    oss << "Control Changes: " << controlChanges_.size() << "\n";

    return oss.str();
}

}  // namespace audio
