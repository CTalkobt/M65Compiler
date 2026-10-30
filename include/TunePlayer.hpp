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

    // The digi voices' audio DMA writes: index from $D710 ($D711 the
    // enable, $D71C + ch the other side's volume, $D720 + ch * 16 + r)
    void setDigiWrite(Write w) { dwrite_ = std::move(w); }
    void setSamples(uint32_t addr) { sbank_ = addr & 0xFFFFFF; }   // tune_samples
    void setDigiChannels(uint8_t first) { dfirst_ = first > 3 ? 3 : first; }
    void setDigiVolume(uint8_t v) { dvol_ = v; }

    void setNtsc(bool ntsc) { ntsc_ = ntsc; }
    void setMirror(bool m) { mirror_ = m; }
    void setFirstVoice(uint8_t first) { first_ = first > 11 ? 11 : first; }
    void setTempo(uint16_t scale) { scale_ = scale; rate(); }
    void setVibrato(uint8_t extra) { vibx_ = extra; }
    void setTranspose(int voice, int8_t s) { if (voice >= 0 && voice < 12) transp_[voice] = s; }
    void setDetune(int voice, int8_t a) { if (voice >= 0 && voice < 12) detune_[voice] = a; }
    // a register as the program left it (sid_init's volume in $18): the
    // filter keeps the bits of $17 and $18 it does not set
    void setShadow(uint8_t idx, uint8_t val) { shadow_[idx & 0x7F] = val; }

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
        uint8_t dvol = 255;             // a digi voice's volume (vol:)
        uint16_t freq = 0, pw = 0;
        int8_t pwdir = 0;
        uint8_t arpa = 0, arpb = 0, arpi = 0;
        uint8_t wave = 0, ad = 0, sr = 0, vib = 0, gap = 0, vdel = 0;
        uint16_t ipw = 0;
        int8_t pwm = 0, slide = 0;
        int8_t gsemi = 0;
        uint8_t gpend = 0, glide = 0;
        uint16_t tgt = 0;
        // the instrument's hard restart, filter and wave table
        uint8_t hr = 0, filt = 0;
        int8_t fsweep = 0;
        uint16_t cutoff = 0, table = 0;
        // its control register's waveform now, the table's step and pitch,
        // the hard restart done, its bit in the filter routing ($17)
        uint8_t cw = 0, wti = 0, hrdone = 0, fbit = 0;
        int8_t wsemi = 0;
    };
    Write write_, dwrite_;
    std::vector<uint8_t> tune_;
    Voice v_[16];                     // 12-15: the digi voices
    int8_t transp_[12] = {}, detune_[12] = {};
    bool ntsc_ = false, mirror_ = false, active_ = false, mute_ = false;
    uint8_t first_ = 0, vibx_ = 0, n_ = 0, ninst_ = 0;
    uint16_t scale_ = 0x100, step_ = 0x100, fpt_ = 0x100, acc_ = 0, ins_ = 0;
    uint8_t rsize_ = 10, soon_ = 4;   // an instrument's bytes; frames to the next tick (1-4)
    uint16_t cut_[4] = {};            // each chip's filter cutoff
    int8_t csw_[4] = {};              //   and its sweep a frame
    uint8_t shadow_[128] = {};
    uint8_t nd_ = 0, dfirst_ = 0, dvol_ = 255;   // digi voices; the first channel; volume
    uint16_t stab_ = 0;               // the sample table
    bool e9_ = false;                 // its entries 9 bytes (a loop's), else 7
    uint32_t sbank_ = 0;              // the samples' address
    unsigned ticks_ = 0;

    uint8_t byte(unsigned at) const { return at < tune_.size() ? tune_[at] : 0xFF; }
    uint8_t rd(Voice& v) { return byte(v.pos++); }
    void put(uint8_t idx, uint8_t val);
    void w(const Voice& v, uint8_t reg, uint8_t val) { put((uint8_t)(v.base + reg), val); }
    void ctrl(const Voice& v) { w(v, 4, (uint8_t)((v.cw & 0xFE) | v.gated)); }
    void wf(const Voice& v, uint16_t f) { w(v, 0, (uint8_t)f); w(v, 1, (uint8_t)(f >> 8)); }
    void wpw(const Voice& v) { w(v, 2, (uint8_t)v.pw); w(v, 3, (uint8_t)(v.pw >> 8)); }
    void inst(Voice& v, uint8_t i);
    void filter(Voice& v);
    void wcut(int chip);
    bool nextIsNote(const Voice& v) const;
    uint16_t freq(uint8_t note) const;
    uint16_t notef(const Voice& v, int idx, uint8_t add) const;
    void next(Voice& v, int idx);
    void dnext(Voice& v, int idx);
    void dhit(Voice& v, int idx, uint8_t h);
    void dvset(Voice& v, int idx);
    void dput(uint8_t idx, uint8_t val) { if (!mute_ && dwrite_) dwrite_(idx, val); }
    void tick();
    void effects();
    void rate();
    void silence();
};
