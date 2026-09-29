#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Chiptune-specific procedural composer
// Features: Retro game music, arpeggios, simple melodies
class ChiptuneComposer : public ProceduralComposer {
public:
    ChiptuneComposer();
    ~ChiptuneComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Chiptune-specific parameters
    void setRetroStyle(float style);  // 0.0 = modern, 1.0 = 8-bit
    void setArpeggioAmount(float amount);  // How much arp vs melody
    void setSquareWaveIntensity(float intensity);  // Digital harshness

private:
    float retroStyle_ = 0.8f;
    float arpeggioAmount_ = 0.6f;
    float squareWaveIntensity_ = 0.7f;

    static const std::string description_;
};

}  // namespace procedural
