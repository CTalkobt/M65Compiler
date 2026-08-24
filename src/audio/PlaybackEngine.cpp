#include "audio/PlaybackEngine.hpp"
#include <algorithm>
#include <cmath>

namespace audio {

PlaybackEngine::PlaybackEngine()
    : trackVolumes_(16, 100), trackMuted_(16, false) {}

bool PlaybackEngine::loadSong(const std::shared_ptr<Song>& song) {
    if (!song) return false;

    song_ = song;
    sequencer_ = std::make_unique<Sequencer>();

    // Calculate total ticks: bars * beats/bar * ticks/beat
    uint32_t barsEstimate = 32;  // Default estimate
    totalTicks_ = barsEstimate * 4 * 480;  // 4/4 time, 480 ticks per beat

    // Reset playback state
    currentTick_ = 0;
    isPlaying_ = false;
    isPaused_ = false;

    // Set initial tempo
    setTempo(song->getTempo());

    // Initialize track volumes from song
    auto tracks = song->getTracks();
    for (size_t i = 0; i < tracks.size() && i < 16; ++i) {
        if (tracks[i]) {
            trackVolumes_[i] = tracks[i]->getVolume();
        }
    }

    return true;
}

void PlaybackEngine::play() {
    if (!song_) return;

    isPlaying_ = true;
    isPaused_ = false;
    stats_.eventsPlayed = 0;
}

void PlaybackEngine::pause() {
    isPlaying_ = false;
    isPaused_ = true;
}

void PlaybackEngine::stop() {
    isPlaying_ = false;
    isPaused_ = false;
    currentTick_ = 0;
    activeNotes_.clear();
}

void PlaybackEngine::reset() {
    stop();
    stats_ = PlaybackStats();
}

float PlaybackEngine::getProgress() const {
    if (totalTicks_ == 0) return 0.0f;
    return static_cast<float>(currentTick_) / totalTicks_;
}

void PlaybackEngine::seek(uint32_t tick) {
    currentTick_ = std::min(tick, totalTicks_);
    activeNotes_.clear();  // Clear active notes on seek
}

void PlaybackEngine::seekToBar(uint32_t bar) {
    uint32_t ticksPerBar = 4 * 480;  // 4 beats, 480 ticks per beat
    seek(bar * ticksPerBar);
}

void PlaybackEngine::seekToPercent(float percent) {
    uint32_t tick = static_cast<uint32_t>(totalTicks_ * std::max(0.0f, std::min(1.0f, percent)));
    seek(tick);
}

void PlaybackEngine::setTempo(uint32_t beatsPerMinute) {
    tempoBeatsPerMinute_ = std::max(1u, std::min(300u, beatsPerMinute));
    updateMsPerTick();
}

void PlaybackEngine::setPlaybackSpeed(float speed) {
    playbackSpeed_ = std::max(0.1f, std::min(2.0f, speed));
    updateMsPerTick();
}

void PlaybackEngine::setMasterVolume(uint8_t volume) {
    masterVolume_ = volume;
}

void PlaybackEngine::setTrackVolume(uint8_t trackId, uint8_t volume) {
    if (trackId < trackVolumes_.size()) {
        trackVolumes_[trackId] = volume;
    }
}

uint8_t PlaybackEngine::getTrackVolume(uint8_t trackId) const {
    if (trackId < trackVolumes_.size()) {
        return trackVolumes_[trackId];
    }
    return 0;
}

void PlaybackEngine::setTrackMuted(uint8_t trackId, bool muted) {
    if (trackId < trackMuted_.size()) {
        trackMuted_[trackId] = muted;
    }
}

bool PlaybackEngine::isTrackMuted(uint8_t trackId) const {
    if (trackId < trackMuted_.size()) {
        return trackMuted_[trackId];
    }
    return false;
}

void PlaybackEngine::setLooping(bool enabled) {
    looping_ = enabled;
}

void PlaybackEngine::setLoopPoints(uint32_t startTick, uint32_t endTick) {
    loopStartTick_ = startTick;
    loopEndTick_ = std::max(startTick + 1, endTick);
}

void PlaybackEngine::setEventCallback(EventCallback callback) {
    eventCallback_ = callback;
}

void PlaybackEngine::scheduleEvents(uint32_t fromTick, uint32_t toTick) {
    generateNoteEvents(fromTick, toTick);
    generateControlEvents(fromTick, toTick);
}

std::vector<AudioEvent> PlaybackEngine::update(float deltaTimeMs) {
    if (!isPlaying_ || !song_) {
        return std::vector<AudioEvent>();
    }

    // Update current tick based on delta time and playback speed
    float tickDelta = (deltaTimeMs / msPerTick_) * playbackSpeed_;
    currentTick_ += static_cast<uint32_t>(tickDelta);

    // Handle looping
    if (looping_ && currentTick_ >= loopEndTick_) {
        currentTick_ = loopStartTick_;
    } else if (currentTick_ >= totalTicks_) {
        stop();
        return std::vector<AudioEvent>();
    }

    // Schedule events for upcoming time window
    uint32_t scheduleAhead = static_cast<uint32_t>(50.0f / msPerTick_);  // 50ms ahead
    scheduleEvents(currentTick_, currentTick_ + scheduleAhead);

    // Get pending events
    auto events = getPendingEvents();

    // Call callback for each event
    for (const auto& event : events) {
        if (eventCallback_) {
            eventCallback_(event);
        }
        stats_.eventsPlayed++;
    }

    // Update active notes tracking
    std::vector<ActiveNote> stillActive;
    for (const auto& note : activeNotes_) {
        if (currentTick_ < note.startTick + note.duration) {
            stillActive.push_back(note);
        }
    }
    activeNotes_ = stillActive;
    stats_.notesPlaying = activeNotes_.size();

    return events;
}

void PlaybackEngine::resetStats() {
    stats_ = PlaybackStats();
}

void PlaybackEngine::updateMsPerTick() {
    // Calculate milliseconds per tick
    // Formula: (60,000 ms/min) / (tempo * 480 ticks/beat)
    float ticksPerSecond = (tempoBeatsPerMinute_ * 480.0f) / 60.0f;
    msPerTick_ = 1000.0f / (ticksPerSecond * playbackSpeed_);
}

void PlaybackEngine::generateNoteEvents(uint32_t fromTick, uint32_t toTick) {
    if (!song_) return;

    // Iterate through all tracks and find notes in the time range
    auto tracks = song_->getTracks();
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        auto track = tracks[trackIdx];
        if (!track || isTrackMuted(trackIdx)) continue;

        // Get all patterns in the track
        auto patterns = track->getPatterns();
        for (auto pattern : patterns) {
            if (!pattern) continue;

            // Check all notes in the pattern
            const auto& notes = pattern->getNotes();
            for (const auto& note_ref : notes) {
                const Note* note = &note_ref;

                uint32_t noteStartTick = note->startTick;
                uint32_t noteEndTick = note->startTick + note->duration;

                // Check if note overlaps with our time window
                if (noteEndTick > fromTick && noteStartTick < toTick) {
                    // Schedule NOTE_ON event if start is in range
                    if (noteStartTick >= fromTick && noteStartTick < toTick) {
                        AudioEvent noteOn(
                            noteStartTick,
                            AudioEventType::NOTE_ON,
                            trackIdx,
                            note->pitch,
                            note->velocity
                        );
                        pendingEvents_.push_back(noteOn);

                        // Track active note
                        activeNotes_.emplace_back(
                            static_cast<uint8_t>(trackIdx),
                            note->pitch,
                            noteStartTick,
                            note->duration
                        );
                    }

                    // Schedule NOTE_OFF event if end is in range
                    if (noteEndTick >= fromTick && noteEndTick < toTick) {
                        AudioEvent noteOff(
                            noteEndTick,
                            AudioEventType::NOTE_OFF,
                            trackIdx,
                            note->pitch,
                            0
                        );
                        pendingEvents_.push_back(noteOff);
                    }
                }
            }
        }
    }

    // Sort events by tick
    std::sort(pendingEvents_.begin(), pendingEvents_.end(),
              [](const AudioEvent& a, const AudioEvent& b) { return a.tick < b.tick; });

    stats_.eventsScheduled = pendingEvents_.size();
}

void PlaybackEngine::generateControlEvents(uint32_t /*fromTick*/, uint32_t /*toTick*/) {
    if (!song_) return;

    // Generate control change events (volume, pan, reverb, etc.)
    // This is a simplified version - full implementation would extract
    // control changes from track automation data

    auto tracks = song_->getTracks();
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        auto track = tracks[trackIdx];
        if (!track) continue;

        // Check for track-level effects and convert to control events
        float reverb = track->getReverbAmount();
        if (reverb > 0.0f) {
            AudioEvent reverbEvent(
                0,  // fromTick would be used here
                AudioEventType::EFFECT_PARAM,
                trackIdx,
                0,  // Effect ID (reverb)
                static_cast<uint8_t>(reverb * 127.0f)
            );
            pendingEvents_.push_back(reverbEvent);
        }
    }
}

std::vector<AudioEvent> PlaybackEngine::getPendingEvents() {
    std::vector<AudioEvent> events;

    // Extract events that are ready to play
    auto it = pendingEvents_.begin();
    while (it != pendingEvents_.end()) {
        if (it->tick <= currentTick_) {
            events.push_back(*it);
            it = pendingEvents_.erase(it);
        } else {
            ++it;
        }
    }

    return events;
}

}  // namespace audio
