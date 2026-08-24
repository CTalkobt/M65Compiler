#include "audio/EffectsChain.hpp"
#include <algorithm>

namespace audio {

EffectsChain::EffectsChain()
    : masterDry_(100), masterWet_(50), bypassAll_(false), samplesProcessed_(0) {
}

void EffectsChain::addEffect(std::unique_ptr<Effect> effect) {
    if (effect && effects_.size() < 8) {  // Limit to 8 effects in chain
        effect->initialize();
        effects_.push_back(std::move(effect));
    }
}

void EffectsChain::removeEffect(uint8_t index) {
    if (index < effects_.size()) {
        effects_[index]->shutdown();
        effects_.erase(effects_.begin() + index);
    }
}

void EffectsChain::clearEffects() {
    for (auto& effect : effects_) {
        effect->shutdown();
    }
    effects_.clear();
}

Effect* EffectsChain::getEffect(uint8_t index) {
    if (index < effects_.size()) {
        return effects_[index].get();
    }
    return nullptr;
}

const Effect* EffectsChain::getEffect(uint8_t index) const {
    if (index < effects_.size()) {
        return effects_[index].get();
    }
    return nullptr;
}

void EffectsChain::moveEffect(uint8_t fromIndex, uint8_t toIndex) {
    if (fromIndex < effects_.size() && toIndex < effects_.size() && fromIndex != toIndex) {
        auto effect = std::move(effects_[fromIndex]);
        effects_.erase(effects_.begin() + fromIndex);
        effects_.insert(effects_.begin() + toIndex, std::move(effect));
    }
}

void EffectsChain::swapEffects(uint8_t index1, uint8_t index2) {
    if (index1 < effects_.size() && index2 < effects_.size() && index1 != index2) {
        std::swap(effects_[index1], effects_[index2]);
    }
}

void EffectsChain::enableEffect(uint8_t index, bool enabled) {
    if (index < effects_.size()) {
        effects_[index]->setEnabled(enabled);
    }
}

void EffectsChain::setEffectWetDry(uint8_t index, uint8_t wetDry) {
    if (index < effects_.size()) {
        effects_[index]->setWetDry(wetDry);
    }
}

void EffectsChain::process(uint8_t* audioBuffer, uint32_t bufferLength) {
    if (bypassAll_ || effects_.empty() || !audioBuffer) {
        return;
    }

    // Save original dry signal
    std::vector<uint8_t> drySignal(audioBuffer, audioBuffer + bufferLength);

    // Process through effects chain
    for (auto& effect : effects_) {
        if (effect && effect->isEnabled()) {
            effect->process(audioBuffer, bufferLength);
        }
    }

    // Mix dry and wet signals based on master levels
    for (uint32_t i = 0; i < bufferLength; ++i) {
        uint32_t dryValue = drySignal[i];
        uint32_t wetValue = audioBuffer[i];

        uint32_t mixed = (dryValue * masterDry_ + wetValue * masterWet_) / 127;
        audioBuffer[i] = static_cast<uint8_t>(std::min(255U, mixed));
    }

    samplesProcessed_ += bufferLength;
}

void EffectsChain::setMasterDry(uint8_t dry) {
    masterDry_ = std::min(dry, static_cast<uint8_t>(127));
}

void EffectsChain::setMasterWet(uint8_t wet) {
    masterWet_ = std::min(wet, static_cast<uint8_t>(127));
}

uint8_t EffectsChain::mixSignals(uint8_t dry, uint8_t wet, uint8_t mix) const {
    uint32_t mixed = (static_cast<uint32_t>(dry) * (127 - mix) +
                      static_cast<uint32_t>(wet) * mix) / 127;
    return static_cast<uint8_t>(std::min(255U, mixed));
}

}  // namespace audio
