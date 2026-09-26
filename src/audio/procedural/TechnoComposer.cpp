#include "procedural/TechnoComposer.hpp"

namespace procedural {

const std::string TechnoComposer::description_ =
    "Techno: Steady beat, repetitive patterns, driving electronic soundscape";

TechnoComposer::TechnoComposer()
    : ProceduralComposer("Techno") {}

std::unique_ptr<audio::Song> TechnoComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Techno uses very minimal harmony (often single chord)
    auto harmonyProg = harmonicGen_->generateProgression(
        key,
        lengthBars,
        seed,
        ProgressionStyle::MINIMALIST,
        0.2f  // Very minimal harmony for techno
    );

    // Add harmony
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Techno Synth");

    // Generate repetitive bass line
    Scale scale = Scale::Major(static_cast<Note>(static_cast<uint8_t>(key) % 12));
    addBassTrack(*song, scale, lengthTicks, seed ^ 0x7777, "Techno Sub Bass");

    // Heavy drum patterns (techno is rhythm-driven)
    addPercussionTrack(*song, lengthTicks, seed ^ 0x4444, 0, "Techno Drums");

    return song;
}

const std::string& TechnoComposer::getDescription() const {
    return description_;
}

void TechnoComposer::setSubBassIntensity(float intensity) {
    subBassIntensity_ = std::max(0.0f, std::min(1.0f, intensity));
}

void TechnoComposer::setDrumVariation(float variation) {
    drumVariation_ = std::max(0.0f, std::min(1.0f, variation));
}

void TechnoComposer::setFilterSweep(float amount) {
    filterSweep_ = std::max(0.0f, std::min(1.0f, amount));
}

}  // namespace procedural
