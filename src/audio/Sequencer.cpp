#include "audio/Sequencer.hpp"
#include <sstream>

namespace audio {

Sequencer::Sequencer() {
    // Initialize note states for 16 tracks, 128 pitches each
    noteStates_.resize(16, std::vector<bool>(128, false));
}

Sequencer::~Sequencer() {
    stop();
    unloadSong();
}

void Sequencer::loadSong(std::unique_ptr<Song> song) {
    if (isPlaying_) stop();
    currentSong_ = std::move(song);
    currentPosition_ = 0;
}

void Sequencer::unloadSong() {
    if (isPlaying_) stop();
    currentSong_.reset();
}

void Sequencer::play() {
    if (!currentSong_) return;
    if (isPlaying_) return;
    isPlaying_ = true;
    isPaused_ = false;
    currentSong_->play();
}

void Sequencer::pause() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    isPaused_ = true;
    if (currentSong_) currentSong_->pause();
}

void Sequencer::stop() {
    isPlaying_ = false;
    isPaused_ = false;
    currentPosition_ = 0;
    if (currentSong_) currentSong_->stop();
    metronomeCounter_ = 0;
}

void Sequencer::reset() {
    stop();
}

void Sequencer::setCurrentPosition(uint32_t position) {
    if (!currentSong_) return;
    if (position > currentSong_->getDurationTicks()) {
        position = currentSong_->getDurationTicks();
    }
    currentPosition_ = position;
    if (currentSong_) currentSong_->setCurrentPosition(position);
}

uint32_t Sequencer::quantizeTick(uint32_t tick) const {
    if (!currentSong_) return tick;

    uint32_t quantizeUnit = 0;
    switch (quantizeMode_) {
        case QuantizeMode::NONE:
            return tick;
        case QuantizeMode::SIXTEENTH:
            quantizeUnit = currentSong_->getTimeSignature().ticksPerBeat / 4;
            break;
        case QuantizeMode::EIGHTH:
            quantizeUnit = currentSong_->getTimeSignature().ticksPerBeat / 2;
            break;
        case QuantizeMode::QUARTER:
            quantizeUnit = currentSong_->getTimeSignature().ticksPerBeat;
            break;
        case QuantizeMode::HALF:
            quantizeUnit = currentSong_->getTimeSignature().ticksPerBeat * 2;
            break;
    }

    if (quantizeUnit == 0) return tick;
    return (tick + quantizeUnit / 2) / quantizeUnit * quantizeUnit;
}

void Sequencer::advanceTick(uint32_t deltaTicks) {
    if (!isPlaying_ || !currentSong_) return;

    currentPosition_ += static_cast<uint32_t>(deltaTicks * playbackSpeed_);

    // Check looping
    if (loopEnabled_ && currentPosition_ >= loopEnd_) {
        currentPosition_ = loopStart_;
    } else if (currentPosition_ >= currentSong_->getDurationTicks()) {
        if (loopEnabled_) {
            currentPosition_ = loopStart_;
        } else {
            stop();
            return;
        }
    }

    // Process notes and control changes at current position
    processNotesAtTick(currentPosition_);
    processControlChangesAtTick(currentPosition_);

    // Update metronome
    if (metronomeEnabled_) {
        triggerMetronome();
    }
}

void Sequencer::advanceBar() {
    if (!currentSong_) return;
    uint32_t nextBar = (currentPosition_ / currentSong_->getTimeSignature().ticksPerBar() + 1)
                       * currentSong_->getTimeSignature().ticksPerBar();
    setCurrentPosition(nextBar);
}

Sequencer::ActiveNotes Sequencer::getActiveNotes() const {
    ActiveNotes result;
    if (!currentSong_) return result;

    const auto tracks = currentSong_->getTracks();
    for (size_t i = 0; i < tracks.size() && i < 16; ++i) {
        const auto patterns = tracks[i]->getPatterns();
        for (const auto pattern : patterns) {
            const Note* note = pattern->getNoteAt(currentPosition_);
            if (note) {
                result.notes.push_back({tracks[i], note});
            }
        }
    }

    return result;
}

void Sequencer::startRecording() {
    isRecording_ = true;
}

void Sequencer::stopRecording() {
    isRecording_ = false;
}

uint32_t Sequencer::getTotalNotes() const {
    uint32_t count = 0;
    if (!currentSong_) return count;

    const auto tracks = currentSong_->getTracks();
    for (const auto track : tracks) {
        const auto patterns = track->getPatterns();
        for (const auto pattern : patterns) {
            count += pattern->getNotes().size();
        }
    }

    return count;
}

uint32_t Sequencer::getTotalEvents() const {
    return getTotalNotes();  // Could include CC events, etc.
}

std::string Sequencer::getStatistics() const {
    std::ostringstream oss;
    if (!currentSong_) {
        oss << "No song loaded\n";
        return oss.str();
    }

    oss << "Song: " << currentSong_->getName() << "\n";
    oss << "Tempo: " << currentSong_->getTempo() << " BPM\n";
    oss << "Tracks: " << currentSong_->getTrackCount() << "\n";
    oss << "Total Notes: " << getTotalNotes() << "\n";
    oss << "Duration: " << currentSong_->getDurationBars() << " bars\n";
    oss << "Position: " << currentPosition_ << " ticks\n";
    oss << "Playing: " << (isPlaying_ ? "Yes" : "No") << "\n";

    return oss.str();
}

void Sequencer::processNotesAtTick(uint32_t tick) {
    if (!currentSong_ || !noteEventCallback_) return;

    const auto tracks = currentSong_->getTracks();
    for (size_t i = 0; i < tracks.size() && i < 16; ++i) {
        const auto patterns = tracks[i]->getPatterns();
        for (const auto pattern : patterns) {
            const Note* note = pattern->getNoteAt(tick);

            // Check if note just started
            if (note && !noteStates_[i][note->pitch]) {
                noteStates_[i][note->pitch] = true;
                noteEventCallback_(tick, tracks[i], note, true);  // Note on
            }

            // Check if note just ended
            if (!note && noteStates_[i][note ? note->pitch : 0]) {
                if (note) {
                    noteStates_[i][note->pitch] = false;
                    noteEventCallback_(tick, tracks[i], note, false);  // Note off
                }
            }
        }
    }
}

void Sequencer::processControlChangesAtTick(uint32_t tick) {
    if (!currentSong_ || !ccCallback_) return;

    const auto tracks = currentSong_->getTracks();
    for (const auto track : tracks) {
        const auto patterns = track->getPatterns();
        for (const auto pattern : patterns) {
            const auto& ccs = pattern->getControlChanges();
            for (const auto& cc : ccs) {
                if (cc.tick == tick) {
                    ccCallback_(tick, track, cc);
                }
            }
        }
    }
}

void Sequencer::updateNoteStates() {
    // Reset all note states
    for (auto& trackStates : noteStates_) {
        std::fill(trackStates.begin(), trackStates.end(), false);
    }

    // Set states based on active notes at current position
    auto active = getActiveNotes();
    // Would update noteStates_ based on active notes
}

void Sequencer::triggerMetronome() {
    uint32_t beatTicks = currentSong_->getTimeSignature().ticksPerBeat;
    uint32_t barTicks = currentSong_->getTimeSignature().ticksPerBar();

    // Trigger click on beat boundaries
    if (currentPosition_ % beatTicks == 0) {
        // On-beat click (metronome sound)
        if (currentPosition_ % barTicks == 0) {
            // Louder click on first beat of bar
        }
    }
}

}  // namespace audio
