#pragma once

#include <memory>
#include "../audio/Song.hpp"
#include "Scale.hpp"
#include "MelodicGenerator.hpp"
#include "HarmonicGenerator.hpp"
#include "RhythmicGenerator.hpp"

namespace procedural {

// Base class for genre-specific procedural composers
class ProceduralComposer {
public:
    ProceduralComposer(const std::string& genreName);
    virtual ~ProceduralComposer() = default;

    // Main composition method (implemented by subclasses)
    virtual std::unique_ptr<audio::Song> compose(
        uint32_t lengthBars,
        Key key,
        uint32_t tempoBeatsPerMinute,
        uint32_t seed
    ) = 0;

    // Metadata
    const std::string& getGenreName() const { return genreName_; }
    virtual const std::string& getDescription() const = 0;

    // Common parameters for all genres
    virtual void setComplexity(float complexity);
    virtual void setEnergy(float energy);
    virtual void setEmotionalTone(float tone);  // 0=sad, 0.5=neutral, 1=happy

    float getComplexity() const { return complexity_; }
    float getEnergy() const { return energy_; }
    float getEmotionalTone() const { return emotionalTone_; }

protected:
    std::string genreName_;
    float complexity_;          // 0.0-1.0
    float energy_;              // 0.0-1.0
    float emotionalTone_;       // 0.0-1.0

    // Generator instances (shared across subclasses)
    std::unique_ptr<MelodicGenerator> melodicGen_;
    std::unique_ptr<HarmonicGenerator> harmonicGen_;
    std::unique_ptr<RhythmicGenerator> rhythmicGen_;

    // For derived classes to use
    uint32_t currentTempoBeatsPerMinute_ = 120;

    // Helper methods for subclasses
    void addMelodyTrack(
        audio::Song& song,
        const Scale& scale,
        uint32_t lengthTicks,
        uint32_t seed,
        const std::string& trackName = "Melody"
    );

    void addBassTrack(
        audio::Song& song,
        const Scale& scale,
        uint32_t lengthTicks,
        uint32_t seed,
        const std::string& trackName = "Bass"
    );

    void addHarmonyTrack(
        audio::Song& song,
        const ChordProgression& progression,
        uint32_t lengthTicks,
        uint32_t seed,
        const std::string& trackName = "Harmony"
    );

    void addPercussionTrack(
        audio::Song& song,
        uint32_t lengthTicks,
        uint32_t seed,
        uint8_t drummerId = 0,
        const std::string& trackName = "Percussion"
    );

    // Generate chords from progression into track
    void fillHarmonyTrackWithChords(
        audio::Track& track,
        const ChordProgression& progression,
        uint8_t voicing = 0,
        uint8_t octave = 4
    );

    // Get appropriate note duration based on tempo
    uint32_t getNoteDuration(float noteLengthBeats);
};

}  // namespace procedural
