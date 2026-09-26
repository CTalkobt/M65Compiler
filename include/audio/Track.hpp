#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace audio {

// Forward declarations
class Pattern;

// Track: represents a single melodic or rhythmic line within a song
class Track {
public:
    enum class Channel {
        MELODY = 0,      // Main melodic line
        BASS = 1,        // Bass/low frequency line
        HARMONY = 2,     // Harmonic accompaniment
        PERCUSSION = 3,  // Drums and percussion
        EFFECT = 4       // Special effects, pads
    };

    Track(Channel channel, const std::string& name = "");
    ~Track();  // Needs proper definition for unique_ptr<Pattern> cleanup

    // Identity
    Channel getChannel() const { return channel_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    // Instrument setup
    const std::string& getInstrument() const { return instrument_; }
    void setInstrument(const std::string& instrument) { instrument_ = instrument; }

    // Volume control (0-127)
    uint8_t getVolume() const { return volume_; }
    void setVolume(uint8_t volume) {
        volume_ = (volume <= 127) ? volume : 127;
    }

    // Pan (0-127, 64 = center)
    uint8_t getPan() const { return pan_; }
    void setPan(uint8_t pan) {
        pan_ = (pan <= 127) ? pan : 127;
    }

    // Mute/Solo
    bool isMuted() const { return muted_; }
    void setMuted(bool muted) { muted_ = muted; }

    bool isSolo() const { return solo_; }
    void setSolo(bool solo) { solo_ = solo; }

    // Pattern management
    Pattern* addPattern(std::unique_ptr<Pattern> pattern);
    void removePattern(Pattern* pattern);
    std::vector<Pattern*> getPatterns();
    const std::vector<Pattern*> getPatterns() const;
    size_t getPatternCount() const { return patterns_.size(); }

    // Effects (reverb, chorus, distortion, etc.)
    void setReverbAmount(float amount) { reverb_ = (amount >= 0.0f && amount <= 1.0f) ? amount : 0.0f; }
    float getReverbAmount() const { return reverb_; }

    void setChorusAmount(float amount) { chorus_ = (amount >= 0.0f && amount <= 1.0f) ? amount : 0.0f; }
    float getChorusAmount() const { return chorus_; }

    // MEGA65 specific settings
    void setSIDChip(uint8_t chip) { sidChip_ = (chip < 4) ? chip : 0; }
    uint8_t getSIDChip() const { return sidChip_; }

    void setDIGIEnabled(bool enabled) { digiEnabled_ = enabled; }
    bool getDIGIEnabled() const { return digiEnabled_; }

private:
    Channel channel_;
    std::string name_;
    std::string instrument_;

    uint8_t volume_ = 100;      // 0-127
    uint8_t pan_ = 64;          // 0-127 (64 = center)
    bool muted_ = false;
    bool solo_ = false;

    float reverb_ = 0.0f;       // 0.0 - 1.0
    float chorus_ = 0.0f;       // 0.0 - 1.0

    uint8_t sidChip_ = 0;       // Which SID chip (0-3 for 4 SID chips)
    bool digiEnabled_ = false;  // Use DIGI audio chip

    std::vector<std::unique_ptr<Pattern>> patterns_;
    std::vector<Pattern*> patternPtrs_;  // Non-owning pointers
};

}  // namespace audio
