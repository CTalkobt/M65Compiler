#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Jazz-specific procedural composer
// Features: ii-V-I progressions, swing rhythm, improvisation
class JazzComposer : public ProceduralComposer {
public:
    JazzComposer();
    ~JazzComposer() = default;

    // Main composition method
    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Jazz-specific parameters
    void setSwingAmount(float amount);  // 0.0 = straight, 1.0 = full swing
    float getSwingAmount() const { return swingAmount_; }

    void setReharmonizationDensity(float density);  // How many chords per bar
    float getReharmonizationDensity() const { return reharmonizationDensity_; }

    void setImprovizationDensity(float density);  // How much melodic variation
    float getImprovizationDensity() const { return improvizationDensity_; }

private:
    float swingAmount_ = 0.6f;
    float reharmonizationDensity_ = 0.5f;
    float improvizationDensity_ = 0.7f;

    static const std::string description_;
};

}  // namespace procedural
