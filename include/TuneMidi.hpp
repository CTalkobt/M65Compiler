// TuneMidi.hpp — tune45 import: a Standard MIDI File as tune45 text
#pragma once
#include <string>
#include <vector>

struct MidiImportOptions {
    std::string name = "song";      // the tune's
    int voices = 3;                 // monophonic voices to make (1-12)
    std::vector<int> sources;       // tracks (format 1) or channels (format 0), from 1; empty: the first with notes
    bool channels = false;          // sources are channels, even in a format 1 file
    bool drums = false;             // channel 10 too
    int ticksPerBeat = 4;           // the grid: 4 = 16ths
    int transpose = 0;
    bool loop = false;
    std::string instruments;        // instrument lines to use (a voice's: gm<program>, else voice<n>)
};

// The text, or "" with error set
std::string midiImport(const std::vector<unsigned char>& file, const MidiImportOptions& o, std::string& error);
