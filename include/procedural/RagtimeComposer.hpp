#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Ragtime-specific procedural composer
// Features: Syncopated rhythms, stride bass patterns, syncopated melody
class RagtimeComposer : public ProceduralComposer {
public:
    RagtimeComposer();
    ~RagtimeComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Ragtime-specific parameters
    void setSyncopationLevel(float level);  // 0.0 = straight, 1.0 = heavily syncopated
    float getSyncopationLevel() const { return syncopationLevel_; }

    void setStrideBassIntensity(float intensity);  // Boom-chick bass pattern
    float getStrideBassIntensity() const { return strideBassIntensity_; }

    void setJazzinessAmount(float amount);  // Modern vs traditional ragtime
    float getJazzinessAmount() const { return jazzinessAmount_; }

private:
    float syncopationLevel_ = 0.9f;      // Ragtime is heavily syncopated
    float strideBassIntensity_ = 0.8f;   // Classic stride piano bass
    float jazzinessAmount_ = 0.3f;       // Lean toward traditional ragtime

    static const std::string description_;

    // Helper methods for ragtime-specific patterns
    std::vector<uint32_t> generateStrideBassPattern(uint32_t bars, uint32_t seed);
    std::vector<uint8_t> generateSyncopatedMelody(
        const Scale& scale,
        uint32_t lengthNotes,
        uint32_t seed
    );
};

}  // namespace procedural
