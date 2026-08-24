#include "audio/Effect.hpp"
#include <algorithm>
#include <cmath>

namespace audio {

// ============================================================================
// ReverbEffect Implementation
// ============================================================================

ReverbEffect::ReverbEffect()
    : Effect(EffectType::REVERB, "Reverb") {
    delayBuffer_.fill(0);
}

void ReverbEffect::initialize() {
    delayBuffer_.fill(0);
    delayIndex_ = 0;
}

void ReverbEffect::shutdown() {
    delayBuffer_.fill(0);
}

void ReverbEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: roomSize_ = value; break;
        case 1: damping_ = value; break;
        case 2: width_ = value; break;
        case 3: dryLevel_ = value; break;
        default: break;
    }
}

uint8_t ReverbEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return roomSize_;
        case 1: return damping_;
        case 2: return width_;
        case 3: return dryLevel_;
        default: return 0;
    }
}

std::string ReverbEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Room Size";
        case 1: return "Damping";
        case 2: return "Width";
        case 3: return "Dry Level";
        default: return "Unknown";
    }
}

void ReverbEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    // Simplified reverb: delay buffer with feedback
    for (uint32_t i = 0; i < bufferLength; ++i) {
        uint32_t readIndex = (delayIndex_ + 4) % delayBuffer_.size();
        uint32_t delayed = delayBuffer_[readIndex];

        // Mix with input
        uint32_t mixed = (static_cast<uint32_t>(audioBuffer[i]) + delayed) / 2;
        delayBuffer_[delayIndex_] = mixed;

        // Apply damping
        delayBuffer_[delayIndex_] = (delayBuffer_[delayIndex_] * (127 - damping_) / 128);

        delayIndex_ = (delayIndex_ + 1) % delayBuffer_.size();
    }
}

// ============================================================================
// ChorusEffect Implementation
// ============================================================================

ChorusEffect::ChorusEffect()
    : Effect(EffectType::CHORUS, "Chorus") {
    delayBuffer_.fill(128);  // Initialize to silence
}

void ChorusEffect::initialize() {
    delayBuffer_.fill(128);
    writeIndex_ = 0;
    lfoPhase_ = 0;
}

void ChorusEffect::shutdown() {
    delayBuffer_.fill(128);
}

void ChorusEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: rate_ = value; break;
        case 1: depth_ = value; break;
        case 2: feedback_ = value; break;
        default: break;
    }
}

uint8_t ChorusEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return rate_;
        case 1: return depth_;
        case 2: return feedback_;
        default: return 0;
    }
}

std::string ChorusEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        default: return "Unknown";
    }
}

void ChorusEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    for (uint32_t i = 0; i < bufferLength; ++i) {
        // Generate LFO (triangle wave)
        uint32_t lfoValue = (lfoPhase_ < 128) ? lfoPhase_ : (255 - lfoPhase_);
        lfoPhase_ = (lfoPhase_ + rate_ / 32) % 256;

        // Modulate delay time
        uint32_t delayOffset = (10 + (lfoValue * depth_) / 256) % delayBuffer_.size();
        uint32_t readIndex = (writeIndex_ + delayBuffer_.size() - delayOffset) % delayBuffer_.size();

        uint8_t delayed = delayBuffer_[readIndex];
        uint32_t output = (static_cast<uint32_t>(audioBuffer[i]) + delayed) / 2;

        delayBuffer_[writeIndex_] = audioBuffer[i];
        audioBuffer[i] = static_cast<uint8_t>(output);

        writeIndex_ = (writeIndex_ + 1) % delayBuffer_.size();
    }
}

// ============================================================================
// DelayEffect Implementation
// ============================================================================

DelayEffect::DelayEffect()
    : Effect(EffectType::DELAY, "Delay") {
    delayBuffer_.fill(128);
}

void DelayEffect::initialize() {
    delayBuffer_.fill(128);
    writeIndex_ = 0;
    readIndex_ = 0;
}

void DelayEffect::shutdown() {
    delayBuffer_.fill(128);
}

void DelayEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: delayTime_ = value; break;
        case 1: feedback_ = value; break;
        case 2: mix_ = value; break;
        default: break;
    }
}

uint8_t DelayEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return delayTime_;
        case 1: return feedback_;
        case 2: return mix_;
        default: return 0;
    }
}

std::string DelayEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Mix";
        default: return "Unknown";
    }
}

void DelayEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    uint32_t delaySize = (static_cast<uint32_t>(delayTime_) + 1) * 4;
    delaySize = std::min(delaySize, static_cast<uint32_t>(delayBuffer_.size()));

    for (uint32_t i = 0; i < bufferLength; ++i) {
        readIndex_ = (writeIndex_ + delayBuffer_.size() - delaySize) % delayBuffer_.size();
        uint8_t delayed = delayBuffer_[readIndex_];

        uint32_t mixed = (static_cast<uint32_t>(audioBuffer[i]) * (127 - mix_) +
                         static_cast<uint32_t>(delayed) * mix_) / 127;

        delayBuffer_[writeIndex_] = audioBuffer[i];
        uint32_t feedback_out = (audioBuffer[i] + delayed * feedback_ / 127) / 2;
        delayBuffer_[writeIndex_] = static_cast<uint8_t>(feedback_out);

        audioBuffer[i] = static_cast<uint8_t>(mixed);
        writeIndex_ = (writeIndex_ + 1) % delayBuffer_.size();
    }
}

// ============================================================================
// DistortionEffect Implementation
// ============================================================================

DistortionEffect::DistortionEffect()
    : Effect(EffectType::DISTORTION, "Distortion") {
}

void DistortionEffect::initialize() {
    lastSample_ = 128;
}

void DistortionEffect::shutdown() {
    lastSample_ = 128;
}

void DistortionEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: drive_ = value; break;
        case 1: tone_ = value; break;
        case 2: level_ = value; break;
        default: break;
    }
}

uint8_t DistortionEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return drive_;
        case 1: return tone_;
        case 2: return level_;
        default: return 0;
    }
}

std::string DistortionEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Drive";
        case 1: return "Tone";
        case 2: return "Level";
        default: return "Unknown";
    }
}

void DistortionEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    for (uint32_t i = 0; i < bufferLength; ++i) {
        // Apply gain (drive)
        int32_t sample = static_cast<int32_t>(audioBuffer[i]) - 128;
        sample = (sample * static_cast<int32_t>(drive_)) / 64;

        // Hard clipping
        sample = std::max(-127, std::min(127, sample));

        // Simple low-pass filter (tone control)
        int32_t filtered = (static_cast<int32_t>(lastSample_) - 128 + sample) / 2;
        lastSample_ = static_cast<uint8_t>(filtered + 128);

        // Apply output level
        int32_t output = (filtered * static_cast<int32_t>(level_)) / 127;
        audioBuffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, output + 128)));
    }
}

// ============================================================================
// FlangerEffect Implementation
// ============================================================================

FlangerEffect::FlangerEffect()
    : Effect(EffectType::FLANGER, "Flanger") {
    delayBuffer_.fill(128);
}

void FlangerEffect::initialize() {
    delayBuffer_.fill(128);
    writeIndex_ = 0;
    lfoPhase_ = 0;
}

void FlangerEffect::shutdown() {
    delayBuffer_.fill(128);
}

void FlangerEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: rate_ = value; break;
        case 1: depth_ = value; break;
        case 2: feedback_ = value; break;
        default: break;
    }
}

uint8_t FlangerEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return rate_;
        case 1: return depth_;
        case 2: return feedback_;
        default: return 0;
    }
}

std::string FlangerEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        default: return "Unknown";
    }
}

void FlangerEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    for (uint32_t i = 0; i < bufferLength; ++i) {
        // Generate sine-like LFO
        uint32_t lfoValue = (lfoPhase_ < 128) ? lfoPhase_ : (255 - lfoPhase_);
        lfoPhase_ = (lfoPhase_ + rate_ / 32) % 256;

        // Modulate delay
        uint32_t delayOffset = (2 + (lfoValue * depth_) / 512) % delayBuffer_.size();
        uint32_t readIndex = (writeIndex_ + delayBuffer_.size() - delayOffset) % delayBuffer_.size();

        uint8_t delayed = delayBuffer_[readIndex];
        uint32_t mixed = (static_cast<uint32_t>(audioBuffer[i]) + delayed) / 2;

        delayBuffer_[writeIndex_] = audioBuffer[i];
        audioBuffer[i] = static_cast<uint8_t>(mixed);

        writeIndex_ = (writeIndex_ + 1) % delayBuffer_.size();
    }
}

// ============================================================================
// PhaserEffect Implementation
// ============================================================================

PhaserEffect::PhaserEffect()
    : Effect(EffectType::PHASER, "Phaser") {
}

void PhaserEffect::initialize() {
    lfoPhase_ = 0;
}

void PhaserEffect::shutdown() {
    // Phaser doesn't need buffer cleanup
}

void PhaserEffect::setParameter(uint8_t index, uint8_t value) {
    switch (index) {
        case 0: rate_ = value; break;
        case 1: depth_ = value; break;
        case 2: feedback_ = value; break;
        default: break;
    }
}

uint8_t PhaserEffect::getParameter(uint8_t index) const {
    switch (index) {
        case 0: return rate_;
        case 1: return depth_;
        case 2: return feedback_;
        default: return 0;
    }
}

std::string PhaserEffect::getParameterName(uint8_t index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        default: return "Unknown";
    }
}

void PhaserEffect::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (!enabled_ || bufferLength == 0) return;

    for (uint32_t i = 0; i < bufferLength; ++i) {
        // Generate LFO
        lfoPhase_ = (lfoPhase_ + rate_ / 32) % 256;

        // All-pass filter simulation using phase shift
        int32_t sample = static_cast<int32_t>(audioBuffer[i]) - 128;
        int32_t phased = sample - (sample * static_cast<int32_t>(depth_) / 256);

        audioBuffer[i] = static_cast<uint8_t>(std::max(0, std::min(255, phased + 128)));
    }
}

}  // namespace audio
