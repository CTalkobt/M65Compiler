#pragma once

#include <cstdint>
#include <string>
#include <array>
#include "audio/AudioDriver.hpp"
#include "audio/KeyboardController.hpp"

namespace audio {

// Synthesizer UI screens
enum class UIScreen : uint8_t {
    MAIN_MENU = 0,
    VOICE_CONTROL = 1,
    ADSR_ENVELOPE = 2,
    FILTER_CONTROL = 3,
    SEQUENCER_CONTROL = 4,
    PERFORMANCE = 5
};

// Real-time parameter control for live synthesis
class SynthesizerUI {
public:
    SynthesizerUI(AudioDriver& driver, KeyboardController& keyboard);
    ~SynthesizerUI() = default;

    // Initialization
    void initialize();
    void shutdown();

    // Screen navigation
    void setScreen(UIScreen screen);
    UIScreen getCurrentScreen() const { return currentScreen_; }
    void nextScreen();
    void previousScreen();

    // Parameter control
    void setSelectedVoice(uint8_t voiceId);  // 0-2 for 3 voices per SID
    void setSelectedSID(uint8_t chipId);     // 0-3 for 4 SID chips
    uint8_t getSelectedVoice() const { return selectedVoice_; }
    uint8_t getSelectedSID() const { return selectedSID_; }

    // Parameter adjustment methods
    void adjustFrequency(int16_t delta);
    void adjustWaveform(int8_t delta);
    void adjustPulseWidth(int16_t delta);

    // ADSR control
    void adjustAttack(int8_t delta);
    void adjustDecay(int8_t delta);
    void adjustSustain(int8_t delta);
    void adjustRelease(int8_t delta);

    // Filter control
    void adjustFilterCutoff(int16_t delta);
    void adjustFilterResonance(int8_t delta);
    void adjustFilterType(int8_t delta);
    void adjustFilterVolume(int8_t delta);

    // Sequencer/playback control
    void playSequence();
    void pauseSequence();
    void stopSequence();
    void nextBar();
    void previousBar();
    void adjustTempo(int16_t delta);

    // Display/rendering
    void render();
    void displayStatus(const std::string& message);

    // Update
    void update();

private:
    AudioDriver& audioDriver_;
    KeyboardController& keyboardController_;

    UIScreen currentScreen_ = UIScreen::MAIN_MENU;
    uint8_t selectedVoice_ = 0;
    uint8_t selectedSID_ = 0;

    // Current parameter values (for display)
    uint16_t displayFrequency_ = 0;
    uint8_t displayWaveform_ = 0;
    uint16_t displayPulseWidth_ = 0x800;
    uint8_t displayAttack_ = 9;
    uint8_t displayDecay_ = 0;
    uint8_t displaySustain_ = 15;
    uint8_t displayRelease_ = 0;
    uint16_t displayFilterCutoff_ = 0;
    uint8_t displayFilterResonance_ = 0;
    uint8_t displayFilterType_ = 0;
    uint8_t displayFilterVolume_ = 15;

    // Status message
    std::string statusMessage_;
    uint32_t statusMessageTime_ = 0;

    // Cursor position for parameter selection
    uint8_t selectedParameter_ = 0;

    // Helper methods
    void renderMainMenu();
    void renderVoiceControl();
    void renderADSREnvelope();
    void renderFilterControl();
    void renderSequencerControl();
    void renderPerformance();

    void clearScreen();
    void drawBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
    void drawText(uint8_t x, uint8_t y, const std::string& text);

    // Parameter range helpers
    uint8_t clampAttack(int16_t value) const;
    uint8_t clampDecay(int16_t value) const;
    uint8_t clampSustain(int16_t value) const;
    uint8_t clampRelease(int16_t value) const;
    uint16_t clampCutoff(int32_t value) const;
    uint8_t clampResonance(int16_t value) const;
    uint8_t clampVolume(int16_t value) const;
};

}  // namespace audio
