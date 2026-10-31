// TunePack.hpp — a tune's data coded (#tune45): the player reads it as it
// plays, no copy unpacked. The streams' events Huffman-coded bit by bit, a
// code for each kind of field (22: the op or note, durations, instruments,
// ...); their repeats found again across the voices as patterns, called by
// number and nested two deep (a pattern's body may call a pattern of its
// own), transposed. Each stream and pattern begins on a byte: the player
// keeps a voice's place as a byte's offset and a bit (a pattern's return,
// the loop point). The instruments: a byte for each group of their fields,
// its entry in the group's dictionary (an instrument read in a few copies).
//
// The data: the header as a plain tune's (flags bit 5: coded), the voices'
// stream offsets (and with digi voices the sample table's), then the code
// tables' offset and the pattern table's (2 bytes a pattern: its body's
// offset); the wave tables and the sample table as a plain tune's; the
// code tables (each: its longest code's length L, 0 for a single symbol
// then that symbol; else the codes of each length 1..L, then the symbols
// in canonical order); the instruments' groups of fields (kGroups: the
// envelope's, the pulse width's ...), each a dictionary of the values it
// takes (its entries, 0 for 256, then theirs); the instruments (byte 6-7:
// a byte a group each, its entry); the patterns' bodies (ending $87) and
// the streams. Calls: $86 n r, $8B n r t (the pattern's number, times,
// transposition: each in its own code).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tunepack {

// the contexts (the player's numbers)
enum Ctx {
    OP = 0, DUR, INST, LNOTE, ARPA, ARPB, GLS, GLK, TLO, THI, PWLO, PWHI, PWS, CLO, CHI, CS,
    PAT, REP, TR, DOP, DHIT, DVOL,
    NCTX
};
// an instrument's bytes in groups (the player's order): the envelope
// (waveform, AD, SR, gap, slide, hard restart), the pulse width and its
// sweep, the vibrato and its delay, the filter, the wave table, the cycle's
// limits (19-byte instruments)
constexpr int kGroups = 6;
constexpr int kGroupSize[kGroups] = {6, 3, 2, 4, 2, 2};
constexpr int kGroupField[19] = {0, 1, 2, 6, 9, 10, 3, 4, 7, 5, 8, 11, 12, 13, 14, 15, 16, 17, 18};
constexpr int kDepth = 2;            // patterns called within patterns: two deep at most
constexpr int kMaxLen = 15;          // the longest code (bits)

// The coded data of a tune's plain data (TuneCompiler::plainBlob), or empty
// when it cannot be (over 255 patterns, 256 entries in a group, 64 KB ...).
// stats: a line of what it found. The player's reader: tune_code.s45.
std::vector<uint8_t> pack(const std::vector<uint8_t>& plain, std::string* stats = nullptr);

// A reader of coded data (the model's): the next symbol at a place
struct Reader {
    const std::vector<uint8_t>* d = nullptr;
    uint16_t tab[NCTX] = {};
    uint16_t dict[kGroups] = {};     // each group's dictionary's entries
    void init(const std::vector<uint8_t>& data);
    // an instrument's bytes (isize of them) from its groups' entries
    void instrument(unsigned at, int isize, uint8_t* r) const;
    uint8_t byte(unsigned at) const { return at < d->size() ? (*d)[at] : 0xFF; }
    // the symbol in context c at (pos, bit), both moved past it
    uint8_t symbol(int c, uint16_t& pos, uint8_t& bit) const;
};

}  // namespace tunepack
