// TuneAudio.cpp — four reSIDfp chips and the sound's way out (see TuneAudio.hpp)
#include "TuneAudio.hpp"
#include "residfp/residfp.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unistd.h>

struct TuneAudio::Chips {
    reSIDfp::residfp sid[4];
};

TuneAudio::TuneAudio(bool ntsc, bool mos6581, int rate) : chips_(new Chips), rate_(rate) {
    double clock = ntsc ? 1022727.0 : 985248.0;   // the SID clock (as the MEGA65's)
    cyclesPerFrame_ = clock / (ntsc ? 60.0 : 50.0);
    for (auto& s : chips_->sid) {
        s.setChipModel(mos6581 ? reSIDfp::MOS6581 : reSIDfp::CSG8580);
        s.setSamplingParameters(clock, reSIDfp::RESAMPLE, rate);
        s.reset();
    }
}

TuneAudio::~TuneAudio() = default;

void TuneAudio::write(uint8_t index, uint8_t value) {
    chips_->sid[(index >> 5) & 3].write(index & 0x1F, value);
}

void TuneAudio::frame(std::vector<int16_t>& stereo) {
    carry_ += cyclesPerFrame_;
    unsigned cycles = (unsigned)carry_;
    carry_ -= cycles;
    size_t room = (size_t)std::ceil(cycles / 950000.0 * rate_) + 64;
    std::vector<short> buf[4];
    int n[4];
    for (int c = 0; c < 4; c++) {
        buf[c].resize(room);
        n[c] = chips_->sid[c].clock(cycles, buf[c].data());
    }
    int m = n[0];
    for (int c = 1; c < 4; c++) if (n[c] < m) m = n[c];
    for (int k = 0; k < m; k++) {
        stereo.push_back((int16_t)(((int)buf[0][k] + buf[1][k]) / 2));
        stereo.push_back((int16_t)(((int)buf[2][k] + buf[3][k]) / 2));
    }
}

struct SidChips::Chips {
    reSIDfp::residfp sid[3];
};

SidChips::SidChips(const std::vector<bool>& mos6581, double clock, int rate)
    : chips_(new Chips), n_((int)std::max<size_t>(1, std::min<size_t>(3, mos6581.size()))), rate_(rate), clock_(clock) {
    for (int c = 0; c < n_; c++) {
        auto& s = chips_->sid[c];
        s.setChipModel(mos6581[c] ? reSIDfp::MOS6581 : reSIDfp::CSG8580);
        s.setSamplingParameters(clock, reSIDfp::RESAMPLE, rate);
        s.reset();
    }
}

SidChips::~SidChips() = default;

void SidChips::write(int chip, uint8_t reg, uint8_t value) {
    if (chip >= 0 && chip < n_) chips_->sid[chip].write(reg & 0x1F, value);
}

void SidChips::clock(uint32_t cycles, bool mono, std::vector<int16_t>& stereo) {
    while (cycles) {
        unsigned step = std::min<uint32_t>(cycles, 20000);   // (a buffer's worth at a time)
        cycles -= step;
        size_t room = (size_t)std::ceil(step / clock_ * rate_) + 64;
        std::vector<short> buf[3];
        int n[3] = {0, 0, 0};
        for (int c = 0; c < n_; c++) {
            buf[c].resize(room);
            n[c] = chips_->sid[c].clock(step, buf[c].data());
        }
        int m = n[0];
        for (int c = 1; c < n_; c++) if (n[c] < m) m = n[c];
        for (int k = 0; k < m; k++) {
            int l, r;
            if (n_ == 1) l = r = buf[0][k];
            else if (mono) {
                int sum = 0;
                for (int c = 0; c < n_; c++) sum += buf[c][k];
                l = r = sum / n_;
            } else if (n_ == 2) { l = buf[0][k]; r = buf[1][k]; }
            else { l = (buf[0][k] + buf[2][k]) / 2; r = (buf[1][k] + buf[2][k]) / 2; }
            stereo.push_back((int16_t)l);
            stereo.push_back((int16_t)r);
        }
    }
}

namespace {
bool onPath(const std::string& prog) {
    const char* path = getenv("PATH");
    if (!path) return false;
    std::string p = path;
    size_t start = 0;
    while (start <= p.size()) {
        size_t end = p.find(':', start);
        std::string dir = p.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!dir.empty() && access((dir + "/" + prog).c_str(), X_OK) == 0) return true;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return false;
}

void le32(FILE* f, uint32_t v) { for (int k = 0; k < 4; k++) fputc((int)((v >> (8 * k)) & 0xFF), f); }
void le16(FILE* f, uint16_t v) { fputc(v & 0xFF, f); fputc(v >> 8, f); }
}  // namespace

bool AudioSink::open(const std::string& wav, int rate) {
    rate_ = rate;
    if (!wav.empty()) {
        file_ = fopen(wav.c_str(), "wb");
        if (!file_) { error_ = "cannot write " + wav; return false; }
        for (int k = 0; k < 44; k++) fputc(0, file_);   // (the header, at close)
        via_ = wav;
        return true;
    }
    std::string r = std::to_string(rate);
    struct { const char* prog; std::string cmd; } players[] = {
        {"pw-play", "pw-play --rate " + r + " --channels 2 --format s16 -"},
        {"paplay", "paplay --raw --rate=" + r + " --channels=2 --format=s16le"},
        {"aplay", "aplay -q -t raw -f S16_LE -r " + r + " -c 2"},
    };
    for (const auto& p : players)
        if (onPath(p.prog)) {
            pipe_ = popen(p.cmd.c_str(), "w");
            if (pipe_) { via_ = p.prog; return true; }
        }
    error_ = "no audio player found (pw-play, paplay or aplay): use --wav <file>";
    return false;
}

void AudioSink::put(const std::vector<int16_t>& stereo) {
    FILE* f = pipe_ ? pipe_ : file_;
    if (!f || stereo.empty()) return;
    for (int16_t s : stereo) le16(f, (uint16_t)s);   // (little-endian whatever the host)
    samples_ += (uint32_t)(stereo.size() / 2);
}

void AudioSink::close() {
    if (pipe_) { pclose(pipe_); pipe_ = nullptr; }
    if (file_) {
        uint32_t data = samples_ * 4;
        fseek(file_, 0, SEEK_SET);
        fwrite("RIFF", 1, 4, file_); le32(file_, 36 + data); fwrite("WAVEfmt ", 1, 8, file_);
        le32(file_, 16); le16(file_, 1); le16(file_, 2); le32(file_, (uint32_t)rate_);
        le32(file_, (uint32_t)rate_ * 4); le16(file_, 4); le16(file_, 16);
        fwrite("data", 1, 4, file_); le32(file_, data);
        fclose(file_);
        file_ = nullptr;
    }
}
