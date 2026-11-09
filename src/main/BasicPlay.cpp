// BasicPlay.cpp — BASIC 65's PLAY strings as the tune player's streams, for
// basic45 --compile (#369, #478)
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
// U, X, M, P are the player's events $8D (through BASIC's voltab), $8E, $8F,
// $90 (#474-#477). The streams (<audio_tune.h>): a note $00-$7F n d (longer
// than 255 ticks: ties $82 d after it), a rest $80 d, an envelope $81 e; at
// the end $FE (L: from the start again) or $FF. Voices 4-6 are tune voices
// 7-9: SID 3, as BASIC's.

#include "BasicPlay.hpp"

#include <algorithm>
#include <cstring>

namespace {

// sound_defaults' envelopes (atkmus, susmus, wavmus, pwhmus)
const uint8_t kAtk[10] = {0x09, 0xC0, 0x00, 0x05, 0x94, 0x09, 0x09, 0x09, 0x89, 0x09};
const uint8_t kSus[10] = {0x00, 0xC0, 0xF0, 0x50, 0x40, 0x21, 0x00, 0x90, 0x41, 0x00};
const uint8_t kWav[10] = {0x41, 0x21, 0x11, 0x81, 0x11, 0x21, 0x41, 0x41, 0x41, 0x11};
const uint8_t kPwh[10] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x02, 0x00};

const int kScale[7] = {9, 11, 0, 2, 4, 5, 7};   // A B C D E F G (scalen)
const int kVol[10] = {0, 1, 3, 5, 7, 8, 10, 12, 14, 15};   // U's volumes (voltab)

// a note (op its number) or rest ($80) for so many ticks: at most 255 an
// event, ties after
void timed(std::vector<uint8_t>& s, uint8_t op, int ticks) {
    int first = std::min(ticks, 255);
    s.push_back(op);
    s.push_back((uint8_t)first);
    for (ticks -= first; ticks > 0; ticks -= 255) {
        s.push_back(0x82);
        s.push_back((uint8_t)std::min(ticks, 255));
    }
}

}  // namespace

PlayTune PlayBank::add(const std::vector<std::optional<std::string>>& voices) {
    PlayTune r;
    r.data.assign(13, 0);
    int mask = 0;
    for (size_t v = 0; v < voices.size() && v < 6; v++) {
        if (!voices[v]) continue;
        mask |= 1 << v;
        const std::string& str = *voices[v];
        std::vector<uint8_t> s = {0x81, 0};   // (envelope 0)
        int octave = 4, len = 8, sharp = 0, env = 0;
        bool dot = false, loop = false, any = false;
        char flag = 0;
        for (unsigned char c : str) {
            if (c == 'L') { loop = true; break; }
            if (c == ' ') continue;
            if (c >= 'A' && c <= 'G') {
                int n = kScale[c - 'A'] + sharp + 12 * (octave + 1);
                timed(s, (uint8_t)std::clamp(n, 0, 127), dot ? len * 3 / 2 : len);
                any = true; sharp = 0; dot = false; flag = 0;
                continue;
            }
            const char* lengths = "WHQIS";
            const char* l = c ? strchr(lengths, c) : nullptr;
            if (l) { len = 32 >> (l - lengths); continue; }
            if (c == 'R') {
                timed(s, 0x80, dot ? len * 3 / 2 : len);
                any = true; sharp = 0; dot = false; flag = 0;
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
                case 'T': if (d != env) { env = d; s.push_back(0x81); s.push_back((uint8_t)d); } break;
                case 'X': if (d >= 2) r.badValue = true; else { s.push_back(0x8E); s.push_back((uint8_t)d); } break;
                case 'U': s.push_back(0x8D); s.push_back((uint8_t)kVol[d]); break;
                case 'M': s.push_back(0x8F); s.push_back((uint8_t)d); break;
                case 'P': s.push_back(0x90); s.push_back((uint8_t)d); break;
                default: break;
                }
            }
            flag = 0;
        }
        s.push_back(loop && any ? 0xFE : 0xFF);   // (an L with nothing before it: the voice ends)
        size_t at = kStreams + streams_.size();
        if (at + s.size() > 0xFFFF) { r.error = "PLAY: the program's strings are over 64 KB"; return r; }
        streams_.insert(streams_.end(), s.begin(), s.end());
        r.data[1 + 2 * v] = (uint8_t)at;
        r.data[2 + 2 * v] = (uint8_t)(at >> 8);
    }
    if (!mask) { r.error = "PLAY without a string"; return r; }
    r.data[0] = (uint8_t)mask;
    return r;
}

std::vector<uint8_t> PlayBank::blob() const {
    std::vector<uint8_t> b(kHeader, 0);
    b[0] = 0;                       // (plays once: a voice's L is its $FE)
    b[1] = 9;
    b[2] = 0x20;                    // 3.125 frames a tick (8.8)
    b[3] = 0x03;
    b[4] = 10;
    b[5] = kInstrument;
    b[6] = (uint8_t)kHeader;
    b[7] = 0;
    for (int v = 0; v < 9; v++) {   // (each voice empty at first)
        b[8 + 2 * v] = (uint8_t)kStreams;
        b[9 + 2 * v] = (uint8_t)(kStreams >> 8);
    }
    for (int e = 0; e < 10; e++) {
        uint8_t in[kInstrument] = {(uint8_t)(kWav[e] & 0xFE), kAtk[e], kSus[e], 0, kPwh[e]};
        b.insert(b.end(), in, in + kInstrument);
    }
    b.insert(b.end(), streams_.begin(), streams_.end());
    return b;
}
