#include "procedural/FolkComposer.hpp"

namespace procedural {

const std::string FolkComposer::description_ =
    "Folk: Simple progressions, storytelling structure, traditional melodies";

FolkComposer::FolkComposer()
    : ProceduralComposer("Folk") {}

std::unique_ptr<audio::Song> FolkComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Folk uses simple I-IV-V progressions
    auto harmonyProg = harmonicGen_->generateFolkProgression(key, lengthBars, seed);

    // Add harmony
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Folk Chords");

    // Generate folk melody (pentatonic for authenticity)
    Scale pentatonic = Scale::Pentatonic(
        static_cast<Note>(static_cast<uint8_t>(key) % 12),
        true  // Major pentatonic
    );

    addMelodyTrack(*song, pentatonic, lengthTicks, seed ^ 0x2222, "Folk Melody");

    // Folk often has bass accompaniment
    addBassTrack(*song, pentatonic, lengthTicks, seed ^ 0x3333, "Folk Bass");

    return song;
}

const std::string& FolkComposer::getDescription() const {
    return description_;
}

void FolkComposer::setNarrativeStructure(float structure) {
    narrativeStructure_ = std::max(0.0f, std::min(1.0f, structure));
}

void FolkComposer::setOrnamentationAmount(float amount) {
    ornamentationAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

void FolkComposer::setTraditionalism(float level) {
    traditionalism_ = std::max(0.0f, std::min(1.0f, level));
}

}  // namespace procedural
