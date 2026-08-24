#pragma once

#include "ProceduralComposer.hpp"

namespace procedural {

// Classical-specific procedural composer
// Features: Complex harmonies, sonata forms, development sections
class ClassicalComposer : public ProceduralComposer {
public:
    ClassicalComposer();
    ~ClassicalComposer() = default;

    std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) override;

    const std::string& getDescription() const override;

    // Classical-specific parameters
    void setFormality(float level);  // How strict the structure
    void setModulationDensity(float density);  // Key changes per piece
    void setPolyphonyLevel(float level);  // Contrapuntal complexity

private:
    float formality_ = 0.9f;
    float modulationDensity_ = 0.4f;
    float polyphonyLevel_ = 0.6f;

    static const std::string description_;
};

}  // namespace procedural
