#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Ambient-specific procedural composer
// Features: Minimalist chords, slow evolution, pad textures
class AmbientComposer : public ProceduralComposer {
public:
    AmbientComposer();
    ~AmbientComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Ambient-specific parameters
    void setMinimalism(float level);  // 0.0 = complex, 1.0 = single note
    void setEvolutionRate(float rate);  // How fast soundscape changes
    void setReverbAmount(float amount);  // Overall reverb effect

private:
    float minimalism_ = 0.7f;
    float evolutionRate_ = 0.3f;
    float reverbAmount_ = 0.8f;

    static const std::string description_;
};

}  // namespace procedural
