#include "procedural/Scale.hpp"
#include <algorithm>

namespace procedural {

Scale::Scale(uint8_t root, const std::vector<uint8_t>& intervals, const std::string& name)
    : rootNote_(root), name_(name) {

    // Store scale notes (root + intervals)
    for (uint8_t interval : intervals) {
        notes_.push_back((root + interval) % 12);
    }
    std::sort(notes_.begin(), notes_.end());
    notes_.erase(std::unique(notes_.begin(), notes_.end()), notes_.end());
}

Scale Scale::Major(Note root) {
    // Major scale: W-W-H-W-W-W-H (intervals: 0,2,4,5,7,9,11)
    std::vector<uint8_t> intervals = { 0, 2, 4, 5, 7, 9, 11 };
    return Scale(static_cast<uint8_t>(root), intervals, "Major");
}

Scale Scale::NaturalMinor(Note root) {
    // Natural minor: W-H-W-W-H-W-W (intervals: 0,2,3,5,7,8,10)
    std::vector<uint8_t> intervals = { 0, 2, 3, 5, 7, 8, 10 };
    return Scale(static_cast<uint8_t>(root), intervals, "Natural Minor");
}

Scale Scale::HarmonicMinor(Note root) {
    // Harmonic minor: natural minor with raised 7th (0,2,3,5,7,8,11)
    std::vector<uint8_t> intervals = { 0, 2, 3, 5, 7, 8, 11 };
    return Scale(static_cast<uint8_t>(root), intervals, "Harmonic Minor");
}

Scale Scale::MelodicMinor(Note root) {
    // Melodic minor: natural minor with raised 6th and 7th ascending (0,2,3,5,7,9,11)
    std::vector<uint8_t> intervals = { 0, 2, 3, 5, 7, 9, 11 };
    return Scale(static_cast<uint8_t>(root), intervals, "Melodic Minor");
}

Scale Scale::Pentatonic(Note root, bool major) {
    std::vector<uint8_t> intervals;
    if (major) {
        // Major pentatonic: 1-2-3-5-6 (0,2,4,7,9)
        intervals = { 0, 2, 4, 7, 9 };
    } else {
        // Minor pentatonic: 1-b3-4-5-b7 (0,3,5,7,10)
        intervals = { 0, 3, 5, 7, 10 };
    }
    return Scale(static_cast<uint8_t>(root), intervals, major ? "Major Pentatonic" : "Minor Pentatonic");
}

Scale Scale::Blues(Note root) {
    // Blues scale: minor pentatonic + b5 (0,3,5,6,7,10)
    std::vector<uint8_t> intervals = { 0, 3, 5, 6, 7, 10 };
    return Scale(static_cast<uint8_t>(root), intervals, "Blues");
}

Scale Scale::Dorian(Note root) {
    // Dorian: 1-2-b3-4-5-6-b7 (0,2,3,5,7,9,10)
    std::vector<uint8_t> intervals = { 0, 2, 3, 5, 7, 9, 10 };
    return Scale(static_cast<uint8_t>(root), intervals, "Dorian");
}

Scale Scale::Phrygian(Note root) {
    // Phrygian: 1-b2-b3-4-5-b6-b7 (0,1,3,5,7,8,10)
    std::vector<uint8_t> intervals = { 0, 1, 3, 5, 7, 8, 10 };
    return Scale(static_cast<uint8_t>(root), intervals, "Phrygian");
}

Scale Scale::Lydian(Note root) {
    // Lydian: 1-2-3-#4-5-6-7 (0,2,4,6,7,9,11)
    std::vector<uint8_t> intervals = { 0, 2, 4, 6, 7, 9, 11 };
    return Scale(static_cast<uint8_t>(root), intervals, "Lydian");
}

Scale Scale::Mixolydian(Note root) {
    // Mixolydian: 1-2-3-4-5-6-b7 (0,2,4,5,7,9,10)
    std::vector<uint8_t> intervals = { 0, 2, 4, 5, 7, 9, 10 };
    return Scale(static_cast<uint8_t>(root), intervals, "Mixolydian");
}

bool Scale::contains(uint8_t pitch) const {
    uint8_t pitchInOctave = pitch % 12;
    return std::find(notes_.begin(), notes_.end(), pitchInOctave) != notes_.end();
}

uint8_t Scale::getNextNote(uint8_t pitch) const {
    if (notes_.empty()) return pitch;

    uint8_t pitchInOctave = pitch % 12;
    uint8_t octave = pitch / 12;

    // Find current note in scale
    auto it = std::find(notes_.begin(), notes_.end(), pitchInOctave);
    if (it == notes_.end()) {
        // Pitch not in scale, find closest higher note
        it = std::find_if(notes_.begin(), notes_.end(),
                         [pitchInOctave](uint8_t n) { return n > pitchInOctave; });
        if (it != notes_.end()) {
            return *it + octave * 12;
        } else {
            // Wrap to next octave
            return notes_[0] + (octave + 1) * 12;
        }
    }

    // Move to next scale degree
    ++it;
    if (it != notes_.end()) {
        return *it + octave * 12;
    } else {
        // Wrap to next octave
        return notes_[0] + (octave + 1) * 12;
    }
}

uint8_t Scale::getPreviousNote(uint8_t pitch) const {
    if (notes_.empty()) return pitch;

    uint8_t pitchInOctave = pitch % 12;
    uint8_t octave = pitch / 12;

    // Find current note in scale
    auto it = std::find(notes_.begin(), notes_.end(), pitchInOctave);
    if (it == notes_.end()) {
        // Pitch not in scale, find closest lower note
        auto rit = std::find_if(notes_.rbegin(), notes_.rend(),
                               [pitchInOctave](uint8_t n) { return n < pitchInOctave; });
        if (rit != notes_.rend()) {
            return *rit + octave * 12;
        } else {
            // Wrap to previous octave
            return notes_.back() + (octave > 0 ? (octave - 1) : octave) * 12;
        }
    }

    // Move to previous scale degree
    if (it != notes_.begin()) {
        --it;
        return *it + octave * 12;
    } else {
        // Wrap to previous octave
        return notes_.back() + (octave > 0 ? (octave - 1) : octave) * 12;
    }
}

int Scale::getScaleDegree(uint8_t pitch) const {
    if (notes_.empty()) return -1;

    uint8_t pitchInOctave = pitch % 12;

    // Find scale degree (0-based)
    auto it = std::find(notes_.begin(), notes_.end(), pitchInOctave);
    if (it != notes_.end()) {
        return static_cast<int>(std::distance(notes_.begin(), it));
    }

    return -1;  // Not in scale
}

}  // namespace procedural
