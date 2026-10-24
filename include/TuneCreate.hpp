// TuneCreate.hpp — tune45 create: a tune composed in the text format, by
// the composer of examples/c/musicgen (ported): a style's chords and their
// Markov table, AABA form (12-bar blues), melodies from rhythm cells and a
// motif, voice-led chords, the style's bass line and drums
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct CreateOptions {
    std::string genre;          // pop, rock, blues, jazz, classical, ambient, chiptune ("" random)
    int bpm = 0;                // 0: the style's own
    int bars = 32;              // whole sections (8, blues 12)
    int key = -1;               // 0 = C .. 11 = B, -1 random
    int voices = 3;             // 3: lead, bass, chords (an arpeggio); 4: + drums; 6: + chord voices
    bool loop = true;
    uint32_t seed = 0;          // 0: from the time
    std::string name;           // the tune's ("" the genre's)
};

struct CreateResult {
    std::string text;           // instruments and the tune
    // for the checks (melody against the chords): per 16th step of the
    // whole tune, the melody (0 hold, 1 rest, else a note), the chord's
    // notes and the scale (masks of pitch classes, the key's own) and the key
    std::vector<uint8_t> melody;
    std::vector<uint16_t> chordMask, scaleMask;
    int key = 0, lo = 0, hi = 0;
    std::string genre, error;
    unsigned barsDone = 0, sectionLen = 8;
    bool blues = false;
};

CreateResult tuneCreate(const CreateOptions& o);
// examples/c/musicgen's checks of a melody: "" when they hold
std::string tuneCheck(const CreateResult& r);
const std::vector<std::string>& tuneGenres();
