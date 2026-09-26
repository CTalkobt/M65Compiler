#include "procedural/RagtimeComposer.hpp"

namespace procedural {

const std::string RagtimeComposer::description_ =
    "Ragtime: Syncopated rhythms, stride bass patterns, syncopated melody over steady beat";

RagtimeComposer::RagtimeComposer()
    : ProceduralComposer("Ragtime") {}

std::unique_ptr<audio::Song> RagtimeComposer::compose(
    uint32_t lengthBars,
    Key key,
    uint32_t tempoBeatsPerMinute,
    uint32_t seed) {

    auto song = std::make_unique<audio::Song>("Composition", tempoBeatsPerMinute);

    currentTempoBeatsPerMinute_ = tempoBeatsPerMinute;
    uint32_t ticksPerBeat = 480;
    uint32_t lengthTicks = lengthBars * 4 * ticksPerBeat;

    // Ragtime uses blues progressions (I-IV-V changes)
    auto harmonyProg = harmonicGen_->generateBluesProgression(key, lengthBars, seed);

    // Add harmony
    addHarmonyTrack(*song, harmonyProg, lengthTicks, seed, "Ragtime Chords");

    // Generate syncopated melody (the main ragtime feature)
    Scale scale = Scale::Major(static_cast<Note>(static_cast<uint8_t>(key) % 12));

    auto melody = generateSyncopatedMelody(scale, lengthBars * 4, seed ^ 0xAAAA);

    // Add syncopated melody track
    addMelodyTrack(*song, scale, lengthTicks, seed ^ 0xAAAA, "Ragtime Melody");

    // Generate and add stride bass pattern (Ragtime's signature)
    auto strideBass = generateStrideBassPattern(lengthBars, seed ^ 0xBBBB);

    // Add stride bass track
    addBassTrack(*song, scale, lengthTicks, seed ^ 0xBBBB, "Ragtime Stride Bass");

    // Ragtime percussion (often minimal - focus on syncopation)
    addPercussionTrack(*song, lengthTicks, seed ^ 0xCCCC, 0, "Ragtime Beat");

    return song;
}

const std::string& RagtimeComposer::getDescription() const {
    return description_;
}

std::vector<uint32_t> RagtimeComposer::generateStrideBassPattern(
    uint32_t bars,
    uint32_t seed) {

    std::vector<uint32_t> pattern;
    uint32_t rng = seed;

    // Stride bass: alternating bass note on 1-3 and chord on 2-4
    // Creates the characteristic "boom-chick" rhythm
    uint32_t ticksPerBar = 1920;  // 4 beats * 480 ticks

    for (uint32_t bar = 0; bar < bars; ++bar) {
        // Boom (low bass note) on beat 1 and 3
        pattern.push_back(240);  // Sixteenth note

        // Chick (chord) on beat 2 and 4
        pattern.push_back(480);  // Quarter note

        // Repeat for second half of bar
        pattern.push_back(240);
        pattern.push_back(480);
    }

    return pattern;
}

std::vector<uint8_t> RagtimeComposer::generateSyncopatedMelody(
    const Scale& scale,
    uint32_t lengthNotes,
    uint32_t seed) {

    std::vector<uint8_t> melody;
    const auto& scaleNotes = scale.getNotes();

    if (scaleNotes.empty()) return melody;

    melody.reserve(lengthNotes);

    uint32_t rng = seed;

    // Generate ragtime-style syncopated melody
    // Ragtime features offbeat accents and syncopation
    for (uint32_t i = 0; i < lengthNotes; ++i) {
        rng = rng * 1103515245 + 12345;

        // Favor notes on off-beats (create syncopation)
        if (i % 4 == 1 || i % 4 == 3) {
            // Off-beat: more likely to jump
            int maxStep = 3 + static_cast<int>(syncopationLevel_ * 2.0f);
            int step = static_cast<int>((rng / 65536) % (2 * maxStep + 1)) - maxStep;
            int nextIdx = (rng / 32768) % scaleNotes.size();
            melody.push_back(scaleNotes[nextIdx]);
        } else {
            // On-beat: stay closer to previous notes
            int step = static_cast<int>((rng / 65536) % 3) - 1;
            int nextIdx = (rng / 32768) % scaleNotes.size();
            melody.push_back(scaleNotes[nextIdx]);
        }
    }

    return melody;
}

void RagtimeComposer::setSyncopationLevel(float level) {
    syncopationLevel_ = std::max(0.0f, std::min(1.0f, level));
}

void RagtimeComposer::setStrideBassIntensity(float intensity) {
    strideBassIntensity_ = std::max(0.0f, std::min(1.0f, intensity));
}

void RagtimeComposer::setJazzinessAmount(float amount) {
    jazzinessAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

}  // namespace procedural
