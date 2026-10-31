// TunePlayer.cpp — the model of lib/stdlib_asm/tune.s45 (see TunePlayer.hpp):
// each step as the assembly does it, in the same order, with its 8- and
// 16-bit arithmetic
#include "TunePlayer.hpp"

using namespace tunepack;

namespace {
const uint8_t vreg[12] = {0x00, 0x07, 0x0E, 0x20, 0x27, 0x2E, 0x40, 0x47, 0x4E, 0x60, 0x67, 0x6E};
const uint16_t oct[24] = {
    17820, 18880, 20003, 21192, 22452, 23787, 25202, 26700, 28288, 29970, 31752, 33640,   // PAL
    17167, 18188, 19270, 20415, 21629, 22916, 24278, 25722, 27251, 28872, 30589, 32407};  // NTSC
const int8_t tri[8] = {0, 1, 2, 1, 0, -1, -2, -1};
}  // namespace

void TunePlayer::put(uint8_t idx, uint8_t val) {
    if (mute_) return;
    write_(idx, val);
    shadow_[idx & 0x7F] = val;
    if (mirror_ && idx < 0x40) {
        write_((uint8_t)(idx + 0x40), val);
        shadow_[idx + 0x40] = val;
    }
}

void TunePlayer::rate() {
    // NTSC: scale * 213 >> 8 (a frame is 5/6 of a PAL one)
    step_ = ntsc_ ? (uint16_t)(((uint32_t)scale_ * 213) >> 8) : scale_;
}

void TunePlayer::inst(Voice& v, uint8_t i) {
    if (i >= ninst_) return;
    // its bytes: as they are, or (coded) its groups' entries
    uint8_t r[19] = {};
    if (coded_) code_.instrument((uint16_t)(ins_ + i * (rsize_ >= 19 ? 6 : 5)), rsize_, r);
    else {
        unsigned at = (uint16_t)(ins_ + i * rsize_);
        for (int k = 0; k < rsize_; k++) r[k] = byte(at + k);
    }
    v.wave = r[0];
    v.ad = r[1];
    v.sr = r[2];
    v.ipw = (uint16_t)(r[3] | (r[4] << 8));
    v.vib = r[5] & 15;
    v.vspd = r[5] >> 4;
    v.gap = r[6];
    v.pwm = (int8_t)r[7];
    v.vdel = r[8];
    v.slide = (int8_t)r[9];
    v.cw = v.wave;
    v.hr = v.filt = v.tail = v.free = v.pwrun = v.cutrun = v.ipwsaw = 0;
    v.fsweep = 0;
    v.cutoff = v.table = 0;
    if (rsize_ >= 17) {   // (10: a tune of before the hard restart, filter and wave tables)
        v.hr = r[10] & 3;
        v.tail = r[10] & 0x80;
        v.free = r[10] & 0x40;
        v.pwrun = r[10] & 0x20;
        v.cutrun = r[10] & 0x10;
        v.ipwsaw = r[10] & 0x04;
        v.filt = r[11];
        v.cutoff = (uint16_t)(r[12] | (r[13] << 8));
        v.fsweep = (int8_t)r[14];
        v.table = (uint16_t)(r[15] | (r[16] << 8));
    }
    v.ipwlo = rsize_ >= 19 ? r[17] : 0;   // (19: a pulse-width cycle's limits)
    v.ipwhi = rsize_ >= 19 ? r[18] : 0;
}

// a chip's cutoff to its registers: $15 the low 3 bits, $16 the high 8
void TunePlayer::wcut(int c) {
    put((uint8_t)(c * 0x20 + 0x15), (uint8_t)(cut_[c] & 7));
    put((uint8_t)(c * 0x20 + 0x16), (uint8_t)(cut_[c] >> 3));
}

// a note's filter: a filtered instrument sets its chip's cutoff and sweep,
// resonance, mode and routes the voice through it (the volume and the other
// voices' routing kept); an unfiltered one takes the voice out
void TunePlayer::filter(Voice& v) {
    int c = v.base >> 5;
    uint8_t r17 = (uint8_t)((v.base & 0x60) | 0x17), r18 = (uint8_t)(r17 + 1);
    if (!v.filt) {
        if (shadow_[r17] & v.fbit) put(r17, (uint8_t)(shadow_[r17] & ~v.fbit));
        return;
    }
    if (!v.cutrun) {   // (cutrun: the cutoff as it is)
        cut_[c] = v.cutoff;
        csw_[c] = v.fsweep;
        wcut(c);
    }
    put(r17, (uint8_t)((v.filt << 4) | (shadow_[r17] & 0x0F) | v.fbit));
    put(r18, (uint8_t)((shadow_[r18] & 0x8F) | (v.filt & 0x70)));
}

// the voice's next event a note (after instruments, arpeggios, glides,
// pulse widths and cutoffs, through pattern calls and ends, and from the
// loop point once)? Read on a copy of the voice
bool TunePlayer::nextIsNote(const Voice& voice) const {
    Voice v = voice;
    TunePlayer& self = const_cast<TunePlayer&>(*this);   // (reads only: the copy moves)
    bool guard = false;
    for (;;) {
        uint8_t b = self.rd(v, OP);
        if (b == 0x81) self.rd(v, INST);
        else if (b == 0x83) { self.rd(v, ARPA); self.rd(v, ARPB); }
        else if (b == 0x84) { self.rd(v, GLS); self.rd(v, GLK); }
        else if (b == 0x85) { self.rd(v, TLO); self.rd(v, THI); }
        else if (b == 0x89) { self.rd(v, PWLO); self.rd(v, PWHI); self.rd(v, PWS); }
        else if (b == 0x8A) { self.rd(v, CLO); self.rd(v, CHI); self.rd(v, CS); }
        else if (b == 0x86 || b == 0x8B) self.call(v, b, false);
        else if (b == 0x87) self.back(v);
        else if (b == 0x8C) continue;
        else if (b == 0xFE && !guard) { guard = true; v.pos = v.start; v.bit = v.sbit; }
        else return b < 0x80;
    }
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
    for (int j = 0; j < nd_; j++) dput((uint8_t)(0x10 + (dfirst_ + j) * 16), 0);   // (the channels stopped)
}

void TunePlayer::play(const std::vector<uint8_t>& tune) {
    silence();
    tune_ = tune;
    uint8_t nvd = byte(1), ndd = (uint8_t)((byte(0) >> 1) & 7);   // the voices in the data
    uint8_t nv = nvd;
    if (nv) {
        if (nv > 12) nv = 12;
        uint8_t room = (uint8_t)(12 - first_);
        if (room < nv) nv = room;
    }
    n_ = nv;
    fpt_ = (uint16_t)(byte(2) | (byte(3) << 8));
    if (!fpt_) fpt_ = 0x100;
    coded_ = (byte(0) & 0x20) != 0;
    if (coded_) {   // (the code tables; the pattern table's offset after the voices')
        code_.init(tune_);
        unsigned h = 8 + 2 * (nvd + ndd) + (ndd ? 2 : 0);
        pats_ = (uint16_t)(byte(h + 2) | (byte(h + 3) << 8));
    }
    ninst_ = byte(4);
    rsize_ = byte(5) ? byte(5) : 10;
    for (int c = 0; c < 4; c++) { cut_[c] = 0; csw_[c] = 0; }
    ins_ = (uint16_t)(byte(6) | (byte(7) << 8));
    rate();
    for (int k = 0; k < n_; k++) {
        Voice& v = v_[k];
        v.base = vreg[first_ + k];
        v.start = v.pos = (uint16_t)(byte(8 + 2 * k) | (byte(9 + 2 * k) << 8));
        v.bit = v.sbit = v.sp = v.ptr = 0;
        v.left = 1;
        v.gated = v.done = 0;
        v.arpa = v.arpb = 0;
        v.t = 0;
        v.pwdir = 0;
        v.gpend = v.glide = 0;
        v.hrdone = v.wti = 0;
        v.wsemi = 0;
        v.fbit = (uint8_t)(1 << ((first_ + k) % 3));
        inst(v, 0);
    }
    // the digi voices: as many as channels from the first
    nd_ = ndd;
    if (nd_ > 4 - dfirst_) nd_ = (uint8_t)(4 - dfirst_);
    stab_ = (uint16_t)(byte(8 + 2 * (nvd + ndd)) | (byte(9 + 2 * (nvd + ndd)) << 8));
    e9_ = (byte(0) & 0x10) != 0;
    for (int j = 0; j < nd_; j++) {
        Voice& v = v_[12 + j];
        v.start = v.pos = (uint16_t)(byte(8 + 2 * (nvd + j)) | (byte(9 + 2 * (nvd + j)) << 8));
        v.bit = v.sbit = v.sp = v.ptr = 0;
        v.left = 1;
        v.gated = v.done = 0;
        v.dvol = 255;
    }
    acc_ = fpt_ >= step_ ? (uint16_t)(fpt_ - step_) : 0;
    ticks_ = 0;
    active_ = n_ + nd_ != 0;
}

// A digi voice's next event: a hit (dhit), a rest or tie (the sample rings
// on), a stop ($81: the channel off), its volume ($83: no time)
void TunePlayer::dnext(Voice& v, int idx) {
    bool guard = false;
    uint8_t c = (uint8_t)(0x10 + (dfirst_ + idx - 12) * 16);
    for (;;) {
        uint8_t b = rd(v, DOP);
        if (b < 0x80 || b == 0x84) {
            if (b == 0x84) b = rd(v, DHIT);
            v.left = rd(v, DUR);
            dhit(v, idx, b);
            return;
        }
        if (b == 0x80 || b == 0x82) { v.left = rd(v, DUR); return; }
        if (b == 0x81) { dput(c, 0); v.left = rd(v, DUR); return; }   // stopped
        if (b == 0x83) { v.dvol = rd(v, DVOL); dvset(v, idx); continue; }
        if (b == 0x86) { call(v, b, true); continue; }
        if (b == 0x87) { back(v); continue; }
        if (b == 0x8C) { v.start = v.pos; v.sbit = v.bit; continue; }   // the loop point: back to here at the end
        if (b == 0xFE && !guard) { guard = true; v.pos = v.start; v.bit = v.sbit; continue; }
        v.done = 1;
        return;
    }
}

// the pulse-width sweep a frame: bouncing between $200 and $E00, or free
// (on to 0 or $FFF, stopped there)
void TunePlayer::pwstep(Voice& v) {
    if (!v.pwdir) return;
    v.pw = (uint16_t)(v.pw + v.pwdir);
    uint8_t hi = (uint8_t)(v.pw >> 8);
    if (v.pwhi) {   // a cycle: within 0-$FFF, turning at its limits
        if (hi >= 0x10) v.pw = (hi & 0x80) ? 0 : 0xFFF;
        uint8_t p4 = (uint8_t)(v.pw >> 4);
        if (v.pwdir > 0 ? p4 >= v.pwhi : p4 <= v.pwlo) {   // (at a limit: it turns)
            if (v.pwsaw) v.pw = (uint16_t)((v.pwdir > 0 ? v.pwlo : v.pwhi) << 4);   // (a sawtooth: back)
            else v.pwdir = (int8_t)(0 - v.pwdir);
        }
    } else if (v.pwfree) {
        if (hi >= 0x10) { v.pw = (hi & 0x80) ? 0 : 0xFFF; v.pwdir = 0; }
    } else if (hi >= 0x0E || hi < 0x02) v.pwdir = (int8_t)(0 - v.pwdir);
    wpw(v);
}

// a pattern call ($86 lo hi n, the $86 read; coded: its number, n): on at
// it, n times, then after; $8B: transposed t more (two deep: a third call
// is passed over)
void TunePlayer::call(Voice& v, uint8_t b, bool digi) {
    uint16_t at;
    if (coded_) {
        uint8_t n = rd(v, PAT);
        at = (uint16_t)(byte(pats_ + 2u * n) | (byte(pats_ + 2u * n + 1) << 8));
    } else {
        uint8_t lo = rd(v, PAT), hi = rd(v, PAT);
        at = (uint16_t)(lo | (hi << 8));
    }
    uint8_t rep = rd(v, REP);
    uint8_t t = b == 0x8B && !digi ? rd(v, TR) : 0;
    if (v.sp >= kDepth) return;
    auto& l = v.lv[v.sp++];
    l.ret = v.pos;
    l.rbit = v.bit;
    l.pat = at;
    l.rep = rep;
    l.optr = v.ptr;
    v.ptr = (uint8_t)(v.ptr + t);
    v.pos = at;
    v.bit = 0;
}

// the voice's volume on its channel: tune_digi_volume's times the voice's
// / 256 (all of it: the former as it is), both sides
void TunePlayer::dvset(Voice& v, int idx) {
    uint8_t c = (uint8_t)(0x10 + (dfirst_ + idx - 12) * 16);
    uint8_t vol = v.dvol == 255 ? dvol_ : (uint8_t)((dvol_ * v.dvol) >> 8);
    dput((uint8_t)(c + 9), vol);
    dput((uint8_t)(0x0C + dfirst_ + idx - 12), vol);
}

// hit h: its sample on the voice's channel, as digi_play sets one up
// (stopped, where it loops to, the current address, the rate, the end, the
// volume on both sides, then on: 8-bit unsigned, once or looping)
void TunePlayer::dhit(Voice& v, int idx, uint8_t h) {
    unsigned e = stab_ + h * (e9_ ? 9u : 7u);
    uint32_t addr = (sbank_ + (byte(e) | (byte(e + 1) << 8))) & 0xFFFFFF;
    uint16_t end = (uint16_t)(addr + (byte(e + 2) | (byte(e + 3) << 8)));
    uint16_t lp = e9_ ? (uint16_t)(byte(e + 7) | (byte(e + 8) << 8)) : 0xFFFF;
    uint32_t loopAt = lp == 0xFFFF ? addr : (addr + lp) & 0xFFFFFF;
    uint8_t c = (uint8_t)(0x10 + (dfirst_ + idx - 12) * 16);
    dput(c, 0);
    dput((uint8_t)(c + 1), (uint8_t)loopAt);
    dput((uint8_t)(c + 2), (uint8_t)(loopAt >> 8));
    dput((uint8_t)(c + 3), (uint8_t)(loopAt >> 16));
    dput((uint8_t)(c + 0xA), (uint8_t)addr);
    dput((uint8_t)(c + 0xB), (uint8_t)(addr >> 8));
    dput((uint8_t)(c + 0xC), (uint8_t)(addr >> 16));
    dput((uint8_t)(c + 4), byte(e + 4));
    dput((uint8_t)(c + 5), byte(e + 5));
    dput((uint8_t)(c + 6), byte(e + 6));
    dput((uint8_t)(c + 7), (uint8_t)end);
    dput((uint8_t)(c + 8), (uint8_t)(end >> 8));
    dvset(v, idx);
    dput(c, lp == 0xFFFF ? 0xA2 : 0xE2);
    dput(0x01, 0x80);
}

void TunePlayer::stop() { silence(); }

void TunePlayer::next(Voice& v, int idx) {
    bool guard = false;
    for (;;) {
        uint8_t b = rd(v, OP);
        if (b < 0x80) {
            v.note = (uint8_t)((b + v.ptr) & 0x7F);   // (a pattern's transposition)
            v.left = rd(v, DUR);
            v.gated = 0;
            v.t = 0;
            v.vph = v.vcnt = 0;
            v.arpi = 0;
            v.hrdone = v.wti = 0;
            v.wsemi = 0;
            v.cw = v.wave;
            ctrl(v);
            if (!v.pwrun) {   // (pwrun: the pulse width as it is)
                v.pw = v.ipw;
                v.pwdir = v.pwm;
                v.pwfree = v.free;
            }
            v.pwlo = v.ipwlo;   // (its cycle's limits)
            v.pwhi = v.ipwhi;
            v.pwsaw = v.ipwsaw;
            wpw(v);
            w(v, 5, v.ad);
            w(v, 6, v.sr);
            filter(v);
            v.freq = notef(v, idx, 0);
            v.glide = v.gpend;
            if (v.glide) {      // to this pitch, from gsemi semitones below
                v.gpend = 0;
                v.tgt = v.freq;
                v.freq = notef(v, idx, (uint8_t)(0 - (uint8_t)v.gsemi));
            }
            if (v.table) {      // the wave table's first step: its waveform and pitch
                v.cw = byte(v.table + 2u);
                v.wsemi = (int8_t)byte(v.table + 3u);
            }
            wf(v, v.wsemi ? notef(v, idx, (uint8_t)v.wsemi) : v.freq);
            v.gated = 1;
            ctrl(v);
            return;
        }
        if (b == 0x80) { v.left = rd(v, DUR); v.gated = 0; ctrl(v); return; }
        if (b == 0x81) { inst(v, rd(v, INST)); continue; }
        if (b == 0x82) { v.left = rd(v, DUR); return; }
        if (b == 0x83) { v.arpa = rd(v, ARPA); v.arpb = rd(v, ARPB); continue; }
        if (b == 0x84) { v.gsemi = (int8_t)rd(v, GLS); v.gpend = rd(v, GLK); continue; }
        if (b == 0x85) { uint8_t lo = rd(v, TLO); fpt_ = (uint16_t)(lo | (rd(v, THI) << 8)); continue; }
        if (b == 0x86 || b == 0x8B) { call(v, b, false); continue; }
        if (b == 0x87) { back(v); continue; }
        if (b == 0x89) {   // the pulse width and its sweep
            uint8_t lo = rd(v, PWLO), hi = rd(v, PWHI);
            v.pw = (uint16_t)(lo | (hi << 8));
            v.pwdir = (int8_t)rd(v, PWS);
            v.pwfree = 0x40;   // (free)
            wpw(v);
            continue;
        }
        if (b == 0x8A) {   // the chip's cutoff and its sweep
            int c = v.base / 0x20;
            uint8_t lo = rd(v, CLO), hi = rd(v, CHI);
            cut_[c] = (uint16_t)(lo | (hi << 8));
            csw_[c] = (int8_t)rd(v, CS);
            wcut(c);
            continue;
        }
        if (b == 0x88) {   // legato: the pitch, the note going on
            v.note = (uint8_t)((rd(v, LNOTE) + v.ptr) & 0x7F);
            v.left = rd(v, DUR);
            v.glide = 0;
            v.freq = notef(v, idx, 0);
            wf(v, v.freq);
            return;
        }
        if (b == 0x8C) { v.start = v.pos; v.sbit = v.bit; continue; }   // the loop point: back to here at the end
        if (b == 0xFE && !guard) { guard = true; v.pos = v.start; v.bit = v.sbit; continue; }
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
    for (int k = 12; k < 12 + nd_; k++) {
        Voice& v = v_[k];
        if (v.done) { done++; continue; }
        if (--v.left == 0) {
            dnext(v, k);
            if (v.done) done++;
        }
    }
    if (done == n_ + nd_) active_ = false;
}

void TunePlayer::effects() {
    for (int k = 0; k < n_; k++) {
        Voice& v = v_[k];
        // hard restart: hr frames before a note, the gate off and the
        // envelope's release at its fastest (so the next attack starts clean)
        if (v.hr && !v.hrdone && !v.done && v.left == 1 && soon_ <= v.hr && nextIsNote(v)) {
            v.hrdone = 1;
            v.gated = 0;
            ctrl(v);
            w(v, 5, 0);
            w(v, 6, 0);
        }
        if (!v.gated && !(v.tail && !v.done)) {   // (tail: on through the release)
            if (v.pwrun && !v.done) pwstep(v);   // (pwrun: the pulse width swept in rests too)
            continue;
        }
        if (v.t != 255) v.t++;
        pwstep(v);
        if (v.glide && v.t >= 2) {   // (distance + 2^k - 1) >> k towards the target
            uint32_t mask = (1u << v.glide) - 1;
            if (v.tgt == v.freq) v.glide = 0;
            else if (v.tgt > v.freq) v.freq = (uint16_t)(v.freq + (((uint32_t)(v.tgt - v.freq) + mask) >> v.glide));
            else v.freq = (uint16_t)(v.freq - (((uint32_t)(v.freq - v.tgt) + mask) >> v.glide));
        }
        if (v.slide) {
            int f = v.freq + v.slide * 16;   // (stopping at 0 and $FFFF)
            v.freq = (uint16_t)(f < 0 ? 0 : f > 0xFFFF ? 0xFFFF : f);
        }
        if (v.table && v.t >= 2) {   // the wave table's next step (at its end: its loop, or the last again)
            uint8_t len = byte(v.table), loop = byte(v.table + 1u);
            if (++v.wti >= len) v.wti = loop == 0xFF ? (uint8_t)(len - 1) : loop;
            uint8_t wave = byte(v.table + 2u + 2u * v.wti);
            v.wsemi = (int8_t)byte(v.table + 3u + 2u * v.wti);
            if (wave != v.cw) { v.cw = wave; ctrl(v); }
        }
        if (v.arpa | v.arpb) {
            v.arpi = (uint8_t)(v.arpi + 1 >= (v.arpb == 0x80 ? 2 : 3) ? 0 : v.arpi + 1);   // (b $80: two notes)
            uint8_t add = v.arpi == 0 ? 0 : v.arpi == 1 ? v.arpa : v.arpb;
            wf(v, notef(v, k, (uint8_t)(add + (uint8_t)v.wsemi)));
            continue;
        }
        if (v.wsemi) {
            wf(v, notef(v, k, (uint8_t)v.wsemi));
            continue;
        }
        uint16_t f = v.freq;
        uint8_t depth = (uint8_t)(v.vib + vibx_);
        uint8_t step = v.t;
        if (depth && v.t > v.vdel && v.vspd) {   // (slower: a step every speed + 1 frames)
            if (!v.vcnt) { v.vcnt = v.vspd; v.vph++; }
            else v.vcnt--;
            step = v.vph;
        }
        if (depth && v.t > v.vdel && tri[step & 7]) {
            int tr = tri[step & 7];
            uint16_t prod = (uint16_t)((v.freq >> 8) * depth);
            if (tr & 1) prod >>= 1;
            f = (uint16_t)(tr > 0 ? f + prod : f - prod);
        }
        wf(v, f);
    }
    // each chip's cutoff sweep, stopping at 0 and $7FF
    for (int c = 0; c < 4; c++) {
        if (!csw_[c]) continue;
        int x = cut_[c] + csw_[c];
        if (x < 0 || x > 0x7FF) {
            x = x < 0 ? 0 : 0x7FF;
            csw_[c] = 0;
        }
        cut_[c] = (uint16_t)x;
        wcut(c);
    }
}

void TunePlayer::seek(unsigned ticks) {
    mute_ = true;
    for (; ticks && active_; ticks--) tick();
    mute_ = false;
}

void TunePlayer::update() {
    if (!active_) return;
    acc_ = (uint16_t)(acc_ + step_);
    while (acc_ >= fpt_) {
        acc_ = (uint16_t)(acc_ - fpt_);
        tick();
        if (!active_) return;
    }
    // frames to the next tick (1-4), for the hard restart
    uint32_t a = (uint32_t)acc_ + step_;
    soon_ = 1;
    while (a < fpt_ && soon_ < 4) {
        a += step_;
        soon_++;
    }
    effects();
}
