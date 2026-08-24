#include "audio/Song.hpp"
#include "audio/Track.hpp"
#include <fstream>
#include <iostream>

namespace audio {

Song::Song(const std::string& name, uint32_t tempo)
    : name_(name), tempo_(tempo), timeSig_(4, 4, 480) {
    if (tempo_ < 1 || tempo_ > 999) tempo_ = 120;
    durationTicks_ = timeSig_.ticksPerBar() * 32;  // Default 32 bars
}

Song::~Song() = default;  // Destructor for proper cleanup of unique_ptr<Track>

Track* Song::addTrack(std::unique_ptr<Track> track) {
    if (!track) return nullptr;
    Track* ptr = track.get();
    tracks_.push_back(std::move(track));
    trackPtrs_.push_back(ptr);
    return ptr;
}

void Song::removeTrack(Track* track) {
    if (!track) return;

    // Remove from owning vector
    for (auto it = tracks_.begin(); it != tracks_.end(); ++it) {
        if (it->get() == track) {
            tracks_.erase(it);
            break;
        }
    }

    // Remove from pointer vector
    for (auto it = trackPtrs_.begin(); it != trackPtrs_.end(); ++it) {
        if (*it == track) {
            trackPtrs_.erase(it);
            break;
        }
    }
}

std::vector<Track*> Song::getTracks() {
    return trackPtrs_;
}

const std::vector<Track*> Song::getTracks() const {
    return trackPtrs_;
}

void Song::play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    isPaused_ = false;
}

void Song::pause() {
    if (!isPlaying_) return;
    isPaused_ = true;
    isPlaying_ = false;
}

void Song::stop() {
    isPlaying_ = false;
    isPaused_ = false;
    currentPosition_ = 0;
}

void Song::reset() {
    currentPosition_ = 0;
    isPlaying_ = false;
    isPaused_ = false;
}

bool Song::saveTo(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing\n";
        return false;
    }

    // Write header (magic number)
    const char* magic = "SONG";
    file.write(magic, 4);

    // Write song metadata
    uint32_t nameLen = name_.length();
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(name_.c_str(), nameLen);

    file.write(reinterpret_cast<const char*>(&tempo_), sizeof(tempo_));
    file.write(reinterpret_cast<const char*>(&timeSig_.numerator), sizeof(timeSig_.numerator));
    file.write(reinterpret_cast<const char*>(&timeSig_.denominator), sizeof(timeSig_.denominator));
    file.write(reinterpret_cast<const char*>(&timeSig_.ticksPerBeat), sizeof(timeSig_.ticksPerBeat));
    file.write(reinterpret_cast<const char*>(&durationTicks_), sizeof(durationTicks_));

    // Write track count and data
    uint32_t trackCount = tracks_.size();
    file.write(reinterpret_cast<const char*>(&trackCount), sizeof(trackCount));

    // Note: Full serialization of tracks would be implemented here
    // For now, just basic structure

    file.close();
    return true;
}

bool Song::loadFrom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for reading\n";
        return false;
    }

    // Read header
    char magic[4];
    file.read(magic, 4);
    if (std::string(magic, 4) != "SONG") {
        std::cerr << "Error: Invalid song file format\n";
        return false;
    }

    // Read song metadata
    uint32_t nameLen;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    name_.resize(nameLen);
    file.read(&name_[0], nameLen);

    file.read(reinterpret_cast<char*>(&tempo_), sizeof(tempo_));
    file.read(reinterpret_cast<char*>(&timeSig_.numerator), sizeof(timeSig_.numerator));
    file.read(reinterpret_cast<char*>(&timeSig_.denominator), sizeof(timeSig_.denominator));
    file.read(reinterpret_cast<char*>(&timeSig_.ticksPerBeat), sizeof(timeSig_.ticksPerBeat));
    file.read(reinterpret_cast<char*>(&durationTicks_), sizeof(durationTicks_));

    // Read track count
    uint32_t trackCount;
    file.read(reinterpret_cast<char*>(&trackCount), sizeof(trackCount));

    // Note: Full deserialization of tracks would be implemented here

    file.close();
    return true;
}

}  // namespace audio
