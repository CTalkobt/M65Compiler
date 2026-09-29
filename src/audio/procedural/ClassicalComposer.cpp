#include "procedural/ClassicalComposer.hpp"

namespace procedural {

const std::string ClassicalComposer::description_ =
    "Classical: Complex harmonies, sonata forms, development sections, modulation";

ClassicalComposer::ClassicalComposer()
    : ProceduralComposer("Classical") {}

std::unique_ptr<audio::Song> ClassicalComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Classical uses formal progressions (sonata form)
    auto harmonyProg = harmonicGen_->generateClassicalProgression(key, lengthBars, seed);

    // Add harmony
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Classical Harmony");

    // Generate classical melody with more complexity
    Scale scale = Scale::Major(static_cast<Note>(static_cast<uint8_t>(key) % 12));

    auto melody = melodicGen_->generateMarkovMelody(
        scale,
        lengthBars * 4,
        seed ^ 0x5555,
        complexity_ + 0.3f  // Higher complexity for classical
    );

    // Add melody (violin line)
    addMelodyTrack(*song, scale, lengthTicks, seed ^ 0x5555, "Classical Melody");

    // Add bass line (cello)
    addBassTrack(*song, scale, lengthTicks, seed ^ 0x6666, "Classical Bass");

    return song;
}

const std::string& ClassicalComposer::getDescription() const {
    return description_;
}

void ClassicalComposer::setFormality(float level) {
    formality_ = std::max(0.0f, std::min(1.0f, level));
}

void ClassicalComposer::setModulationDensity(float density) {
    modulationDensity_ = std::max(0.0f, std::min(1.0f, density));
}

void ClassicalComposer::setPolyphonyLevel(float level) {
    polyphonyLevel_ = std::max(0.0f, std::min(1.0f, level));
}

}  // namespace procedural
