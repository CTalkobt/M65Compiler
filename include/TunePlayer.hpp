// TunePlayer.hpp — tune45's model of the library's tune player
// (lib/stdlib_asm/tune.s45): the same SID register writes, frame by frame.
// tune45 play feeds them to reSIDfp; the tests compare them with the real
// player's in xemu, so the two must stay the same.
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

class TunePlayer {
public:
    // A register write: index chip * $20 + register (0-127), the value
    using Write = std::function<void(uint8_t index, uint8_t value)>;

    explicit TunePlayer(Write w) : write_(std::move(w)) {}

    void setNtsc(bool ntsc) { ntsc_ = ntsc; }
    void setMirror(bool m) { mirror_ = m; }
    void setFirstVoice(uint8_t first) { first_ = first > 11 ? 11 : first; }
    void setTempo(uint16_t scale) { scale_ = scale; rate(); }
    void setVibrato(uint8_t extra) { vibx_ = extra; }
    void setTranspose(int voice, int8_t s) { if (voice >= 0 && voice < 12) transp_[voice] = s; }
    void setDetune(int voice, int8_t a) { if (voice >= 0 && voice < 12) detune_[voice] = a; }

    void play(const std::vector<uint8_t>& tune);
    void stop();
    void update();          // a frame
    void seek(unsigned ticks);   // on, silently
    bool playing() const { return active_; }
    unsigned ticks() const { return ticks_; }

private:
    struct Voice {
        uint16_t pos = 0, start = 0;
        uint8_t left = 0, gated = 0, done = 0, note = 0, t = 0, base = 0;
        uint16_t freq = 0, pw = 0;
        int8_t pwdir = 0;
        uint8_t arpa = 0, arpb = 0, arpi = 0;
        uint8_t wave = 0, ad = 0, sr = 0, vib = 0, gap = 0, vdel = 0;
        uint16_t ipw = 0;
        int8_t pwm = 0, slide = 0;
        int8_t gsemi = 0;
        uint8_t gpend = 0, glide = 0;
        uint16_t tgt = 0;
    };
    Write write_;
    std::vector<uint8_t> tune_;
    Voice v_[12];
    int8_t transp_[12] = {}, detune_[12] = {};
    bool ntsc_ = false, mirror_ = false, active_ = false, mute_ = false;
    uint8_t first_ = 0, vibx_ = 0, n_ = 0, ninst_ = 0;
    uint16_t scale_ = 0x100, step_ = 0x100, fpt_ = 0x100, acc_ = 0, ins_ = 0;
    unsigned ticks_ = 0;

    uint8_t byte(unsigned at) const { return at < tune_.size() ? tune_[at] : 0xFF; }
    uint8_t rd(Voice& v) { return byte(v.pos++); }
    void put(uint8_t idx, uint8_t val);
    void w(const Voice& v, uint8_t reg, uint8_t val) { put((uint8_t)(v.base + reg), val); }
    void ctrl(const Voice& v) { w(v, 4, (uint8_t)((v.wave & 0xFE) | v.gated)); }
    void wf(const Voice& v, uint16_t f) { w(v, 0, (uint8_t)f); w(v, 1, (uint8_t)(f >> 8)); }
    void wpw(const Voice& v) { w(v, 2, (uint8_t)v.pw); w(v, 3, (uint8_t)(v.pw >> 8)); }
    void inst(Voice& v, uint8_t i);
    uint16_t freq(uint8_t note) const;
    uint16_t notef(const Voice& v, int idx, uint8_t add) const;
    void next(Voice& v, int idx);
    void tick();
    void effects();
    void rate();
    void silence();
};
