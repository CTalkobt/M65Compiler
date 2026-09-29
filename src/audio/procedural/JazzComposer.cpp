#include "procedural/JazzComposer.hpp"

namespace procedural {

const std::string JazzComposer::description_ =
    "Jazz: ii-V-I progressions, swing rhythm, complex harmonies with improvisation";

JazzComposer::JazzComposer()
    : ProceduralComposer("Jazz") {}

std::unique_ptr<audio::Song> JazzComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Jazz Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Generate jazz harmony (ii-V-I with variations)
    auto harmonyProg = harmonicGen_->generateJazzProgression(key, lengthBars, seed);

    // Add harmony track
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Jazz Harmony");

    // Generate melody with jazz improvization
    Scale majorScale = Scale::Major(static_cast<Note>(static_cast<uint8_t>(key) % 12));
    auto melody = melodicGen_->generateMarkovMelody(
        majorScale,
        lengthBars * 4,  // 4 notes per bar
        seed ^ 0x12345,
        complexity_ + 0.2f  // Slightly more complex for jazz
    );

    // Add melody track (jazz soloist)
    addMelodyTrack(*song, majorScale, lengthTicks, seed ^ 0x12345, "Jazz Melody");

    // Generate bass line (walks the changes)
    auto bassLine = melodicGen_->generateRandomWalk(
        majorScale,
        lengthBars,  // One note per bar as anchor
        seed ^ 0x54321,
        2,  // Moderate step size
        true
    );

    // Add bass track
    addBassTrack(*song, majorScale, lengthTicks, seed ^ 0x54321, "Jazz Bass");

    // Add drums with swing (from swing amount parameter)
    uint8_t drummerId = 1;  // Jazz drummer (1)
    addPercussionTrack(*song, lengthTicks, seed ^ 0x99999, drummerId, "Jazz Drums");

    return song;
}

const std::string& JazzComposer::getDescription() const {
    return description_;
}

void JazzComposer::setSwingAmount(float amount) {
    swingAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

void JazzComposer::setReharmonizationDensity(float density) {
    reharmonizationDensity_ = std::max(0.0f, std::min(1.0f, density));
}

void JazzComposer::setImprovizationDensity(float density) {
    improvizationDensity_ = std::max(0.0f, std::min(1.0f, density));
}

}  // namespace procedural
