#pragma once

#include <cstdint>
#include <array>

namespace audio {

// SID Chip voice (oscillator + ADSR envelope + filter)
class SIDVoice {
public:
    // Voice control
    void setFrequency(uint16_t freqWord);  // Frequency for note (0x0000-0xFFFF)
    void setWaveform(uint8_t waveform);    // Bit pattern: bit7=Noise, bit6=Pulse, bit5=Sawtooth, bit4=Triangle
    void setPulseWidth(uint16_t pw);       // Pulse width (0x0000-0x0FFF)
    void setGate(bool gate);               // Gate on/off (start/release envelope)

    // ADSR envelope
    void setAttack(uint8_t attack);        // 0-15 (attack time in milliseconds)
    void setDecay(uint8_t decay);          // 0-15 (decay time)
    void setSustain(uint8_t sustain);      // 0-15 (sustain level)
    void setRelease(uint8_t release);      // 0-15 (release time)

    // Getters
    uint16_t getFrequency() const { return frequency_; }
    uint8_t getWaveform() const { return waveform_; }
    uint16_t getPulseWidth() const { return pulseWidth_; }
    bool isGated() const { return gated_; }
    uint8_t getAttack() const { return attack_; }
    uint8_t getDecay() const { return decay_; }
    uint8_t getSustain() const { return sustain_; }
    uint8_t getRelease() const { return release_; }

private:
    uint16_t frequency_ = 0;
    uint8_t waveform_ = 0;
    uint16_t pulseWidth_ = 0x0800;
    bool gated_ = false;
    uint8_t attack_ = 9;
    uint8_t decay_ = 0;
    uint8_t sustain_ = 15;
    uint8_t release_ = 0;
};

// SID Chip filter
class SIDFilter {
public:
    // Filter control
    void setCutoff(uint16_t cutoff);        // Cutoff frequency (0x0000-0x07FF)
    void setResonance(uint8_t resonance);   // Resonance (0-15)
    void setFilterType(uint8_t type);       // Bit pattern: bit7=Off-Voice3, bit6=BP-Voice3, bit5=LP, bit4=HP, bit3=BP
    void setVolume(uint8_t volume);         // Master volume (0-15)

    uint16_t getCutoff() const { return cutoff_; }
    uint8_t getResonance() const { return resonance_; }
    uint8_t getFilterType() const { return filterType_; }
    uint8_t getVolume() const { return volume_; }

private:
    uint16_t cutoff_ = 0;
    uint8_t resonance_ = 0;
    uint8_t filterType_ = 0;
    uint8_t volume_ = 15;
};

// Full SID Chip (6581 or 8580 compatible)
class SIDChip {
public:
    SIDChip(uint8_t chipId);
    ~SIDChip() = default;

    // Chip identification
    uint8_t getChipId() const { return chipId_; }
    uint32_t getBaseAddress() const;

    // Voice access (3 voices per SID)
    SIDVoice& getVoice(uint8_t voiceId);
    const SIDVoice& getVoice(uint8_t voiceId) const;

    // Filter access
    SIDFilter& getFilter();
    const SIDFilter& getFilter() const;

    // Write to hardware registers
    void updateHardware();

    // Note playback
    void noteOn(uint8_t voiceId, uint8_t midiNote, uint8_t velocity = 127);
    void noteOff(uint8_t voiceId);

    // MIDI to frequency conversion (public for testing)
    uint16_t midiNoteToFreq(uint8_t midiNote) const;

private:
    uint8_t chipId_;  // 0-3 for 4 SID chips
    std::array<SIDVoice, 3> voices_;
    SIDFilter filter_;

    // Hardware register writes
    void writeVoiceRegisters(uint8_t voiceId);
    void writeFilterRegisters();
};

}  // namespace audio
