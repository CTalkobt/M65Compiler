#include "audio/AudioDriver.hpp"
#include <algorithm>

namespace audio {

AudioDriver::AudioDriver() {
    // Initialize all 4 SID chips
    for (uint8_t i = 0; i < 4; ++i) {
        sidChips_[i] = std::make_unique<SIDChip>(i);
    }

    // Initialize track routes
    for (uint8_t i = 0; i < 16; ++i) {
        TrackRoute route;
        route.output = AudioOutput::SID_CHIP_0;
        route.voiceId = i % 3;
        route.channel = i + 1;
        trackRoutes_[i] = route;
    }

    // Initialize voice pool
    for (uint8_t i = 0; i < 64; ++i) {
        voicePool_[i].sidChipId = i / 3;
        voicePool_[i].voiceId = i % 3;
        voicePool_[i].allocated = false;
    }
}

bool AudioDriver::initialize() {
    if (initialized_) return true;

    initializeSIDChips();
    initializeRouting();

    initialized_ = true;
    return true;
}

void AudioDriver::shutdown() {
    if (!initialized_) return;

    // Silence all voices
    for (uint8_t c = 0; c < 4; ++c) {
        auto& sid = *sidChips_[c];
        for (uint8_t v = 0; v < 3; ++v) {
            sid.noteOff(v);
        }
        sid.updateHardware();
    }

    initialized_ = false;
}

SIDChip& AudioDriver::getSID(uint8_t chipId) {
    return *sidChips_[chipId & 0x03];
}

const SIDChip& AudioDriver::getSID(uint8_t chipId) const {
    return *sidChips_[chipId & 0x03];
}

void AudioDriver::configureTrackRoute(uint8_t trackId, const TrackRoute& route) {
    if (trackId >= 16) return;
    trackRoutes_[trackId] = route;
}

const TrackRoute& AudioDriver::getTrackRoute(uint8_t trackId) const {
    static const TrackRoute invalid = {AudioOutput::SID_CHIP_0, 0, 1, false};
    if (trackId >= 16) return invalid;
    return trackRoutes_[trackId];
}

void AudioDriver::attachPlaybackEngine(PlaybackEngine* engine) {
    playbackEngine_ = engine;
}

void AudioDriver::detachPlaybackEngine() {
    playbackEngine_ = nullptr;
}

void AudioDriver::handleAudioEvent(const AudioEvent& event) {
    if (!initialized_) return;

    switch (event.type) {
        case AudioEventType::NOTE_ON:
            handleNoteOn(event);
            break;
        case AudioEventType::NOTE_OFF:
            handleNoteOff(event);
            break;
        case AudioEventType::CONTROL_CHANGE:
            handleControlChange(event);
            break;
        case AudioEventType::TEMPO_CHANGE:
            handleTempoChange(event);
            break;
        default:
            break;
    }
}

void AudioDriver::updateHardware() {
    if (!initialized_) return;

    for (uint8_t c = 0; c < 4; ++c) {
        sidChips_[c]->updateHardware();
    }
}

uint32_t AudioDriver::getActiveSIDChips() const {
    uint32_t activeMask = 0;
    for (uint8_t c = 0; c < 4; ++c) {
        // Check if any voice in this SID chip is active
        for (uint8_t v = 0; v < 3; ++v) {
            if (sidChips_[c]->getVoice(v).isGated()) {
                activeMask |= (1 << c);
                break;
            }
        }
    }
    return activeMask;
}

void AudioDriver::setMasterVolume(uint8_t volume) {
    masterVolume_ = std::min(volume, static_cast<uint8_t>(127));

    // Apply to all SID chips
    for (uint8_t c = 0; c < 4; ++c) {
        uint8_t chipVolume = (masterVolume_ * 15) / 127;  // Scale to 0-15 for SID
        sidChips_[c]->getFilter().setVolume(chipVolume);
    }
}

void AudioDriver::mute() {
    muted_ = true;
    setMasterVolume(0);
}

void AudioDriver::unmute() {
    muted_ = false;
    setMasterVolume(masterVolume_);
}

uint8_t AudioDriver::allocateVoice(uint8_t sidChipId) {
    for (uint8_t i = 0; i < 64; ++i) {
        if (voicePool_[i].sidChipId == (sidChipId & 0x03) && !voicePool_[i].allocated) {
            voicePool_[i].allocated = true;
            return i;
        }
    }
    return 0xFF;  // No voice available
}

void AudioDriver::deallocateVoice(uint8_t voiceAllocId) {
    if (voiceAllocId < 64) {
        voicePool_[voiceAllocId].allocated = false;
    }
}

void AudioDriver::handleNoteOn(const AudioEvent& event) {
    // Route to appropriate SID chip and voice
    const auto& route = trackRoutes_[event.trackId];
    if (!route.enabled) return;

    if (route.output != AudioOutput::DIGI_AUDIO) {
        uint8_t chipId = static_cast<uint8_t>(route.output);
        auto& sid = *sidChips_[chipId];
        sid.noteOn(route.voiceId, event.param1, event.param2);
    }
}

void AudioDriver::handleNoteOff(const AudioEvent& event) {
    // Route to appropriate SID chip and voice
    const auto& route = trackRoutes_[event.trackId];
    if (!route.enabled) return;

    if (route.output != AudioOutput::DIGI_AUDIO) {
        uint8_t chipId = static_cast<uint8_t>(route.output);
        auto& sid = *sidChips_[chipId];
        sid.noteOff(route.voiceId);
    }
}

void AudioDriver::handleControlChange(const AudioEvent& event) {
    // CC handling (modulation, volume, pan, etc.)
    // For now, just handle volume CC
    if (event.param1 == 7) {  // Volume control
        const auto& route = trackRoutes_[event.trackId];
        if (route.enabled && route.output != AudioOutput::DIGI_AUDIO) {
            uint8_t chipId = static_cast<uint8_t>(route.output);
            auto& sid = *sidChips_[chipId];
            // Scale MIDI velocity (0-127) to SID volume (0-15)
            uint8_t volume = (event.param2 * 15) / 127;
            sid.getFilter().setVolume(volume);
        }
    }
}

void AudioDriver::handleTempoChange(const AudioEvent& /*event*/) {
    // Tempo changes handled by playback engine
    // This is informational only for audio driver
}

void AudioDriver::initializeSIDChips() {
    for (uint8_t c = 0; c < 4; ++c) {
        auto& sid = *sidChips_[c];

        // Initialize voices
        for (uint8_t v = 0; v < 3; ++v) {
            auto& voice = sid.getVoice(v);
            voice.setFrequency(0);
            voice.setWaveform(0);
            voice.setGate(false);
            voice.setAttack(9);
            voice.setDecay(0);
            voice.setSustain(15);
            voice.setRelease(0);
        }

        // Initialize filter
        auto& filter = sid.getFilter();
        filter.setCutoff(0);
        filter.setResonance(0);
        filter.setFilterType(0);
        filter.setVolume(15);
    }
}

void AudioDriver::initializeRouting() {
    // Default routing: distribute tracks across SID chips
    for (uint8_t t = 0; t < 16; ++t) {
        TrackRoute route;
        route.output = AudioOutput(t / 5);  // Distribute across chips
        route.voiceId = t % 3;
        route.channel = t + 1;
        route.enabled = true;
        trackRoutes_[t] = route;
    }
}

void AudioDriver::writeToMEGA65Memory(uint32_t /*address*/, uint8_t /*value*/) {
    // This would be implemented with actual MEGA65 I/O writes
    // For now, it's a stub for hardware abstraction
    // In real hardware: volatile uint8_t* ptr = (volatile uint8_t*)address; *ptr = value;
}

uint8_t AudioDriver::readFromMEGA65Memory(uint32_t /*address*/) const {
    // This would be implemented with actual MEGA65 I/O reads
    // For now, it's a stub for hardware abstraction
    return 0;
}

}  // namespace audio
