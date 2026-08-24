#include "audio/SynthesizerUI.hpp"
#include <algorithm>
#include <cmath>

namespace audio {

SynthesizerUI::SynthesizerUI(AudioDriver& driver, KeyboardController& keyboard)
    : audioDriver_(driver), keyboardController_(keyboard) {
}

void SynthesizerUI::initialize() {
    currentScreen_ = UIScreen::MAIN_MENU;
    selectedVoice_ = 0;
    selectedSID_ = 0;
    selectedParameter_ = 0;
}

void SynthesizerUI::shutdown() {
    statusMessage_.clear();
}

void SynthesizerUI::setScreen(UIScreen screen) {
    currentScreen_ = screen;
    selectedParameter_ = 0;
}

void SynthesizerUI::nextScreen() {
    uint8_t nextScreen = static_cast<uint8_t>(currentScreen_) + 1;
    if (nextScreen <= static_cast<uint8_t>(UIScreen::PERFORMANCE)) {
        currentScreen_ = static_cast<UIScreen>(nextScreen);
        selectedParameter_ = 0;
    }
}

void SynthesizerUI::previousScreen() {
    if (currentScreen_ != UIScreen::MAIN_MENU) {
        uint8_t prevScreen = static_cast<uint8_t>(currentScreen_) - 1;
        currentScreen_ = static_cast<UIScreen>(prevScreen);
        selectedParameter_ = 0;
    }
}

void SynthesizerUI::setSelectedVoice(uint8_t voiceId) {
    if (voiceId < 3) {
        selectedVoice_ = voiceId;
    }
}

void SynthesizerUI::adjustFrequency(int16_t delta) {
    int32_t newFreq = static_cast<int32_t>(displayFrequency_) + delta;
    newFreq = std::max(0, std::min(65535, static_cast<int>(newFreq)));
    displayFrequency_ = static_cast<uint16_t>(newFreq);

    // Apply to selected voice
    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setFrequency(displayFrequency_);
}

void SynthesizerUI::adjustWaveform(int8_t delta) {
    int16_t newWave = static_cast<int16_t>(displayWaveform_) + delta;
    displayWaveform_ = static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(newWave))));

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setWaveform(displayWaveform_);
}

void SynthesizerUI::adjustPulseWidth(int16_t delta) {
    int32_t newPW = static_cast<int32_t>(displayPulseWidth_) + delta;
    newPW = std::max(0, std::min(4095, static_cast<int>(newPW)));
    displayPulseWidth_ = static_cast<uint16_t>(newPW);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setPulseWidth(displayPulseWidth_);
}

void SynthesizerUI::adjustAttack(int8_t delta) {
    int16_t newAttack = static_cast<int16_t>(displayAttack_) + delta;
    displayAttack_ = clampAttack(newAttack);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setAttack(displayAttack_);
}

void SynthesizerUI::adjustDecay(int8_t delta) {
    int16_t newDecay = static_cast<int16_t>(displayDecay_) + delta;
    displayDecay_ = clampDecay(newDecay);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setDecay(displayDecay_);
}

void SynthesizerUI::adjustSustain(int8_t delta) {
    int16_t newSustain = static_cast<int16_t>(displaySustain_) + delta;
    displaySustain_ = clampSustain(newSustain);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setSustain(displaySustain_);
}

void SynthesizerUI::adjustRelease(int8_t delta) {
    int16_t newRelease = static_cast<int16_t>(displayRelease_) + delta;
    displayRelease_ = clampRelease(newRelease);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getVoice(selectedVoice_).setRelease(displayRelease_);
}

void SynthesizerUI::adjustFilterCutoff(int16_t delta) {
    int32_t newCutoff = static_cast<int32_t>(displayFilterCutoff_) + delta;
    displayFilterCutoff_ = clampCutoff(newCutoff);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getFilter().setCutoff(displayFilterCutoff_);
}

void SynthesizerUI::adjustFilterResonance(int8_t delta) {
    int16_t newRes = static_cast<int16_t>(displayFilterResonance_) + delta;
    displayFilterResonance_ = clampResonance(newRes);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getFilter().setResonance(displayFilterResonance_);
}

void SynthesizerUI::adjustFilterType(int8_t delta) {
    int16_t newType = static_cast<int16_t>(displayFilterType_) + delta;
    displayFilterType_ = static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(newType))));

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getFilter().setFilterType(displayFilterType_);
}

void SynthesizerUI::adjustFilterVolume(int8_t delta) {
    int16_t newVol = static_cast<int16_t>(displayFilterVolume_) + delta;
    displayFilterVolume_ = clampVolume(newVol);

    auto& sid = audioDriver_.getSID(selectedSID_);
    sid.getFilter().setVolume(displayFilterVolume_);
}

void SynthesizerUI::playSequence() {
    displayStatus("Playing...");
}

void SynthesizerUI::pauseSequence() {
    displayStatus("Paused");
}

void SynthesizerUI::stopSequence() {
    displayStatus("Stopped");
}

void SynthesizerUI::nextBar() {
    displayStatus("Next bar");
}

void SynthesizerUI::previousBar() {
    displayStatus("Previous bar");
}

void SynthesizerUI::adjustTempo(int16_t /*delta*/) {
    // Tempo control would integrate with PlaybackEngine
    // Typical range: 40-300 BPM
    displayStatus("Tempo adjusted");
}

void SynthesizerUI::render() {
    // Delegate to screen-specific renderers
    switch (currentScreen_) {
        case UIScreen::MAIN_MENU:
            renderMainMenu();
            break;
        case UIScreen::VOICE_CONTROL:
            renderVoiceControl();
            break;
        case UIScreen::ADSR_ENVELOPE:
            renderADSREnvelope();
            break;
        case UIScreen::FILTER_CONTROL:
            renderFilterControl();
            break;
        case UIScreen::SEQUENCER_CONTROL:
            renderSequencerControl();
            break;
        case UIScreen::PERFORMANCE:
            renderPerformance();
            break;
    }
}

void SynthesizerUI::displayStatus(const std::string& message) {
    statusMessage_ = message;
    statusMessageTime_ = 0;  // In real implementation, timestamp this
}

void SynthesizerUI::update() {
    // Update UI state based on input and time
    // Update synthesizer parameters based on control changes
    // Refresh display if needed
}

// Rendering methods (stubs for now - would interact with MEGA65 screen)
void SynthesizerUI::renderMainMenu() {
    clearScreen();
    drawText(10, 5, "=== MEGA65 Synthesizer ===");
    drawText(10, 10, "1. Voice Control");
    drawText(10, 12, "2. ADSR Envelope");
    drawText(10, 14, "3. Filter Control");
    drawText(10, 16, "4. Sequencer Control");
    drawText(10, 18, "5. Performance");
}

void SynthesizerUI::renderVoiceControl() {
    clearScreen();
    drawText(5, 2, "Voice Control - SID ");
    drawText(25, 2, std::to_string(selectedSID_));

    drawBox(5, 5, 70, 15);
    drawText(8, 7, "Frequency: 0x");
    drawText(8, 9, "Waveform: ");
    drawText(8, 11, "Pulse Width: 0x");
}

void SynthesizerUI::renderADSREnvelope() {
    clearScreen();
    drawText(5, 2, "ADSR Envelope Editor");

    drawBox(5, 5, 70, 18);
    drawText(8, 7, "Attack:  ");
    drawText(8, 9, "Decay:   ");
    drawText(8, 11, "Sustain: ");
    drawText(8, 13, "Release: ");
}

void SynthesizerUI::renderFilterControl() {
    clearScreen();
    drawText(5, 2, "Filter Control");

    drawBox(5, 5, 70, 18);
    drawText(8, 7, "Cutoff Frequency: 0x");
    drawText(8, 9, "Resonance (Q): ");
    drawText(8, 11, "Filter Type: ");
    drawText(8, 13, "Volume: ");
}

void SynthesizerUI::renderSequencerControl() {
    clearScreen();
    drawText(5, 2, "Sequencer Control");

    drawBox(5, 5, 70, 18);
    drawText(8, 7, "Status: Playing");
    drawText(8, 9, "Tempo: 120 BPM");
    drawText(8, 11, "Position: Bar 1");
}

void SynthesizerUI::renderPerformance() {
    clearScreen();
    drawText(5, 2, "Performance Metrics");

    drawBox(5, 5, 70, 18);
    drawText(8, 7, "Active Voices: 3");
    drawText(8, 9, "CPU Load: 45%");
    drawText(8, 11, "Memory Used: 24KB");
}

void SynthesizerUI::clearScreen() {
    // Would clear MEGA65 screen in real implementation
}

void SynthesizerUI::drawBox(uint8_t /*x*/, uint8_t /*y*/, uint8_t /*width*/, uint8_t /*height*/) {
    // Would draw box on MEGA65 screen
}

void SynthesizerUI::drawText(uint8_t /*x*/, uint8_t /*y*/, const std::string& /*text*/) {
    // Would draw text on MEGA65 screen
}

// Parameter range helpers
uint8_t SynthesizerUI::clampAttack(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

uint8_t SynthesizerUI::clampDecay(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

uint8_t SynthesizerUI::clampSustain(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

uint8_t SynthesizerUI::clampRelease(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

uint16_t SynthesizerUI::clampCutoff(int32_t value) const {
    return static_cast<uint16_t>(std::max(0, std::min(2047, static_cast<int>(value))));
}

uint8_t SynthesizerUI::clampResonance(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

uint8_t SynthesizerUI::clampVolume(int16_t value) const {
    return static_cast<uint8_t>(std::max(0, std::min(15, static_cast<int>(value))));
}

}  // namespace audio
