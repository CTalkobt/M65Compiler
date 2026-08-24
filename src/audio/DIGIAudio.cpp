#include "audio/DIGIAudio.hpp"
#include <algorithm>
#include <cstring>

namespace audio {

DIGIAudio::DIGIAudio() {
}

void DIGIAudio::loadSample(const uint8_t* sampleData, uint32_t length) {
    sampleData_.clear();
    if (sampleData && length > 0) {
        sampleData_.insert(sampleData_.begin(), sampleData, sampleData + length);
        sampleLength_ = length;
        currentPosition_ = 0;
    }
}

void DIGIAudio::play() {
    if (sampleLength_ > 0) {
        isPlaying_ = true;
        currentPosition_ = 0;
    }
}

void DIGIAudio::pause() {
    isPlaying_ = false;
}

void DIGIAudio::stop() {
    isPlaying_ = false;
    currentPosition_ = 0;
}

void DIGIAudio::reset() {
    isPlaying_ = false;
    currentPosition_ = 0;
    sampleData_.clear();
    sampleLength_ = 0;
}

void DIGIAudio::setVolume(uint8_t volume) {
    volume_ = std::min(volume, static_cast<uint8_t>(127));
}

void DIGIAudio::setPan(uint8_t pan) {
    pan_ = std::min(pan, static_cast<uint8_t>(127));
}

void DIGIAudio::setPlaybackRate(float rate) {
    playbackRate_ = std::max(0.5f, std::min(rate, 2.0f));
}

float DIGIAudio::getProgress() const {
    if (sampleLength_ == 0) return 0.0f;
    return static_cast<float>(currentPosition_) / sampleLength_;
}

void DIGIAudio::updateHardware() {
    if (!isPlaying_ || sampleLength_ == 0) return;

    // In real hardware, this would:
    // 1. Write current sample to DIGI audio register
    // 2. Advance currentPosition_ based on playbackRate_
    // 3. Handle end-of-sample looping

    // Simulate sample advancement
    uint32_t increment = static_cast<uint32_t>(playbackRate_ * 10);  // Simplified
    currentPosition_ += increment;

    if (currentPosition_ >= sampleLength_) {
        isPlaying_ = false;
        currentPosition_ = 0;
    }
}

void DIGIAudio::writeDIGIRegister(uint16_t offset, uint8_t value) {
    // MEGA65 DIGI registers at $D700 + offset
    // Register map:
    // $00: Control (play/pause/stop)
    // $01: Volume
    // $02: Pan
    // $03-$06: Sample address (24-bit)
    // $07-$0A: Sample length (24-bit)
    // $0B: Playback rate

    volatile uint8_t* digiReg = reinterpret_cast<volatile uint8_t*>(DIGI_BASE + offset);
    *digiReg = value;
}

uint8_t DIGIAudio::readDIGIRegister(uint16_t offset) const {
    volatile uint8_t* digiReg = reinterpret_cast<volatile uint8_t*>(DIGI_BASE + offset);
    return *digiReg;
}

}  // namespace audio
