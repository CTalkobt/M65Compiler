#include "procedural/AmbientComposer.hpp"

namespace procedural {

const std::string AmbientComposer::description_ =
    "Ambient: Minimalist chords, slow evolution, pad textures for soundscapes";

AmbientComposer::AmbientComposer()
    : ProceduralComposer("Ambient") {}

std::unique_ptr<audio::Song> AmbientComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Ambient Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Generate minimalist progression (long held chords)
    auto harmonyProg = harmonicGen_->generateProgression(
        key,
        lengthBars,
        seed,
        ProgressionStyle::MINIMALIST,
        complexity_ * 0.5f  // Keep it minimal
    );

    // Add long-held harmony
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Ambient Pads");

    // Generate ambient melody (very slow, sparse)
    Scale scale = Scale::Major(static_cast<Note>(static_cast<uint8_t>(key) % 12));
    auto melody = melodicGen_->generateRandomWalk(
        scale,
        std::max(2u, lengthBars / 4),  // Very sparse notes
        seed ^ 0xAAAA,
        1,  // Tiny steps (stay close to root)
        true
    );

    // Add sparse atmospheric melody
    addMelodyTrack(*song, scale, lengthTicks, seed ^ 0xAAAA, "Ambient Texture");

    return song;
}

const std::string& AmbientComposer::getDescription() const {
    return description_;
}

void AmbientComposer::setMinimalism(float level) {
    minimalism_ = std::max(0.0f, std::min(1.0f, level));
}

void AmbientComposer::setEvolutionRate(float rate) {
    evolutionRate_ = std::max(0.0f, std::min(1.0f, rate));
}

void AmbientComposer::setReverbAmount(float amount) {
    reverbAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

}  // namespace procedural
