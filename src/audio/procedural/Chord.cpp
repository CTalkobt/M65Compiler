#include "procedural/Chord.hpp"
#include <sstream>

namespace procedural {

Chord::Chord(Note root, ChordQuality quality)
    : root_(root), quality_(quality) {

    initializeIntervals();

    // Generate chord name
    std::ostringstream ss;
    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    ss << noteNames[static_cast<uint8_t>(root_)];

    switch (quality_) {
        case ChordQuality::MAJOR:
            ss << " Major"; break;
        case ChordQuality::MINOR:
            ss << " Minor"; break;
        case ChordQuality::DIMINISHED:
            ss << "°"; break;
        case ChordQuality::AUGMENTED:
            ss << "+"; break;
        case ChordQuality::MAJOR7:
            ss << "Maj7"; break;
        case ChordQuality::MINOR7:
            ss << "m7"; break;
        case ChordQuality::DOMINANT7:
            ss << "7"; break;
        case ChordQuality::MINOR_MAJOR7:
            ss << "m(Maj7)"; break;
        case ChordQuality::HALF_DIM7:
            ss << "ø7"; break;
        case ChordQuality::SUSPENDED2:
            ss << "sus2"; break;
        case ChordQuality::SUSPENDED4:
            ss << "sus4"; break;
        case ChordQuality::MAJOR6:
            ss << "6"; break;
        case ChordQuality::MINOR6:
            ss << "m6"; break;
        case ChordQuality::ADD9:
            ss << "add9"; break;
        case ChordQuality::MAJOR9:
            ss << "Maj9"; break;
        case ChordQuality::MINOR9:
            ss << "m9"; break;
    }

    name_ = ss.str();
}

void Chord::initializeIntervals() {
    intervals_.clear();

    switch (quality_) {
        case ChordQuality::MAJOR:
            intervals_ = { 0, 4, 7 };  // root, major 3rd, perfect 5th
            break;

        case ChordQuality::MINOR:
            intervals_ = { 0, 3, 7 };  // root, minor 3rd, perfect 5th
            break;

        case ChordQuality::DIMINISHED:
            intervals_ = { 0, 3, 6 };  // root, minor 3rd, diminished 5th
            break;

        case ChordQuality::AUGMENTED:
            intervals_ = { 0, 4, 8 };  // root, major 3rd, augmented 5th
            break;

        case ChordQuality::MAJOR7:
            intervals_ = { 0, 4, 7, 11 };  // Major triad + major 7th
            break;

        case ChordQuality::MINOR7:
            intervals_ = { 0, 3, 7, 10 };  // Minor triad + minor 7th
            break;

        case ChordQuality::DOMINANT7:
            intervals_ = { 0, 4, 7, 10 };  // Major triad + minor 7th
            break;

        case ChordQuality::MINOR_MAJOR7:
            intervals_ = { 0, 3, 7, 11 };  // Minor triad + major 7th
            break;

        case ChordQuality::HALF_DIM7:
            intervals_ = { 0, 3, 6, 10 };  // Diminished triad + minor 7th
            break;

        case ChordQuality::SUSPENDED2:
            intervals_ = { 0, 2, 7 };  // root, major 2nd, perfect 5th
            break;

        case ChordQuality::SUSPENDED4:
            intervals_ = { 0, 5, 7 };  // root, perfect 4th, perfect 5th
            break;

        case ChordQuality::MAJOR6:
            intervals_ = { 0, 4, 7, 9 };  // Major triad + major 6th
            break;

        case ChordQuality::MINOR6:
            intervals_ = { 0, 3, 7, 9 };  // Minor triad + major 6th
            break;

        case ChordQuality::ADD9:
            intervals_ = { 0, 4, 7, 14 };  // Major triad + major 9th (2 octaves up)
            break;

        case ChordQuality::MAJOR9:
            intervals_ = { 0, 4, 7, 11, 14 };  // Major 7th + major 9th
            break;

        case ChordQuality::MINOR9:
            intervals_ = { 0, 3, 7, 10, 14 };  // Minor 7th + major 9th
            break;
    }
}

std::vector<uint8_t> Chord::getVoicing(uint8_t voicingStyle, uint8_t octave) const {
    std::vector<uint8_t> voicing;

    // Base octave for root note
    uint8_t baseOctave = octave;
    uint8_t root = static_cast<uint8_t>(root_);

    switch (voicingStyle) {
        case 0:  // Root position
            for (uint8_t interval : intervals_) {
                voicing.push_back(root + interval + baseOctave * 12);
            }
            break;

        case 1:  // First inversion (third in bass)
            if (intervals_.size() > 1) {
                voicing.push_back(root + intervals_[1] + baseOctave * 12);
                for (size_t i = 0; i < intervals_.size(); ++i) {
                    if (i != 1) {
                        voicing.push_back(root + intervals_[i] + (baseOctave + 1) * 12);
                    }
                }
            }
            break;

        case 2:  // Second inversion (fifth in bass)
            if (intervals_.size() > 2) {
                voicing.push_back(root + intervals_[2] + baseOctave * 12);
                voicing.push_back(root + intervals_[0] + (baseOctave + 1) * 12);
                for (size_t i = 1; i < intervals_.size(); ++i) {
                    if (i != 2) {
                        voicing.push_back(root + intervals_[i] + (baseOctave + 1) * 12);
                    }
                }
            }
            break;

        case 3:  // Drop-2 voicing (second note an octave lower)
            for (size_t i = 0; i < intervals_.size(); ++i) {
                if (i == 1 && intervals_.size() > 1) {
                    voicing.push_back(root + intervals_[i] + (baseOctave - 1 + 2) * 12);  // Drop 2 octaves
                } else {
                    voicing.push_back(root + intervals_[i] + baseOctave * 12);
                }
            }
            break;

        default:  // Fallback to root position
            for (uint8_t interval : intervals_) {
                voicing.push_back(root + interval + baseOctave * 12);
            }
            break;
    }

    return voicing;
}

uint8_t Chord::getRootVoicing(uint8_t octave) const {
    uint8_t root = static_cast<uint8_t>(root_);
    return root + octave * 12;
}

uint8_t Chord::getBassNote(uint8_t inversion, uint8_t octave) const {
    if (intervals_.empty()) {
        return static_cast<uint8_t>(root_) + octave * 12;
    }

    uint8_t root = static_cast<uint8_t>(root_);
    uint8_t interval = 0;

    if (inversion > 0 && static_cast<size_t>(inversion - 1) < intervals_.size()) {
        interval = intervals_[inversion - 1];
    }

    return root + interval + octave * 12;
}

}  // namespace procedural
