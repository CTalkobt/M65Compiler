#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <array>

namespace audio {

// Effect types
enum class EffectType : uint8_t {
    REVERB = 0,
    CHORUS = 1,
    DELAY = 2,
    DISTORTION = 3,
    FLANGER = 4,
    PHASER = 5
};

// Base effect class
class Effect {
public:
    Effect(EffectType type, const std::string& name)
        : effectType_(type), effectName_(name), enabled_(true), wetDry_(64) {}

    virtual ~Effect() = default;

    // Effect control
    EffectType getType() const { return effectType_; }
    const std::string& getName() const { return effectName_; }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

    uint8_t getWetDry() const { return wetDry_; }
    void setWetDry(uint8_t wetDry) { wetDry_ = wetDry; }  // 0=dry, 64=50/50, 127=wet

    // Parameter control
    virtual void setParameter(uint8_t index, uint8_t value) = 0;
    virtual uint8_t getParameter(uint8_t index) const = 0;
    virtual uint8_t getParameterCount() const = 0;
    virtual std::string getParameterName(uint8_t index) const = 0;

    // Audio processing (stub - actual processing depends on effect type)
    virtual void process(uint8_t* audioBuffer, uint32_t bufferLength) = 0;

    // Initialize effect
    virtual void initialize() = 0;
    virtual void shutdown() = 0;

protected:
    EffectType effectType_;
    std::string effectName_;
    bool enabled_;
    uint8_t wetDry_;  // Wet/Dry mix (0-127)
};

// Reverb effect
class ReverbEffect : public Effect {
public:
    ReverbEffect();
    ~ReverbEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 4; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Reverb parameters
    uint8_t roomSize_ = 80;      // 0-127
    uint8_t damping_ = 50;        // 0-127
    uint8_t width_ = 100;         // 0-127
    uint8_t dryLevel_ = 40;       // 0-127

    // Reverb state (simplified - real reverb uses delay lines)
    std::array<uint32_t, 8> delayBuffer_;
    uint32_t delayIndex_ = 0;
};

// Chorus effect
class ChorusEffect : public Effect {
public:
    ChorusEffect();
    ~ChorusEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 3; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Chorus parameters
    uint8_t rate_ = 40;           // LFO rate (0-127)
    uint8_t depth_ = 50;          // Modulation depth (0-127)
    uint8_t feedback_ = 30;       // Feedback amount (0-127)

    // Chorus state
    uint32_t lfoPhase_ = 0;
    std::array<uint8_t, 256> delayBuffer_;
    uint32_t writeIndex_ = 0;
};

// Delay/Echo effect
class DelayEffect : public Effect {
public:
    DelayEffect();
    ~DelayEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 3; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Delay parameters
    uint8_t delayTime_ = 50;      // Delay time in ms (0-127)
    uint8_t feedback_ = 60;       // Feedback amount (0-127)
    uint8_t mix_ = 50;            // Wet/Dry mix (0-127)

    // Delay state
    std::array<uint8_t, 512> delayBuffer_;
    uint32_t writeIndex_ = 0;
    uint32_t readIndex_ = 0;
};

// Distortion/Overdrive effect
class DistortionEffect : public Effect {
public:
    DistortionEffect();
    ~DistortionEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 3; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Distortion parameters
    uint8_t drive_ = 50;          // Input gain (0-127)
    uint8_t tone_ = 64;           // Tone control (0-127)
    uint8_t level_ = 80;          // Output level (0-127)

    // Distortion state
    uint8_t lastSample_ = 128;
};

// Flanger effect
class FlangerEffect : public Effect {
public:
    FlangerEffect();
    ~FlangerEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 3; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Flanger parameters
    uint8_t rate_ = 30;           // LFO rate (0-127)
    uint8_t depth_ = 60;          // Depth (0-127)
    uint8_t feedback_ = 40;       // Feedback (0-127)

    // Flanger state
    uint32_t lfoPhase_ = 0;
    std::array<uint8_t, 256> delayBuffer_;
    uint32_t writeIndex_ = 0;
};

// Phaser effect
class PhaserEffect : public Effect {
public:
    PhaserEffect();
    ~PhaserEffect() override = default;

    void initialize() override;
    void shutdown() override;

    void setParameter(uint8_t index, uint8_t value) override;
    uint8_t getParameter(uint8_t index) const override;
    uint8_t getParameterCount() const override { return 3; }
    std::string getParameterName(uint8_t index) const override;

    void process(uint8_t* audioBuffer, uint32_t bufferLength) override;

private:
    // Phaser parameters
    uint8_t rate_ = 35;           // LFO rate (0-127)
    uint8_t depth_ = 70;          // Depth (0-127)
    uint8_t feedback_ = 50;       // Feedback (0-127)

    // Phaser state
    uint32_t lfoPhase_ = 0;
};

}  // namespace audio
