#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Folk-specific procedural composer
// Features: Simple progressions, traditional structures, storytelling
class FolkComposer : public ProceduralComposer {
public:
    FolkComposer();
    ~FolkComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Folk-specific parameters
    void setNarrativeStructure(float structure);  // Verse-Chorus balance
    void setOrnamentationAmount(float amount);  // Melodic embellishment
    void setTraditionalism(float level);  // How classical vs modern

private:
    float narrativeStructure_ = 0.5f;
    float ornamentationAmount_ = 0.4f;
    float traditionalism_ = 0.8f;

    static const std::string description_;
};

}  // namespace procedural
