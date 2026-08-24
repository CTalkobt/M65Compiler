#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Techno-specific procedural composer
// Features: Steady beat, repetitive patterns, electronic sound
class TechnoComposer : public ProceduralComposer {
public:
    TechnoComposer();
    ~TechnoComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Techno-specific parameters
    void setSubBassIntensity(float intensity);  // Heavy low end
    void setDrumVariation(float variation);  // Pattern randomness
    void setFilterSweep(float amount);  // Synth filter automation

private:
    float subBassIntensity_ = 0.8f;
    float drumVariation_ = 0.3f;
    float filterSweep_ = 0.5f;

    static const std::string description_;
};

}  // namespace procedural
