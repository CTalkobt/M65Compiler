// TuneCompiler.hpp — tune45's music: the text format, the player's data
//
// The text (pfest's music.txt format, which it accepts unchanged):
//   ; a comment                        a line ending in \ goes on
//   instrument <name> <wave> <ad> <sr> <pulse> <vibrato> <gap> [pwm=n] [vdelay=n] [slide=n]
//       wave: tri, saw, pulse, noise (or a hex control value); ad, sr,
//       pulse: hex; vibrato: depth; gap: ticks released before the next
//       event; pwm: pulse-width change a frame (signed); vdelay: frames
//       before the vibrato; slide: pitch change a frame (signed, 16ths of
//       the frequency register)
//   tune <name> <frames per tick> loop|once [ticks a bar]
//       frames per tick may have a fraction (6.25); with ticks a bar, the
//       "|" marks are checked
//   v1: ... v12:  a voice: note:ticks (c4 is MIDI 60; c#4, eb4), r:ticks a
//       rest, ~:ticks the note held longer, @name an instrument,
//       arp:4,7 / arp:off an arpeggio, | a bar line
// The data (lib/include/audio_tune.h, lib/stdlib_asm/tune.s45): a blob a
// tune: header, the instruments it uses, a byte stream a voice.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct TuneInstrument {
    std::string name;
    int line = 0;
    uint8_t wave = 0x40, ad = 0, sr = 0xF0;
    uint16_t pw = 0x800;
    uint8_t vib = 0, gap = 0;
    int8_t pwm = 0;
    uint8_t vdelay = 0;
    int8_t slide = 0;
};

struct TuneVoice {
    int line = 0;
    std::vector<uint8_t> stream;    // without the end byte
    unsigned ticks = 0;
    bool defaultInstrument = false; // plays before its first @instrument
    std::vector<int> instruments;   // file instruments it names, in order
};

struct Tune {
    std::string name;
    int line = 0;
    uint16_t fpt = 0x100;           // frames per tick, 8.8
    bool loop = false;
    unsigned barTicks = 0;
    std::vector<TuneVoice> voices;
};

struct TuneFile {
    std::vector<TuneInstrument> instruments;
    std::vector<Tune> tunes;
};

class TuneCompiler {
public:
    // Parse text (file names the diagnostics); false with errors() set
    bool parse(const std::string& text, const std::string& file, TuneFile& out);
    const std::vector<std::string>& errors() const { return errors_; }

    // The player's blob of a tune (its instruments renumbered: those it uses)
    static std::vector<uint8_t> blob(const TuneFile& f, const Tune& t);
    // The file's instruments a tune uses, as the blob numbers them
    static std::vector<int> usedInstruments(const TuneFile& f, const Tune& t);

    // MIDI note of "c4", "c#4", "eb4" (c4 = 60), or -1
    static int noteNumber(const std::string& tok);
    static std::string noteName(int n);

private:
    std::vector<std::string> errors_;
    void error(const std::string& file, int line, const std::string& msg);
};
