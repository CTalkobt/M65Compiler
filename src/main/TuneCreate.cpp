// TuneCreate.cpp — tune45 create (see TuneCreate.hpp): examples/c/musicgen's
// composer, step for step, writing tune45's text instead of playing
#include "TuneCreate.hpp"
#include "TuneCompiler.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace {

enum { MAJ, MIN, DIM, DOM7, MAJ7, MIN7, M7B5, POW };
const uint8_t qualIv[8][4] = {
    {0, 4, 7, 255}, {0, 3, 7, 255}, {0, 3, 6, 255}, {0, 4, 7, 10},
    {0, 4, 7, 11},  {0, 3, 7, 10},  {0, 3, 6, 10},  {0, 7, 12, 255},
};
struct Chord { uint8_t root, qual; uint16_t alt; };

const uint16_t SC_MAJOR = 0x0AB5, SC_MINOR = 0x05AD, SC_MIXO = 0x06B5, SC_BLUES = 0x04E9;

// the instruments (musicgen's 14, its vibrato doubled: tune45's is halved)
struct Inst { const char* name; const char* wave; uint8_t ad, sr; uint16_t pw; int pwm, vib, vdelay; };
enum { I_PIANO, I_ORGAN, I_FLUTE, I_BRASS, I_STRINGS, I_HARPSI, I_SYNTH, I_SQUARE,
       I_HARMONICA, I_VIBES, I_UPRIGHT, I_SYNBASS, I_PLUCK, I_PAD, N_INST };
const Inst insts[N_INST] = {
    {"piano", "pulse", 0x0A, 0x0A, 0x0500, 0, 0, 0},
    {"organ", "pulse", 0x00, 0xF3, 0x0800, 0, 0, 0},
    {"flute", "tri", 0x60, 0xB7, 0x0800, 0, 4, 12},
    {"brass", "saw", 0x52, 0xA5, 0x0800, 0, 2, 20},
    {"strings", "saw", 0x96, 0xC9, 0x0800, 0, 2, 20},
    {"harpsichord", "pulse", 0x09, 0x07, 0x0300, 0, 0, 0},
    {"synth_lead", "pulse", 0x23, 0xB6, 0x0400, 24, 4, 15},
    {"square", "pulse", 0x02, 0xA4, 0x0800, 0, 0, 0},
    {"harmonica", "pulse", 0x43, 0xB5, 0x0280, 0, 6, 8},
    {"vibes", "tri", 0x0A, 0x0A, 0x0800, 0, 2, 2},
    {"upright_bass", "tri", 0x0A, 0x58, 0x0800, 0, 0, 0},
    {"synth_bass", "saw", 0x08, 0x98, 0x0800, 0, 0, 0},
    {"pluck_bass", "pulse", 0x08, 0x28, 0x0600, 0, 0, 0},
    {"pad", "pulse", 0xB8, 0xCA, 0x0800, 6, 0, 0},
};

enum { ACC_HIT, ACC_POWER, ACC_ARP };
enum { FORM_AABA, FORM_BLUES };

struct Style {
    const char* name;
    uint8_t bpm, swing, form;
    uint16_t scale;
    uint8_t nch;
    const Chord* ch;
    const uint8_t* tr;
    uint8_t half, dom;
    const char *kick, *snare, *hat, *bass;
    uint8_t acc;
    const char *comp1, *comp2;
    const uint8_t* cells;
    uint8_t ncells;
    uint8_t lead, chords, bassi, drums;
    uint8_t lo, hi;
};

const Chord popCh[6] = {{0, MAJ, 0}, {7, MAJ, 0}, {9, MIN, 0}, {5, MAJ, 0}, {2, MIN, 0}, {4, MIN, 0}};
const uint8_t popTr[36] = {0, 4, 3, 5, 2, 1, 5, 0, 4, 2, 0, 1, 1, 2, 0, 5, 3, 1,
                           4, 5, 2, 0, 1, 1, 1, 6, 1, 2, 0, 0, 0, 1, 4, 4, 0, 0};
const uint8_t popCells[10] = {1, 5, 5, 9, 4, 0, 1, 5, 13, 4};
const Chord rockCh[6] = {{0, MAJ, 0}, {7, MAJ, 0}, {10, MAJ, 0}, {5, MAJ, 0}, {9, MIN, 0}, {3, MAJ, 0x0018}};
const uint8_t rockTr[36] = {0, 2, 4, 5, 1, 2, 4, 0, 1, 3, 1, 0, 3, 0, 0, 5, 0, 1,
                            5, 2, 2, 0, 1, 0, 1, 1, 1, 4, 0, 1, 2, 0, 3, 3, 0, 0};
const uint8_t rockCells[8] = {5, 5, 1, 13, 5, 0, 4, 1};
const Chord bluesCh[3] = {{0, DOM7, 0}, {5, DOM7, 0}, {7, DOM7, 0}};
const uint8_t bluesTr[9] = {0, 1, 1, 1, 0, 1, 1, 1, 0};
const uint8_t bluesCells[8] = {1, 5, 5, 4, 0, 1, 5, 4};
const Chord jazzCh[8] = {{0, MAJ7, 0}, {7, DOM7, 0}, {2, MIN7, 0}, {9, MIN7, 0}, {5, MAJ7, 0}, {4, MIN7, 0},
                         {9, DOM7, 0x0003}, {2, DOM7, 0x0060}};
const uint8_t jazzTr[64] = {0, 1, 4, 3, 2, 2, 3, 1, 8, 0, 1, 1, 0, 0, 0, 0, 1, 9, 0, 0, 0, 0, 0, 0,
                            0, 1, 6, 0, 1, 0, 0, 2, 2, 1, 2, 1, 0, 3, 0, 1, 0, 0, 1, 5, 1, 0, 3, 0,
                            0, 0, 7, 0, 0, 0, 0, 1, 0, 6, 1, 0, 0, 0, 0, 0};
const uint8_t jazzCells[8] = {5, 5, 4, 1, 5, 0, 4, 5};
const Chord classCh[7] = {{0, MAJ, 0}, {7, MAJ, 0}, {5, MAJ, 0}, {2, MIN, 0}, {9, MIN, 0}, {7, DOM7, 0}, {11, DIM, 0}};
const uint8_t classTr[49] = {0, 3, 4, 3, 3, 2, 1, 6, 0, 0, 0, 2, 2, 0, 2, 4, 0, 2, 0, 3, 1, 0, 4, 0, 0,
                             0, 5, 1, 0, 1, 3, 5, 0, 0, 0, 9, 0, 0, 0, 1, 0, 0, 8, 0, 0, 0, 0, 0, 0};
const uint8_t classCells[8] = {1, 5, 5, 15, 1, 5, 9, 13};
const Chord ambCh[6] = {{0, MIN, 0}, {7, MIN, 0}, {8, MAJ7, 0}, {10, MAJ, 0}, {5, MIN7, 0}, {3, MAJ7, 0}};
const uint8_t ambTr[36] = {0, 1, 4, 2, 3, 3, 3, 0, 3, 1, 1, 0, 2, 1, 0, 4, 2, 2,
                           4, 1, 2, 0, 1, 3, 3, 1, 2, 3, 0, 1, 2, 0, 3, 2, 3, 0};
const uint8_t ambCells[8] = {1, 1, 0, 1, 0, 4, 1, 0};
const Chord chipCh[6] = {{0, MIN, 0}, {7, MAJ, 0x0C00}, {8, MAJ, 0}, {10, MAJ, 0}, {5, MIN, 0}, {3, MAJ, 0}};
const uint8_t chipTr[36] = {0, 2, 4, 3, 3, 1, 7, 0, 1, 0, 1, 0, 1, 1, 0, 6, 1, 1,
                            5, 2, 1, 0, 0, 2, 2, 4, 1, 2, 0, 0, 1, 0, 3, 2, 3, 0};
const uint8_t chipCells[8] = {5, 15, 5, 13, 11, 1, 5, 15};

const Style styles[7] = {
    {"pop", 112, 0, FORM_AABA, SC_MAJOR, 6, popCh, popTr, 1, 1,
     "x.......x.x.....", "....x.......x...", "x.x.x.x.x.x.x.x.",
     "R..R..R.r..R.5..", ACC_HIT, "x..x..x...x.x...", "x.....x.x...x...", popCells, 10, I_SYNTH, I_PIANO, I_SYNBASS, 1, 62, 81},
    {"rock", 128, 0, FORM_AABA, SC_MIXO, 6, rockCh, rockTr, 1, 1,
     "x.....x.x.x.....", "....x.......x...", "x.x.x.x.x.x.x.x.",
     "R.R.R.R.R.R.5.r.", ACC_POWER, "x.-.x.-.x.-.x.x-", "x.-.x.-.x.-.x.-.", rockCells, 8, I_BRASS, I_SQUARE, I_PLUCK, 1, 60, 79},
    {"blues", 92, 1, FORM_BLUES, SC_BLUES, 3, bluesCh, bluesTr, 2, 2,
     "x.......x.......", "....x.......x...", "x.x.x.x.x.x.x.x.",
     "R.3.5.6.7.6.5.3.", ACC_HIT, "....x.-.....x.-.", "..x.....x.-.....", bluesCells, 8, I_HARMONICA, I_PIANO, I_UPRIGHT, 1, 60, 79},
    {"jazz", 132, 1, FORM_AABA, SC_MAJOR, 8, jazzCh, jazzTr, 1, 1,
     "x...............", "..............x.", "x...x.x.x...x.x.",
     "W...W...W...W...", ACC_HIT, "..x.-.....x.-...", "x.-.....x.-..x-.", jazzCells, 8, I_VIBES, I_PIANO, I_UPRIGHT, 1, 62, 81},
    {"classical", 100, 0, FORM_AABA, SC_MAJOR, 7, classCh, classTr, 1, 5,
     "................", "................", "................",
     "LLLLLLLLLLLLLLLL", ACC_HIT, "x...............", "x.......x.......", classCells, 8, I_FLUTE, I_STRINGS, I_HARPSI, 0, 62, 84},
    {"ambient", 70, 0, FORM_AABA, SC_MINOR, 6, ambCh, ambTr, 1, 3,
     "x...............", "................", "..x...x...x...x.",
     "R...............", ACC_HIT, "x...............", "x...............", ambCells, 8, I_VIBES, I_PAD, I_SYNBASS, 0, 60, 79},
    {"chiptune", 150, 0, FORM_AABA, SC_MINOR, 6, chipCh, chipTr, 1, 1,
     "x...x...x...x...", "....x.......x..x", "..x...x...x...x.",
     "R.r.R.r.R.r.R.r.", ACC_ARP, "x...x...x...x...", "x.......x.x.x...", chipCells, 8, I_SQUARE, I_SQUARE, I_PLUCK, 1, 64, 86},
};

const uint8_t NONE = 255, REST = 1, R_NOTE = 1, R_REST = 2;
const uint8_t NOTE_LO = 24, NOTE_HI = 95;

// A voice's events, a 16th step at a time: HOLD, a rest, a note (an
// instrument or an arpeggio before it)
struct Event { int note = -1; bool rest = false; std::string pre; };

class Composer {
public:
    explicit Composer(uint32_t seed) : rs_((uint16_t)(seed ? seed : 0xACE1)) { if (!rs_) rs_ = 0xACE1; }

    CreateResult run(const CreateOptions& o);

private:
    uint16_t rs_;
    const Style* sty_ = nullptr;
    uint8_t key_ = 0, nbars_ = 8, centre_ = 0;
    uint8_t secCh_[2][12] = {};
    uint8_t secMel_[2][12 * 16] = {};
    uint8_t rhythm_[12 * 16] = {};
    uint8_t voicing_[3] = {};

    uint16_t rnd16() { rs_ ^= (uint16_t)(rs_ << 7); rs_ ^= (uint16_t)(rs_ >> 9); rs_ ^= (uint16_t)(rs_ << 8); return rs_; }
    uint8_t rndN(uint8_t n) { return (uint8_t)(((uint32_t)(rnd16() & 0xFF) * n) >> 8); }
    uint8_t pc(uint8_t n) const { return (uint8_t)((n + 120 - key_) % 12); }
    bool inMask(uint8_t n, uint16_t m) const { return (m >> pc(n)) & 1; }
    uint8_t stepIn(uint8_t n, int dir, uint16_t m) const {
        for (int i = 0; i < 12; i++) { n = (uint8_t)(n + dir); if (inMask(n, m)) break; }
        return n;
    }
    uint8_t nearIn(uint8_t n, uint16_t m, int dir) const {
        if (inMask(n, m)) return n;
        for (int d = 1; d < 12; d++) {
            if (inMask((uint8_t)(n + dir * d), m)) return (uint8_t)(n + dir * d);
            if (inMask((uint8_t)(n - dir * d), m)) return (uint8_t)(n - dir * d);
        }
        return n;
    }
    uint16_t chordMask(uint8_t c) const {
        const Chord& ch = sty_->ch[c];
        uint16_t m = 0;
        for (int i = 0; i < 4; i++) {
            uint8_t iv = qualIv[ch.qual][i];
            if (iv == 255) break;
            m |= (uint16_t)(1 << ((ch.root + iv) % 12));
        }
        return m;
    }
    uint16_t scaleMask(uint8_t c) const { return (uint16_t)((sty_->scale ^ sty_->ch[c].alt) | chordMask(c)); }
    uint8_t pickChord(uint8_t prev, uint8_t next);
    void genChords();
    void putCell(uint8_t beat, uint8_t cell);
    uint8_t cell() { return sty_->cells[rndN(sty_->ncells)]; }
    void cadenceBar(uint8_t bar, bool pickup);
    void genRhythm(uint8_t sec);
    void genMelody(uint8_t sec);
    void voiceChord(uint8_t c);
    uint8_t bassRoot(uint8_t c) const { uint8_t n = (uint8_t)(36 + (key_ + sty_->ch[c].root) % 12); return n > 44 ? n - 12 : n; }
};

uint8_t Composer::pickChord(uint8_t prev, uint8_t next) {
    uint8_t n = sty_->nch, w[8];
    unsigned tot = 0;
    for (uint8_t i = 0; i < n; i++) {
        w[i] = sty_->tr[prev * n + i];
        if (next != NONE) w[i] = (uint8_t)(w[i] * sty_->tr[i * n + next]);
        tot += w[i];
    }
    if (!tot) for (uint8_t i = 0; i < n; i++) { w[i] = sty_->tr[prev * n + i]; tot += w[i]; }
    unsigned r = rnd16() % tot;
    for (uint8_t i = 0; i < n; i++) { if (r < w[i]) return i; r -= w[i]; }
    return 0;
}

void Composer::genChords() {
    uint8_t* a = secCh_[0];
    uint8_t* b = secCh_[1];
    if (sty_->form == FORM_BLUES) {
        static const uint8_t twelve[12] = {0, 0, 0, 0, 1, 1, 0, 0, 2, 1, 0, 2};
        for (int i = 0; i < 2; i++) {
            for (int k = 0; k < 12; k++) secCh_[i][k] = twelve[k];
            if (rndN(2)) secCh_[i][1] = 1;
            if (rndN(3) == 0) secCh_[i][9] = 2;
        }
        return;
    }
    a[0] = 0;
    a[1] = pickChord(0, NONE);
    a[2] = pickChord(a[1], sty_->half);
    a[3] = sty_->half;
    a[4] = 0;
    a[5] = sty_->tr[a[1] * sty_->nch + sty_->dom] ? a[1] : pickChord(0, sty_->dom);
    a[6] = sty_->dom;
    a[7] = 0;
    b[0] = pickChord(0, NONE);
    for (int i = 1; i < 6; i++) b[i] = pickChord(b[i - 1], NONE);
    b[6] = pickChord(b[5], sty_->half);
    b[7] = sty_->half;
}

void Composer::putCell(uint8_t beat, uint8_t c) {
    unsigned s = beat * 4u;
    for (int i = 0; i < 4; i++) rhythm_[s + i] = (c >> i) & 1 ? R_NOTE : 0;
    if (c == 0 && rndN(2)) rhythm_[s] = R_REST;
}

void Composer::cadenceBar(uint8_t bar, bool pickup) {
    unsigned s = bar * 16u;
    for (int i = 0; i < 16; i++) rhythm_[s + i] = 0;
    rhythm_[s] = R_NOTE;
    rhythm_[s + (sty_->bpm < 90 ? 12 : 8)] = R_REST;
    if (pickup && rndN(2)) {
        rhythm_[s + 12] = R_NOTE;
        if (rndN(2)) rhythm_[s + 14] = R_NOTE;
    }
}

void Composer::genRhythm(uint8_t sec) {
    uint8_t motif[8];
    for (auto& m : motif) m = cell();
    if (motif[0] == 0) motif[0] = 1;
    if (sty_->form == FORM_BLUES) {
        for (uint8_t b = 0; b < 12; b++) {
            if (b == 0 || b == 1 || b == 4 || b == 5) {
                for (uint8_t k = 0; k < 4; k++) putCell((uint8_t)(b * 4 + k), motif[(b & 1) * 4 + k]);
            } else if (b == 8 || b == 9) {
                for (uint8_t k = 0; k < 4; k++) putCell((uint8_t)(b * 4 + k), cell());
            } else if (b == 10) {
                cadenceBar(b, false);
            } else {
                for (int k = 0; k < 16; k++) rhythm_[b * 16 + k] = 0;
                rhythm_[b * 16] = R_REST;
                if (b != 11 && rndN(3) == 0) putCell((uint8_t)(b * 4 + 2), cell());
                if (b == 11 && rndN(2)) rhythm_[b * 16 + 12] = R_NOTE;
            }
        }
        return;
    }
    for (uint8_t b = 0; b < 8; b++) {
        if (b == 3 || b == 7) { cadenceBar(b, b == 3 || sec == 1); continue; }
        for (uint8_t k = 0; k < 4; k++) {
            uint8_t c = motif[(b & 1) * 4 + k];
            if (b == 2 && k == rndN(4)) c = cell();
            if (b == 6) c = cell();
            if (sec == 1 && b >= 4 && b < 6) c = cell();
            putCell((uint8_t)(b * 4 + k), c);
        }
    }
}

void Composer::genMelody(uint8_t sec) {
    static const uint8_t arch[4][4] = {{0, 2, 4, 1}, {1, 3, 2, 0}, {0, 3, 5, 1}, {2, 4, 3, 0}};
    uint8_t* mel = secMel_[sec];
    unsigned lastI = 0, steps = nbars_ * 16u;
    uint8_t reps = 0;
    int resolve = 0;
    genRhythm(sec);
    uint8_t shape = rndN(4);
    uint8_t p = nearIn((uint8_t)(centre_ + (sec ? 3 : 0)), chordMask(secCh_[sec][0]), 1), prev = p;
    for (unsigned s = 0; s < steps; s++) {
        unsigned bar = s >> 4;
        mel[s] = 0;
        if (rhythm_[s] == R_REST) { mel[s] = REST; continue; }
        if (rhythm_[s] != R_NOTE) continue;
        if (sty_->form == FORM_AABA && bar >= 4 && bar < 6 && secCh_[sec][bar] == secCh_[sec][bar - 4] &&
            rhythm_[s - 64] == R_NOTE && (sec == 0 || bar == 4)) {
            p = mel[s - 64]; mel[s] = p; lastI = s; prev = p; resolve = 0;
            continue;
        }
        if (sty_->form == FORM_BLUES && (bar == 4 || bar == 5) && rhythm_[s - 64] == R_NOTE) {
            p = mel[s - 64]; mel[s] = p; lastI = s; prev = p; resolve = 0;
            continue;
        }
        uint16_t cm = chordMask(secCh_[sec][bar]), sm = scaleMask(secCh_[sec][bar]);
        uint8_t tgt = (uint8_t)(centre_ + (sec ? 2 : 0));
        {
            uint8_t up = arch[shape][bar & 3];
            if (sty_->form == FORM_BLUES) up = arch[shape][(bar >> 1) & 3];
            for (uint8_t k = 0; k < up; k++) tgt = stepIn(tgt, 1, sm);
        }
        int dir = tgt > p ? 1 : (tgt < p ? -1 : (rndN(2) ? 1 : -1));
        if (resolve) {
            p = stepIn(p, -resolve, sm);
            if ((s & 7) == 0 && !inMask(p, cm)) p = nearIn(p, cm, -resolve);
            resolve = 0;
        } else if ((s & 7) == 0) {
            uint8_t q = rndN(4) ? stepIn(p, dir, sm) : p;
            uint8_t p2 = nearIn(q, cm, dir);
            if ((p2 > p ? p2 - p : p - p2) > 7) p2 = nearIn(p, cm, dir);
            if (p2 == p && reps >= 1) p2 = stepIn(p, dir, cm);
            p = p2;
        } else {
            uint8_t r = rndN(10);
            if (r < 6) p = stepIn(p, dir, sm);
            else if (r < 8) p = stepIn(p, -dir, sm);
            else if (r < 9) { }
            else p = stepIn(stepIn(p, dir, cm), dir, cm);
        }
        // (not in musicgen: no leap over an octave, the note an octave nearer)
        if (p > prev + 12 && p >= sty_->lo + 12) p = (uint8_t)(p - 12);
        else if (p + 12 < prev && p + 12 <= sty_->hi) p = (uint8_t)(p + 12);
        while (p > sty_->hi) p = stepIn(p, -1, sm);
        while (p < sty_->lo) p = stepIn(p, 1, sm);
        if (sty_->form == FORM_AABA && (s == 48 || s == 112)) {
            if (s == 112 && sec == 0) {
                p = nearIn(p, 1, 1);
                if (p > sty_->hi) p = (uint8_t)(p - 12);
                if (mel[lastI] > 1) {
                    uint8_t a = stepIn(p, mel[lastI] >= p ? 1 : -1, scaleMask(secCh_[sec][lastI >> 4]));
                    mel[lastI] = a;
                }
            } else {
                p = nearIn(p, cm, -1);
            }
        }
        if (sty_->form == FORM_BLUES && s == 160) {
            p = nearIn(p, 1, -1);
            if (p < sty_->lo) p = (uint8_t)(p + 12);
        }
        if (p == prev) reps++; else reps = 0;
        if ((p > prev ? p - prev : prev - p) >= 5) resolve = p > prev ? 1 : -1;
        mel[s] = p;
        prev = p;
        lastI = s;
    }
}

void Composer::voiceChord(uint8_t c) {
    const Chord& ch = sty_->ch[c];
    uint16_t m = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t iv = qualIv[ch.qual][i];
        if (iv == 255) break;
        if (i == 2 && qualIv[ch.qual][3] != 255) continue;
        m |= (uint16_t)(1 << ((ch.root + iv) % 12));
    }
    if (sty_->acc == ACC_POWER || ch.qual == POW) {
        uint8_t b = (uint8_t)(40 + (key_ + ch.root) % 12);
        voicing_[0] = b; voicing_[1] = (uint8_t)(b + 7); voicing_[2] = (uint8_t)(b + 12);
        return;
    }
    unsigned best = 255;
    uint8_t v[3];
    for (uint8_t b = 52; b < 64; b++) {
        if (!inMask(b, m)) continue;
        v[0] = b; v[1] = stepIn(b, 1, m); v[2] = stepIn(v[1], 1, m);
        unsigned cost = 0;
        if (voicing_[0] == 0) cost = b > 57 ? b - 57u : 57u - b;
        else for (int i = 0; i < 3; i++) cost += v[i] > voicing_[i] ? v[i] - voicing_[i] : voicing_[i] - v[i];
        if (cost < best) {
            best = cost;
            voicing_[0] = v[0]; voicing_[1] = v[1]; voicing_[2] = v[2];
            if (!best) break;
        }
    }
    if (best == 255) { voicing_[0] = 52; voicing_[1] = 55; voicing_[2] = 59; }
}

// A voice's 16th-step events as tune45 text: notes and rests run until the
// next event, split at the bar lines (a note goes on as ~)
std::string voiceText(const std::vector<Event>& ev, const std::vector<int>& stepTicks, unsigned stepsPerBar) {
    std::ostringstream out;
    std::string cur;           // the token's name: a note, r or ~
    unsigned len = 0;
    auto flush = [&]() {
        if (!cur.empty() && len) out << " " << cur << ":" << len;
        len = 0;
    };
    for (size_t s = 0; s < ev.size(); s++) {
        if (s && s % stepsPerBar == 0) {
            flush();
            out << " |";
            if (cur != "r") cur = "~";
        }
        const Event& e = ev[s];
        if (e.rest || e.note >= 0 || !e.pre.empty()) {
            flush();
            if (!e.pre.empty()) out << " " << e.pre;
            if (e.rest) cur = "r";
            else if (e.note >= 0) cur = TuneCompiler::noteName(e.note);
        }
        if (cur.empty()) cur = "r";     // (nothing yet: a rest)
        len += (unsigned)stepTicks[s % stepTicks.size()];
    }
    flush();
    return out.str();
}

}  // namespace

const std::vector<std::string>& tuneGenres() {
    static const std::vector<std::string> g = {"pop", "rock", "blues", "jazz", "classical", "ambient", "chiptune"};
    return g;
}

CreateResult Composer::run(const CreateOptions& o) {
    CreateResult res;
    int si = -1;
    for (int k = 0; k < 7; k++) if (o.genre == styles[k].name) si = k;
    if (!o.genre.empty() && si < 0) { res.error = "no genre " + o.genre + " (pop, rock, blues, jazz, classical, ambient, chiptune)"; return res; }
    if (si < 0) si = rndN(7);
    sty_ = &styles[si];
    res.genre = sty_->name;
    int bpm = o.bpm > 0 ? o.bpm : sty_->bpm;
    nbars_ = sty_->form == FORM_BLUES ? 12 : 8;
    key_ = o.key >= 0 ? (uint8_t)(o.key % 12) : rndN(12);
    centre_ = (uint8_t)(sty_->lo + 7);
    genChords();
    genMelody(0);
    genMelody(1);

    // sections: AABA, as often as the bars ask for (whole sections; it ends on A)
    static const uint8_t plan[4] = {0, 0, 1, 0};
    unsigned sections = (unsigned)std::max(1, (o.bars + nbars_ - 1) / nbars_);
    std::vector<uint8_t> secs;
    for (unsigned k = 0; k < sections; k++) secs.push_back(plan[k & 3]);
    secs.back() = 0;

    // the tick: a 16th (swing: 4, 4, 2, 2 ticks of a 12-tick beat)
    std::vector<int> stepTicks = sty_->swing ? std::vector<int>{4, 4, 2, 2} : std::vector<int>{1};
    double fpt = 50.0 * 60.0 / (bpm * (sty_->swing ? 12.0 : 4.0));
    unsigned barTicks = sty_->swing ? 48 : 16;

    int nv = o.voices <= 3 ? 3 : o.voices >= 6 ? 6 : 4;
    bool drums = nv >= 4 && sty_->drums;
    bool chordsOne = nv < 6 || sty_->acc == ACC_ARP;   // the chords as one voice's arpeggio
    std::vector<Event> lead, bass, drum, ch[3];
    unsigned total = (unsigned)secs.size() * nbars_ * 16;
    lead.resize(total); bass.resize(total); drum.resize(total);
    for (auto& c : ch) c.resize(total);
    lead[0].pre = std::string("@") + insts[sty_->lead].name;
    bass[0].pre = std::string("@") + insts[sty_->bassi].name;
    for (auto& c : ch) c[0].pre = std::string("@") + insts[sty_->chords].name;
    const char* comp = sty_->comp1;
    std::string lastArp, lastDrum;
    voicing_[0] = 0;
    unsigned at = 0;
    for (size_t si2 = 0; si2 < secs.size(); si2++) {
        uint8_t sec = secs[si2];
        for (uint8_t bar = 0; bar < nbars_; bar++) {
            uint8_t c = secCh_[sec][bar];
            uint8_t next = bar + 1 < nbars_ ? secCh_[sec][bar + 1] : (si2 + 1 < secs.size() ? secCh_[secs[si2 + 1]][0] : 0);
            bool lastBar = bar == nbars_ - 1;
            for (uint8_t step = 0; step < 16; step++, at++) {
                if (step == 0) {
                    voiceChord(c);
                    comp = rndN(3) ? sty_->comp1 : sty_->comp2;
                }
                // the melody, and what the checks read (a tune that ends: no
                // pickup after its last cadence)
                uint8_t m = secMel_[sec][bar * 16 + step];
                if (!o.loop && si2 + 1 == secs.size() && bar * 16u + step > (sty_->form == FORM_BLUES ? 160u : 112u) && m > 1)
                    m = REST;
                if (m == REST) lead[at].rest = true;
                else if (m) lead[at].note = m;
                res.melody.push_back(m);
                res.chordMask.push_back(chordMask(c));
                res.scaleMask.push_back(scaleMask(c));
                // the bass
                uint8_t r = bassRoot(c), code = (uint8_t)sty_->bass[step], q = sty_->ch[c].qual;
                int n = -1;
                switch (code) {
                    case 'R': n = r; break;
                    case 'r': n = r + 12; break;
                    case '5': n = r + 7; break;
                    case '3': n = r + qualIv[q][1]; break;
                    case '6': n = r + 9; break;
                    case '7': n = r + 10; break;
                    case 'L': { static const uint8_t alb[4] = {0, 2, 1, 2}; n = voicing_[alb[step & 3]] - 12; break; }
                    case 'W': {
                        uint8_t beat = step >> 2, nr = bassRoot(next);
                        if (beat == 0) n = r;
                        else if (beat == 1) n = r + (rndN(2) ? qualIv[q][1] : 2);
                        else if (beat == 2) n = r + (rndN(3) ? 7 : qualIv[q][1]);
                        else n = rndN(2) ? nr - 1 : nr + 1;
                        break;
                    }
                    case '-': bass[at].rest = true; break;
                    default: break;
                }
                if (n >= 0) {
                    while (n < NOTE_LO) n += 12;
                    while (n > NOTE_HI) n -= 12;
                    bass[at].note = n;
                }
                // the chords
                if (comp[step] == 'x') {
                    if (chordsOne) {
                        uint8_t a = (uint8_t)(voicing_[1] - voicing_[0]), b = (uint8_t)(voicing_[2] - voicing_[0]);
                        std::string arp = "arp:" + std::to_string(a) + "," + std::to_string(b);
                        if (arp != lastArp) ch[0][at].pre += (ch[0][at].pre.empty() ? "" : " ") + arp;
                        lastArp = arp;
                        ch[0][at].note = voicing_[0] + 12;
                    } else {
                        for (int i = 0; i < 3; i++) ch[i][at].note = voicing_[i];
                    }
                } else if (comp[step] == '-') {
                    for (auto& cv : ch) cv[at].rest = true;
                }
                // drums: a note for a tick, then a rest (as musicgen's short hits)
                if (drums) {
                    const char* hit = nullptr;
                    if (lastBar && step >= 12 && sty_->snare[4] == 'x' && (step == 12 || step == 14 || step == 15)) hit = "snare";
                    else if (sty_->kick[step] == 'x') hit = "kick";
                    else if (sty_->snare[step] == 'x') hit = "snare";
                    else if (sty_->hat[step] == 'x') hit = bar == 0 && step == 0 ? "open_hat" : "hat";
                    if (hit) {
                        if (lastDrum != hit) drum[at].pre = std::string("@") + hit;
                        lastDrum = hit;
                        drum[at].note = std::string(hit) == "kick" ? 65 : std::string(hit) == "snare" ? 72 : 104;
                    } else if (at && drum[at - 1].note >= 0) {
                        drum[at].rest = true;
                    }
                }
            }
        }
    }
    res.key = key_; res.lo = sty_->lo; res.hi = sty_->hi; res.barsDone = (unsigned)secs.size() * nbars_;
    res.sectionLen = nbars_; res.blues = sty_->form == FORM_BLUES;

    // the text
    std::ostringstream t;
    static const char* keys[12] = {"c", "c#", "d", "eb", "e", "f", "f#", "g", "ab", "a", "bb", "b"};
    std::string name = o.name.empty() ? res.genre : o.name;
    char fptText[16];
    snprintf(fptText, sizeof fptText, "%.2f", fpt);
    t << "; made by tune45 create: " << res.genre << " in " << keys[key_] << ", " << bpm << " bpm, "
      << res.barsDone << " bars, seed " << o.seed << "\n";
    std::vector<int> used = {sty_->lead, sty_->bassi, sty_->chords};
    std::vector<int> emitted;
    for (int i : used) {
        bool seen = false;
        for (int e : emitted) if (e == i) seen = true;
        if (seen) continue;
        emitted.push_back(i);
        const Inst& in = insts[i];
        char line[160];
        snprintf(line, sizeof line, "instrument %-12s %-5s %02x %02x %04x %d 0", in.name, in.wave, in.ad, in.sr, in.pw, in.vib);
        t << line;
        if (in.pwm) t << " pwm=" << in.pwm;
        if (in.vdelay) t << " vdelay=" << in.vdelay;
        t << "\n";
    }
    if (drums) {
        t << "instrument kick         pulse 08 00 0800 0 0 slide=-47\n"
          << "instrument snare        noise 08 00 0800 0 0\n"
          << "instrument hat          noise 02 00 0800 0 0\n"
          << "instrument open_hat     noise 06 00 0800 0 0\n";
    }
    t << "\ntune " << name << " " << fptText << " " << (o.loop ? "loop" : "once") << " " << barTicks << "\n";
    unsigned spb = 16;
    t << "v1:" << voiceText(lead, stepTicks, spb) << "\n";
    t << "v2:" << voiceText(bass, stepTicks, spb) << "\n";
    t << "v3:" << voiceText(ch[0], stepTicks, spb) << "\n";
    int vno = 4;
    if (drums) t << "v" << vno++ << ":" << voiceText(drum, stepTicks, spb) << "\n";
    if (!chordsOne) {
        t << "v" << vno++ << ":" << voiceText(ch[1], stepTicks, spb) << "\n";
        t << "v" << vno++ << ":" << voiceText(ch[2], stepTicks, spb) << "\n";
    }
    res.text = t.str();
    return res;
}

CreateResult tuneCreate(const CreateOptions& o) {
    CreateOptions opt = o;
    if (!opt.seed) opt.seed = (uint32_t)time(nullptr) & 0xFFFF;
    if (!opt.seed) opt.seed = 1;
    Composer c(opt.seed);
    return c.run(opt);
}

// musicgen's check_songs.py on a created tune's melody: every note in the
// scale (with the chord's changes) or the chord; most notes on beats 1 and
// 3 chord tones; mostly steps, no leap over an octave; the range; the end on
// the tonic. "" when it holds, else what does not
std::string tuneCheck(const CreateResult& r) {
    unsigned notes = 0, outside = 0, strong = 0, strongCt = 0, steps = 0, moves = 0, maxLeap = 0, rng = 0;
    int prev = -1, last = -1;
    // the last section's cadence: bar 8 (blues 11), beat 1
    size_t cadence = r.melody.size() - r.sectionLen * 16 + (r.blues ? 160 : 112);
    for (size_t s = 0; s < r.melody.size(); s++) {
        if (s % (r.sectionLen * 16) == 0) prev = -1;   // (a section of its own, as check_songs.py)
        int n = r.melody[s];
        if (s == cadence) last = n;
        if (n < 24) continue;
        unsigned bar = (unsigned)(s / 16) % r.sectionLen;
        if (r.blues && (bar == 4 || bar == 5)) { prev = n; continue; }   // (the call over the IV chord)
        unsigned pcl = (unsigned)((n - r.key + 120) % 12);
        notes++;
        if (!(((r.scaleMask[s] | r.chordMask[s]) >> pcl) & 1)) outside++;
        if (s % 8 == 0) { strong++; if ((r.chordMask[s] >> pcl) & 1) strongCt++; }
        if (n < r.lo || n > r.hi) rng++;
        if (prev >= 0) {
            unsigned d = (unsigned)std::abs(n - prev);
            moves++;
            if (d <= (r.blues ? 3u : 2u)) steps++;   // (the blues scale's steps include minor thirds)
            if (d > maxLeap) maxLeap = d;
        }
        prev = n;
    }
    std::string why;
    if (outside) why += std::to_string(outside) + " notes outside the scale; ";
    if (maxLeap > 12) why += "a leap of " + std::to_string(maxLeap) + "; ";
    if (rng) why += std::to_string(rng) + " notes out of range; ";
    if (last < 24 || (last - r.key + 120) % 12 != 0) why += "its cadence is not on the tonic; ";
    if (strongCt * 100 < 80 * strong) why += "strong beats " + std::to_string(strongCt) + "/" + std::to_string(strong) + " chord tones; ";
    if (steps * 100 < 40 * moves) why += std::to_string(100 * steps / (moves ? moves : 1)) + "% steps; ";
    (void)notes;
    return why;
}
