#pragma once

#include "audio/Effect.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace audio {

// Effects chain/processor for applying multiple effects in sequence
class EffectsChain {
public:
    EffectsChain();
    ~EffectsChain() = default;

    // Effect management
    void addEffect(std::unique_ptr<Effect> effect);
    void removeEffect(uint8_t index);
    void clearEffects();
    uint8_t getEffectCount() const { return static_cast<uint8_t>(effects_.size()); }

    // Effect access
    Effect* getEffect(uint8_t index);
    const Effect* getEffect(uint8_t index) const;

    // Effect order (reordering for signal flow control)
    void moveEffect(uint8_t fromIndex, uint8_t toIndex);
    void swapEffects(uint8_t index1, uint8_t index2);

    // Effect control
    void enableEffect(uint8_t index, bool enabled);
    void setEffectWetDry(uint8_t index, uint8_t wetDry);

    // Audio processing
    void process(uint8_t* audioBuffer, uint32_t bufferLength);

    // Master control
    void setMasterDry(uint8_t dry);        // Master dry level (0-127)
    void setMasterWet(uint8_t wet);        // Master wet level (0-127)
    uint8_t getMasterDry() const { return masterDry_; }
    uint8_t getMasterWet() const { return masterWet_; }

    // Bypass all effects
    void bypass(bool bypassed) { bypassAll_ = bypassed; }
    bool isBypassed() const { return bypassAll_; }

    // Statistics
    uint32_t getSamplesProcessed() const { return samplesProcessed_; }
    void resetStatistics() { samplesProcessed_ = 0; }

private:
    std::vector<std::unique_ptr<Effect>> effects_;
    uint8_t masterDry_ = 100;
    uint8_t masterWet_ = 50;
    bool bypassAll_ = false;
    uint32_t samplesProcessed_ = 0;

    // Helper for mixing wet/dry signals
    uint8_t mixSignals(uint8_t dry, uint8_t wet, uint8_t mix) const;
};

}  // namespace audio
