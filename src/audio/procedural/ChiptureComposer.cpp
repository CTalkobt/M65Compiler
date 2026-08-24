#include "procedural/ChiptureComposer.hpp"

namespace procedural {

const std::string ChiptuneComposer::description_ =
    "Chiptune: Retro game music, arpeggios, simple melodies, 8-bit aesthetic";

ChiptuneComposer::ChiptuneComposer()
    : ProceduralComposer("Chiptune") {}

std::unique_ptr<audio::Song> ChiptuneComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Chiptune uses simple progressions
    auto harmonyProg = harmonicGen_->generatePopProgression(key, lengthBars, seed);

    // Add harmony (arpeggiated)
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Chiptune Square");

    // Generate simple, catchy melody
    Scale scale = Scale::Pentatonic(
        static_cast<Note>(static_cast<uint8_t>(key) % 12),
        true  // Pentatonic is typical for chiptune
    );

    auto melody = melodicGen_->generateRandomWalk(
        scale,
        lengthBars * 8,  // Quick notes for 8-bit feel
        seed ^ 0x0808,
        1,  // Small steps for pentatonic jumps
        false  // Don't pull toward center (more jumpy)
    );

    // Add melody
    addMelodyTrack(*song, scale, lengthTicks, seed ^ 0x0808, "Chiptune Pulse");

    // Retro drums (simple but energetic)
    addPercussionTrack(*song, lengthTicks, seed ^ 0x0909, 0, "Chiptune Drums");

    return song;
}

const std::string& ChiptuneComposer::getDescription() const {
    return description_;
}

void ChiptuneComposer::setRetroStyle(float style) {
    retroStyle_ = std::max(0.0f, std::min(1.0f, style));
}

void ChiptuneComposer::setArpeggioAmount(float amount) {
    arpeggioAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

void ChiptuneComposer::setSquareWaveIntensity(float intensity) {
    squareWaveIntensity_ = std::max(0.0f, std::min(1.0f, intensity));
}

}  // namespace procedural
