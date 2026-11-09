// BasicPlay.cpp — BASIC 65's PLAY strings as tune45 text, then the tune
// player's data (TuneCompiler), for basic45 --compile (#369)
//
// As b65.src V920413's play_one_character reads a string: A-G a note (# and
// $ before it: sharp, flat), W H Q I S its length from then on (a whole note
// to a sixteenth), . the next note or rest dotted, R a rest, O T X U M P
// with a digit: octave, envelope, filter, volume, modulation, portamento;
// L back to the start; anything else ignored. A voice starts in octave 4
// with quarter notes and envelope 0 (sound_defaults). A tick is a 32nd note
// (a dotted sixteenth is 3), 3.125 frames at TEMPO 12 (a whole note 2
// seconds); the run time scales it for the TEMPO in force and puts the
// envelopes' current values (ENVELOPE) into the instruments.
//
// Not played yet (the tune player has no matching events: #474-#477): U, X,
// M, P (warned). Voices 4-6 are tune voices 7-9: SID 3, as BASIC's.

#include "BasicPlay.hpp"
#include "TuneCompiler.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <sstream>

namespace {

// sound_defaults' envelopes (atkmus, susmus, wavmus, pwhmus)
const uint8_t kAtk[10] = {0x09, 0xC0, 0x00, 0x05, 0x94, 0x09, 0x09, 0x09, 0x89, 0x09};
const uint8_t kSus[10] = {0x00, 0xC0, 0xF0, 0x50, 0x40, 0x21, 0x00, 0x90, 0x41, 0x00};
const uint8_t kWav[10] = {0x41, 0x21, 0x11, 0x81, 0x11, 0x21, 0x41, 0x41, 0x41, 0x11};
const uint8_t kPwh[10] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x02, 0x00};

const int kScale[7] = {9, 11, 0, 2, 4, 5, 7};   // A B C D E F G (scalen)

std::string hex(unsigned v, int digits) {
    static const char* d = "0123456789abcdef";
    std::string s(digits, '0');
    for (int i = digits - 1; i >= 0; i--, v >>= 4) s[i] = d[v & 15];
    return s;
}

}  // namespace

PlayTune convertPlay(const std::vector<std::optional<std::string>>& voices) {
    PlayTune r;
    std::ostringstream t;
    for (int e = 0; e < 10; e++)
        t << "instrument e" << e << " " << hex(kWav[e] & 0xFE, 2) << " " << hex(kAtk[e], 2) << " "
          << hex(kSus[e], 2) << " " << hex(kPwh[e] << 8, 3) << " 0 0\n";
    bool anyLoop = false, allLoop = true;
    std::set<char> unplayed;
    std::vector<std::string> lines(9);
    std::vector<int> lens(6, 0);           // each voice's ticks
    int looped = 0;                        // the voices with an L (RPLAY: on while the tune plays)
    int mask = 0;
    for (size_t v = 0; v < voices.size() && v < 6; v++) {
        if (!voices[v]) continue;
        mask |= 1 << v;
        const std::string& s = *voices[v];
        std::ostringstream o;
        int octave = 4, len = 8, sharp = 0, env = 0;
        bool dot = false, loop = false;
        char flag = 0;
        o << "@e0";
        for (unsigned char c : s) {
            if (c == 'L') { loop = true; break; }
            if (c == ' ') continue;
            if (c >= 'A' && c <= 'G') {
                int n = kScale[c - 'A'] + sharp + 12 * (octave + 1);
                int ticks = dot ? len * 3 / 2 : len;
                o << " " << TuneCompiler::noteName(n) << ":" << ticks;
                lens[v] += ticks;
                sharp = 0; dot = false; flag = 0;
                continue;
            }
            const char* lengths = "WHQIS";
            const char* l = c ? strchr(lengths, c) : nullptr;
            if (l) { len = 32 >> (l - lengths); continue; }
            if (c == 'R') {
                o << " r:" << (dot ? len * 3 / 2 : len);
                lens[v] += dot ? len * 3 / 2 : len;
                sharp = 0; dot = false; flag = 0;
                continue;
            }
            if (c == '.') { dot = true; continue; }
            if (strchr("OTXUMP", c) && c) { flag = (char)c; continue; }
            if (c == '#') { sharp = 1; continue; }
            if (c == '$') { sharp = -1; continue; }
            if (c >= '0' && c <= '9' && flag) {
                int d = c - '0';
                switch (flag) {
                case 'O': if (d >= 7) r.badValue = true; else octave = d; break;
                case 'T': if (d != env) { env = d; o << " @e" << d; } break;
                case 'X': if (d >= 2) r.badValue = true; else unplayed.insert('X'); break;
                default: unplayed.insert(flag); break;
                }
            }
            flag = 0;
        }
        if (loop) { anyLoop = true; looped |= 1 << v; } else allLoop = false;
        lines[v < 3 ? v : v + 3] = o.str();
    }
    if (!mask) { r.error = "PLAY without a string"; return r; }
    // (the tune's voices as long: the shorter ones end in rests; RPLAY uses
    // each one's own length)
    int most = 0;
    for (int l : lens) most = std::max(most, l);
    if (!most) { r.error = "PLAY: no note or rest to play"; return r; }
    t << "tune play 3.125 " << (anyLoop ? "loop" : "once") << "\n";
    for (int v = 0; v < 9; v++) {
        int bv = v < 3 ? v : v >= 6 ? v - 3 : -1;     // (tune voices 4-6: SID 2, unused)
        int have = bv >= 0 ? lens[bv] : 0;
        std::string line = lines[v];
        for (int rest = most - have; rest > 0; rest -= 255) line += " r:" + std::to_string(std::min(rest, 255));
        t << "v" << v + 1 << ": " << line << "\n";
    }
    if (anyLoop) {
        for (int l : lens)
            if (l && l != most) { r.warnings.push_back("PLAY: looping voices of different lengths loop together"); break; }
    }
    for (char c : unplayed) {
        const char* what = c == 'U' ? "U (volume, #474)" : c == 'X' ? "X (filter, #475)"
                         : c == 'M' ? "M (modulation, #476)" : "P (portamento, #477)";
        r.warnings.push_back(std::string("PLAY's ") + what + " is not played yet");
    }
    if (anyLoop && !allLoop) r.warnings.push_back("PLAY: L on some voices only: all of them loop");
    TuneCompiler tc;
    TuneFile f;
    if (!tc.parse(t.str(), "PLAY", f) || f.tunes.empty()) {
        r.error = "PLAY: " + (tc.errors().empty() ? std::string("not converted") : tc.errors()[0]);
        return r;
    }
    const Tune& tune = f.tunes[0];
    std::vector<uint8_t> blob = TuneCompiler::plainBlob(f, tune);
    std::vector<int> used = TuneCompiler::usedInstruments(f, tune);
    r.data.push_back((uint8_t)mask);
    r.data.push_back((uint8_t)used.size());
    r.data.push_back(blob[6]);
    r.data.push_back(blob[7]);
    r.data.push_back(blob[5]);
    for (int v = 0; v < 6; v++) {
        int l = (looped & (1 << v)) ? 0xFFFF : lens[v];
        r.data.push_back((uint8_t)l);
        r.data.push_back((uint8_t)(l >> 8));
    }
    for (int i : used) r.data.push_back((uint8_t)i);
    r.data.insert(r.data.end(), blob.begin(), blob.end());
    return r;
}
