// TunePlayer.cpp — the model of lib/stdlib_asm/tune.s45 (see TunePlayer.hpp):
// each step as the assembly does it, in the same order, with its 8- and
// 16-bit arithmetic
#include "TunePlayer.hpp"

namespace {
const uint8_t vreg[12] = {0x00, 0x07, 0x0E, 0x20, 0x27, 0x2E, 0x40, 0x47, 0x4E, 0x60, 0x67, 0x6E};
const uint16_t oct[24] = {
    17820, 18880, 20003, 21192, 22452, 23787, 25202, 26700, 28288, 29970, 31752, 33640,   // PAL
    17167, 18188, 19270, 20415, 21629, 22916, 24278, 25722, 27251, 28872, 30589, 32407};  // NTSC
const int8_t tri[8] = {0, 1, 2, 1, 0, -1, -2, -1};
}  // namespace

void TunePlayer::put(uint8_t idx, uint8_t val) {
    write_(idx, val);
    if (mirror_ && idx < 0x40) write_((uint8_t)(idx + 0x40), val);
}

void TunePlayer::rate() {
    // NTSC: scale * 213 >> 8 (a frame is 5/6 of a PAL one)
    step_ = ntsc_ ? (uint16_t)(((uint32_t)scale_ * 213) >> 8) : scale_;
}

void TunePlayer::inst(Voice& v, uint8_t i) {
    if (i >= ninst_) return;
    unsigned at = ins_ + i * 10u;
    v.wave = byte(at);
    v.ad = byte(at + 1);
    v.sr = byte(at + 2);
    v.ipw = (uint16_t)(byte(at + 3) | (byte(at + 4) << 8));
    v.vib = byte(at + 5);
    v.gap = byte(at + 6);
    v.pwm = (int8_t)byte(at + 7);
    v.vdel = byte(at + 8);
    v.slide = (int8_t)byte(at + 9);
}

uint16_t TunePlayer::freq(uint8_t note) const {
    unsigned octave = note / 12, n = note % 12 + (ntsc_ ? 12 : 0);
    uint16_t r = oct[n];
    if (octave > 8 || (octave == 8 && (r & 0x8000))) return 0xFFFF;
    if (octave == 8) return (uint16_t)(r << 1);
    while (octave < 7) { r >>= 1; octave++; }
    return r;
}

uint16_t TunePlayer::notef(const Voice& v, int idx, uint8_t add) const {
    uint8_t n = (uint8_t)(add + v.note + (uint8_t)transp_[idx]);
    if (n & 0x80) n = transp_[idx] < 0 ? 0 : 127;
    uint16_t f = freq(n);
    int8_t d = detune_[idx];
    if (d) {
        uint16_t s = (uint16_t)(f >> 6);
        if (d > 0) for (int k = 0; k < d; k++) f = (uint16_t)(f + s);
        else for (int k = 0; k < -d; k++) f = (uint16_t)(f - s);
    }
    return f;
}

void TunePlayer::silence() {
    active_ = false;
    for (int k = 0; k < n_; k++) {
        v_[k].gated = 0;
        ctrl(v_[k]);
    }
}

void TunePlayer::play(const std::vector<uint8_t>& tune) {
    silence();
    tune_ = tune;
    uint8_t nv = byte(1);
    if (nv) {
        if (nv > 12) nv = 12;
        uint8_t room = (uint8_t)(12 - first_);
        if (room < nv) nv = room;
    }
    n_ = nv;
    fpt_ = (uint16_t)(byte(2) | (byte(3) << 8));
    if (!fpt_) fpt_ = 0x100;
    ninst_ = byte(4);
    ins_ = (uint16_t)(byte(6) | (byte(7) << 8));
    rate();
    for (int k = 0; k < n_; k++) {
        Voice& v = v_[k];
        v.base = vreg[first_ + k];
        v.start = v.pos = (uint16_t)(byte(8 + 2 * k) | (byte(9 + 2 * k) << 8));
        v.left = 1;
        v.gated = v.done = 0;
        v.arpa = v.arpb = 0;
        v.t = 0;
        v.pwdir = 0;
        inst(v, 0);
    }
    acc_ = fpt_ >= step_ ? (uint16_t)(fpt_ - step_) : 0;
    ticks_ = 0;
    active_ = n_ != 0;
}

void TunePlayer::stop() { silence(); }

void TunePlayer::next(Voice& v, int idx) {
    bool guard = false;
    for (;;) {
        uint8_t b = rd(v);
        if (b < 0x80) {
            v.note = b;
            v.left = rd(v);
            v.gated = 0;
            v.t = 0;
            v.arpi = 0;
            ctrl(v);
            v.pw = v.ipw;
            v.pwdir = v.pwm;
            wpw(v);
            w(v, 5, v.ad);
            w(v, 6, v.sr);
            v.freq = notef(v, idx, 0);
            wf(v, v.freq);
            v.gated = 1;
            ctrl(v);
            return;
        }
        if (b == 0x80) { v.left = rd(v); v.gated = 0; ctrl(v); return; }
        if (b == 0x81) { inst(v, rd(v)); continue; }
        if (b == 0x82) { v.left = rd(v); return; }
        if (b == 0x83) { v.arpa = rd(v); v.arpb = rd(v); continue; }
        if (b == 0xFE && !guard) { guard = true; v.pos = v.start; continue; }
        v.done = 1;
        v.gated = 0;
        ctrl(v);
        return;
    }
}

void TunePlayer::tick() {
    ticks_++;
    int done = 0;
    for (int k = 0; k < n_; k++) {
        Voice& v = v_[k];
        if (v.done) { done++; continue; }
        if (--v.left == 0) {
            next(v, k);
            if (v.done) done++;
        } else if (v.gated && v.left == v.gap) {
            v.gated = 0;
            ctrl(v);
        }
    }
    if (done == n_) active_ = false;
}

void TunePlayer::effects() {
    for (int k = 0; k < n_; k++) {
        Voice& v = v_[k];
        if (!v.gated) continue;
        if (v.t != 255) v.t++;
        if (v.pwdir) {
            v.pw = (uint16_t)(v.pw + v.pwdir);
            uint8_t hi = (uint8_t)(v.pw >> 8);
            if (hi >= 0x0E || hi < 0x02) v.pwdir = (int8_t)(0 - v.pwdir);
            wpw(v);
        }
        if (v.slide) {
            int f = v.freq + v.slide * 16;   // (stopping at 0 and $FFFF)
            v.freq = (uint16_t)(f < 0 ? 0 : f > 0xFFFF ? 0xFFFF : f);
        }
        if (v.arpa | v.arpb) {
            v.arpi = (uint8_t)(v.arpi + 1 >= 3 ? 0 : v.arpi + 1);
            uint8_t add = v.arpi == 0 ? 0 : v.arpi == 1 ? v.arpa : v.arpb;
            wf(v, notef(v, k, add));
            continue;
        }
        uint16_t f = v.freq;
        uint8_t depth = (uint8_t)(v.vib + vibx_);
        if (depth && v.t > v.vdel && tri[v.t & 7]) {
            int tr = tri[v.t & 7];
            uint16_t prod = (uint16_t)((v.freq >> 8) * depth);
            if (tr & 1) prod >>= 1;
            f = (uint16_t)(tr > 0 ? f + prod : f - prod);
        }
        wf(v, f);
    }
}

void TunePlayer::update() {
    if (!active_) return;
    acc_ = (uint16_t)(acc_ + step_);
    while (acc_ >= fpt_) {
        acc_ = (uint16_t)(acc_ - fpt_);
        tick();
        if (!active_) return;
    }
    effects();
}
