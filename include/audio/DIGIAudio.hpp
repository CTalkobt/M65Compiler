#pragma once

#include <cstdint>
#include <vector>

namespace audio {

// DIGI Audio Channel - PCM sample playback on MEGA65
class DIGIAudio {
public:
    DIGIAudio();
    ~DIGIAudio() = default;

    // Sample playback
    void loadSample(const uint8_t* sampleData, uint32_t length);
    void play();
    void pause();
    void stop();
    void reset();

    // Playback control
    void setVolume(uint8_t volume);        // 0-127
    void setPan(uint8_t pan);              // 0-127 (64 = center)
    void setPlaybackRate(float rate);      // 0.5x - 2.0x

    // Status
    bool isPlaying() const { return isPlaying_; }
    uint32_t getCurrentPosition() const { return currentPosition_; }
    uint32_t getSampleLength() const { return sampleLength_; }
    float getProgress() const;

    // Hardware update
    void updateHardware();

private:
    // Sample data
    std::vector<uint8_t> sampleData_;
    uint32_t sampleLength_ = 0;
    uint32_t currentPosition_ = 0;

    // Playback state
    bool isPlaying_ = false;
    float playbackRate_ = 1.0f;

    // Volume and pan
    uint8_t volume_ = 127;
    uint8_t pan_ = 64;

    // Hardware registers
    void writeDIGIRegister(uint16_t offset, uint8_t value);
    uint8_t readDIGIRegister(uint16_t offset) const;

    // MEGA65 DIGI addresses
    static constexpr uint32_t DIGI_BASE = 0xD700;  // DIGI base address on MEGA65
};

}  // namespace audio
