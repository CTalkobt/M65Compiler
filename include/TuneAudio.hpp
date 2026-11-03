// TuneAudio.hpp — tune45 play's sound: the MEGA65's four SIDs as reSIDfp
// chips (third_party/libresidfp), SIDs 1-2 left, 3-4 right, rendered a
// frame at a time to 16-bit stereo; and where it goes (a player program on
// the PATH, or a WAV file)
#pragma once
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

class TuneAudio {
public:
    TuneAudio(bool ntsc, bool mos6581, int rate = 48000);
    ~TuneAudio();
    void write(uint8_t index, uint8_t value);       // chip * $20 + register
    void frame(std::vector<int16_t>& stereo);       // a video frame, appended L, R, L, R...
    int rate() const { return rate_; }

private:
    struct Chips;
    std::unique_ptr<Chips> chips_;
    int rate_;
    double cyclesPerFrame_, carry_ = 0;
};

// A C64 tune's SIDs (tune45 play of a .sid): 1-3 reSIDfp chips at the
// C64's clock, each written at the cycle the tune wrote it. Mixed: one SID
// on both sides; two the first left, the second right; three the first
// left, the second right, the third on both (mono: all on both)
class SidChips {
public:
    // models: true the 6581, per chip; clock 985248 PAL, 1022727 NTSC
    SidChips(const std::vector<bool>& mos6581, double clock, int rate = 48000);
    ~SidChips();
    void write(int chip, uint8_t reg, uint8_t value);
    void clock(uint32_t cycles, bool mono, std::vector<int16_t>& stereo);   // appends L, R, L, R...
    int rate() const { return rate_; }
    int chips() const { return n_; }

private:
    struct Chips;
    std::unique_ptr<Chips> chips_;
    int n_, rate_;
    double clock_;
};

// Where the samples go: a raw-audio player on the PATH (pw-play, paplay,
// aplay), or a WAV file
class AudioSink {
public:
    // "" (a player) or a .wav path; false with error() set
    bool open(const std::string& wav, int rate);
    void put(const std::vector<int16_t>& stereo);
    void close();
    const std::string& error() const { return error_; }
    const std::string& via() const { return via_; }

private:
    FILE* pipe_ = nullptr;
    FILE* file_ = nullptr;
    std::string error_, via_;
    uint32_t samples_ = 0;
    int rate_ = 48000;
};
