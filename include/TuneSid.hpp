// TuneSid.hpp — tune45 import of a C64 tune (.sid): its player run on an
// emulated 6502 (SidFile.hpp), the SID registers it writes made tune45
// text. A tick a call of the player (a frame, or the CIA timer's rate);
// each voice's notes where its gate goes on (also off and on within one
// call: a hard restart) until it goes off; a note's instrument from its
// registers (waveform, AD, SR, pulse width and its sweep, the filter when
// the voice is routed through it) and its first frames (a waveform or a
// pitch that changes there: a wave table, as the C64's drums are made);
// its pitch after them an arpeggio (arp:, two or three notes a frame in
// turn) or legato notes (~e4: a pitch that moves and stays).
#pragma once
#include "SidFile.hpp"
#include <string>

struct SidImportOptions {
    std::string name;               // the tune's (a C identifier; default from the file's)
    std::string source;             // the .sid's file name (the text's first line)
    int song = 0;                   // from 1 (0: the file's start song)
    double seconds = 180;           // how long to run it (without a loop found)
    bool findLoop = true;           // where it plays again as before: a tune that loops there (|:)
    double search = 900;            //   looked for so far (seconds)
    bool patterns = true;           // its repeats as patterns
};

// The text, or "" with error set
std::string sidImport(const PsidFile& p, const SidImportOptions& o, std::string& error);
