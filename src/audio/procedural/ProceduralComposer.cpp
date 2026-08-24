#include "procedural/ProceduralComposer.hpp"
#include "audio/Track.hpp"
#include "audio/Pattern.hpp"

namespace procedural {

ProceduralComposer::ProceduralComposer(const std::string& genreName)
    : genreName_(genreName),
      complexity_(0.5f),
      energy_(0.5f),
      emotionalTone_(0.5f) {

    // Initialize generator instances
    melodicGen_ = std::make_unique<MelodicGenerator>();
    harmonicGen_ = std::make_unique<HarmonicGenerator>();
    rhythmicGen_ = std::make_unique<RhythmicGenerator>();
}

void ProceduralComposer::setComplexity(float complexity) {
    complexity_ = std::max(0.0f, std::min(1.0f, complexity));
}

void ProceduralComposer::setEnergy(float energy) {
    energy_ = std::max(0.0f, std::min(1.0f, energy));
}

void ProceduralComposer::setEmotionalTone(float tone) {
    emotionalTone_ = std::max(0.0f, std::min(1.0f, tone));
}

void ProceduralComposer::addMelodyTrack(
    audio::Song& song,
    const Scale& scale,
    uint32_t lengthTicks,
    uint32_t seed,
    const std::string& trackName) {

    // Create track
    auto track = std::make_unique<audio::Track>(
        audio::Track::Channel::MELODY,
        trackName
    );

    // Generate melody
    auto melody = melodicGen_->generate(
        scale,
        lengthTicks,
        seed,
        MelodicStrategy::MARKOV_CHAIN,
        complexity_
    );

    // Add notes to track
    auto patterns = track->getPatterns();
    if (!patterns.empty() && patterns[0]) {
        uint32_t noteDuration = 480;  // Quarter note
        uint32_t currentTick = 0;

        for (uint8_t pitch : melody) {
            if (currentTick < lengthTicks) {
                patterns[0]->addNote(pitch, currentTick, noteDuration, 100);
                currentTick += noteDuration;
            }
        }
    }

    song.addTrack(std::move(track));
}

void ProceduralComposer::addBassTrack(
    audio::Song& song,
    const Scale& scale,
    uint32_t lengthTicks,
    uint32_t seed,
    const std::string& trackName) {

    auto track = std::make_unique<audio::Track>(
        audio::Track::Channel::BASS,
        trackName
    );

    // Bass typically moves slower, with lower notes
    auto bassLine = melodicGen_->generateRandomWalk(
        scale,
        lengthTicks / 960,  // Longer notes for bass
        seed,
        1,  // Smaller steps
        true
    );

    auto patterns = track->getPatterns();
    if (!patterns.empty() && patterns[0]) {
        uint32_t noteDuration = 960;  // Half note
        uint32_t currentTick = 0;

        for (uint8_t pitch : bassLine) {
            if (currentTick < lengthTicks) {
                // Transpose down 2 octaves
                uint8_t bassNote = (pitch >= 24) ? (pitch - 24) : pitch;
                patterns[0]->addNote(bassNote, currentTick, noteDuration, 90);
                currentTick += noteDuration;
            }
        }
    }

    song.addTrack(std::move(track));
}

void ProceduralComposer::addHarmonyTrack(
    audio::Song& song,
    const ChordProgression& progression,
    uint32_t /*lengthTicks*/,
    uint32_t /*seed*/,
    const std::string& trackName) {

    auto track = std::make_unique<audio::Track>(
        audio::Track::Channel::HARMONY,
        trackName
    );

    fillHarmonyTrackWithChords(*track, progression, 0, 4);
    song.addTrack(std::move(track));
}

void ProceduralComposer::addPercussionTrack(
    audio::Song& song,
    uint32_t lengthTicks,
    uint32_t seed,
    uint8_t drummerId,
    const std::string& trackName) {

    auto track = std::make_unique<audio::Track>(
        audio::Track::Channel::PERCUSSION,
        trackName
    );

    auto patterns = track->getPatterns();
    if (!patterns.empty() && patterns[0]) {
        uint32_t lengthBars = lengthTicks / (480 * 4);
        uint32_t ticksPerBeat = 480;

        auto drumPattern = rhythmicGen_->generateDrumPattern(
            lengthBars,
            ticksPerBeat,
            seed,
            drummerId
        );

        // Add drum hits as notes with percussion pitches
        for (const auto& [tick, intensity] : drumPattern) {
            uint8_t pitch = 36 + (intensity / 16) % 16;  // Percussion note range
            patterns[0]->addNote(pitch, tick, 240, intensity);
        }
    }

    song.addTrack(std::move(track));
}

void ProceduralComposer::fillHarmonyTrackWithChords(
    audio::Track& track,
    const ChordProgression& progression,
    uint8_t /*voicing*/,
    uint8_t octave) {

    // Get patterns from track
    auto patterns = track.getPatterns();
    if (patterns.empty()) return;

    auto pattern = patterns[0];
    if (!pattern) return;

    uint32_t currentTick = 0;

    for (const auto& entry : progression.getChords()) {
        const Chord& chord = entry.chord;
        uint32_t duration = entry.durationTicks / 4;  // Distribute across 4 notes

        // Get voicing notes
        auto voicingNotes = chord.getVoicing(entry.voicing, octave);

        // Add chord tones
        for (size_t i = 0; i < voicingNotes.size() && currentTick < progression.getTotalDuration(); ++i) {
            pattern->addNote(
                voicingNotes[i],
                currentTick,
                duration,
                80
            );
            currentTick += duration;
        }
    }
}

uint32_t ProceduralComposer::getNoteDuration(float noteLengthBeats) {
    // Convert beat length to ticks
    // Assuming 480 ticks per beat (standard)
    return static_cast<uint32_t>(noteLengthBeats * 480.0f);
}

}  // namespace procedural
