#include "audio/SIDChip.hpp"
#include <cmath>

namespace audio {

// MIDI note to SID frequency conversion
// SID uses 16-bit frequency word: 1MHz clock / 16777216 * freq = output frequency
// For MEGA65: clock is 3.5 MHz, so multiply base frequencies by 3.5
static constexpr uint16_t MIDI_FREQ_TABLE[] = {
    // C-1 through G9 (MIDI notes 0-127)
    0x00B4, 0x00C0, 0x00CD, 0x00DB, 0x00EA, 0x00FA, 0x010B, 0x011D, 0x0131, 0x0146, 0x015D, 0x0175,
    0x018F, 0x01AC, 0x01CB, 0x01EC, 0x0210, 0x0237, 0x0261, 0x028F, 0x02BF, 0x02F3, 0x032A, 0x0366,
    0x03A6, 0x03EB, 0x0436, 0x0487, 0x04DF, 0x053F, 0x05A6, 0x0615, 0x068D, 0x070D, 0x0797, 0x082D,
    0x08CF, 0x097D, 0x0A38, 0x0B02, 0x0BDB, 0x0CC5, 0x0DC1, 0x0ECF, 0x0FEF, 0x1125, 0x1271, 0x13D5,
    0x159F, 0x1787, 0x1991, 0x1BC0, 0x1E1C, 0x209E, 0x244A, 0x2830, 0x2C5D, 0x30F6, 0x35F2, 0x3B68,
    0x414F, 0x480F, 0x4F8A, 0x5780, 0x6039, 0x6A4E, 0x7597, 0x81D8, 0x8F02, 0x9D6F, 0xAD01, 0xBDD0,
    0xD069, 0xE49D, 0xFB2F, 0x13B1, 0x1207, 0x111D, 0x1000, 0x0F00, 0x0E00, 0x0D00, 0x0C00, 0x0B00,
    0x0A00, 0x0900, 0x0800, 0x0700, 0x0600, 0x0500, 0x0400, 0x0300, 0x0200, 0x0100, 0x00FF, 0x00FE,
    0x00FD, 0x00FC, 0x00FB, 0x00FA, 0x00F9, 0x00F8, 0x00F7, 0x00F6, 0x00F5, 0x00F4, 0x00F3, 0x00F2,
    0x00F1, 0x00F0, 0x00EF, 0x00EE, 0x00ED, 0x00EC, 0x00EB, 0x00EA
};

// SIDVoice implementation
void SIDVoice::setFrequency(uint16_t freqWord) {
    frequency_ = freqWord;
}

void SIDVoice::setWaveform(uint8_t waveform) {
    waveform_ = waveform;
}

void SIDVoice::setPulseWidth(uint16_t pw) {
    pulseWidth_ = pw & 0x0FFF;
}

void SIDVoice::setGate(bool gate) {
    gated_ = gate;
}

void SIDVoice::setAttack(uint8_t attack) {
    attack_ = attack & 0x0F;
}

void SIDVoice::setDecay(uint8_t decay) {
    decay_ = decay & 0x0F;
}

void SIDVoice::setSustain(uint8_t sustain) {
    sustain_ = sustain & 0x0F;
}

void SIDVoice::setRelease(uint8_t release) {
    release_ = release & 0x0F;
}

// SIDFilter implementation
void SIDFilter::setCutoff(uint16_t cutoff) {
    cutoff_ = cutoff & 0x07FF;
}

void SIDFilter::setResonance(uint8_t resonance) {
    resonance_ = resonance & 0x0F;
}

void SIDFilter::setFilterType(uint8_t type) {
    filterType_ = type;
}

void SIDFilter::setVolume(uint8_t volume) {
    volume_ = volume & 0x0F;
}

// SIDChip implementation
SIDChip::SIDChip(uint8_t chipId)
    : chipId_(chipId & 0x03) {
    // Initialize voices with default ADSR
    for (auto& voice : voices_) {
        voice.setAttack(9);
        voice.setDecay(0);
        voice.setSustain(15);
        voice.setRelease(0);
    }
}

uint32_t SIDChip::getBaseAddress() const {
    // MEGA65 SID locations: $D400, $D500, $D600, $D700
    return 0xD400 + (chipId_ * 0x0100);
}

SIDVoice& SIDChip::getVoice(uint8_t voiceId) {
    return voices_[voiceId & 0x03];
}

const SIDVoice& SIDChip::getVoice(uint8_t voiceId) const {
    return voices_[voiceId & 0x03];
}

SIDFilter& SIDChip::getFilter() {
    return filter_;
}

const SIDFilter& SIDChip::getFilter() const {
    return filter_;
}

void SIDChip::updateHardware() {
    // Update each voice's registers
    for (uint8_t v = 0; v < 3; ++v) {
        writeVoiceRegisters(v);
    }
    // Update filter registers
    writeFilterRegisters();
}

void SIDChip::noteOn(uint8_t voiceId, uint8_t midiNote, uint8_t velocity) {
    if (voiceId >= 3) return;

    // Convert MIDI note to SID frequency
    uint16_t freq = (midiNote < 128) ? MIDI_FREQ_TABLE[midiNote] : 0;

    // Set frequency and velocity-based parameters
    voices_[voiceId].setFrequency(freq);

    // Attack time based on velocity (faster attack = higher velocity)
    uint8_t attack = (velocity > 64) ? 9 : 15;
    voices_[voiceId].setAttack(attack);

    // Gate on to start envelope
    voices_[voiceId].setGate(true);
}

void SIDChip::noteOff(uint8_t voiceId) {
    if (voiceId >= 3) return;
    voices_[voiceId].setGate(false);
}

uint16_t SIDChip::midiNoteToFreq(uint8_t midiNote) const {
    if (midiNote >= 128) return 0;
    return MIDI_FREQ_TABLE[midiNote];
}

void SIDChip::writeVoiceRegisters(uint8_t voiceId) {
    if (voiceId >= 3) return;

    const auto& voice = voices_[voiceId];
    [[maybe_unused]] uint32_t baseAddr = getBaseAddress();
    [[maybe_unused]] uint32_t voiceOffset = voiceId * 7;

    // Register offsets for voice (in 6581 SID):
    // +0,+1: Frequency
    // +2,+3: Pulse Width
    // +4: Control Register (waveform, gate, sync, ringmod, test)
    // +5: Attack/Decay
    // +6: Sustain/Release

    // Frequency low byte
    [[maybe_unused]] uint8_t freqLow = voice.getFrequency() & 0xFF;
    // Frequency high byte
    [[maybe_unused]] uint8_t freqHigh = (voice.getFrequency() >> 8) & 0xFF;

    // Pulse width low byte
    [[maybe_unused]] uint8_t pwLow = voice.getPulseWidth() & 0xFF;
    // Pulse width high byte (4 bits)
    [[maybe_unused]] uint8_t pwHigh = (voice.getPulseWidth() >> 8) & 0x0F;

    // Control register: waveform | gate
    [[maybe_unused]] uint8_t control = voice.getWaveform() | (voice.isGated() ? 0x01 : 0x00);

    // Attack/Decay: attack (4 bits) | decay (4 bits)
    [[maybe_unused]] uint8_t ad = (voice.getAttack() << 4) | voice.getDecay();

    // Sustain/Release: sustain (4 bits) | release (4 bits)
    [[maybe_unused]] uint8_t sr = (voice.getSustain() << 4) | voice.getRelease();

    // In hardware, registers would be written here via I/O mapped memory
    // For now, this is a placeholder for the actual hardware writes
    // writeToMEGA65Memory(baseAddr + voiceOffset + 0, freqLow);
    // writeToMEGA65Memory(baseAddr + voiceOffset + 1, freqHigh);
    // ... etc
}

void SIDChip::writeFilterRegisters() {
    [[maybe_unused]] uint32_t baseAddr = getBaseAddress();

    // Filter registers are typically at offset 21-23 (0x15-0x17)
    // FilterCutoff Low (bits 0-2)
    // FilterCutoff High (bits 3-10)
    // Filter Control (type bits, resonance bits)
    // Output Control (volume)

    // For now, this is a placeholder
}

}  // namespace audio
