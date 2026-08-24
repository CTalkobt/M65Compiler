#include "audio/Track.hpp"
#include "audio/Pattern.hpp"

namespace audio {

Track::Track(Channel channel, const std::string& name)
    : channel_(channel), name_(name) {
    // Set default instrument based on channel
    switch (channel_) {
        case Channel::MELODY:
            instrument_ = "piano";
            break;
        case Channel::BASS:
            instrument_ = "bass";
            break;
        case Channel::HARMONY:
            instrument_ = "strings";
            break;
        case Channel::PERCUSSION:
            instrument_ = "drums";
            break;
        case Channel::EFFECT:
            instrument_ = "pad";
            break;
    }
}

Pattern* Track::addPattern(std::unique_ptr<Pattern> pattern) {
    if (!pattern) return nullptr;
    Pattern* ptr = pattern.get();
    patterns_.push_back(std::move(pattern));
    patternPtrs_.push_back(ptr);
    return ptr;
}

void Track::removePattern(Pattern* pattern) {
    if (!pattern) return;

    for (auto it = patterns_.begin(); it != patterns_.end(); ++it) {
        if (it->get() == pattern) {
            patterns_.erase(it);
            break;
        }
    }

    for (auto it = patternPtrs_.begin(); it != patternPtrs_.end(); ++it) {
        if (*it == pattern) {
            patternPtrs_.erase(it);
            break;
        }
    }
}

std::vector<Pattern*> Track::getPatterns() {
    return patternPtrs_;
}

const std::vector<Pattern*> Track::getPatterns() const {
    return patternPtrs_;
}

Track::~Track() = default;  // Destructor for proper cleanup of unique_ptr<Pattern>

}  // namespace audio
