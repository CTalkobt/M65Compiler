// TuneCompiler.hpp — tune45's music: the text format, the player's data
//
// The text (pfest's music.txt format, which it accepts unchanged):
//   ; a comment                        a line ending in \ goes on
//   instrument <name> <wave> <ad> <sr> <pulse> <vibrato> <gap> [pwm=n] [vdelay=n] [slide=n]
//              [hr=n] [filter=lp|bp|hp|notch|lp+bp.. res=n cutoff=hex fsweep=n] [table=name]
//       wave: tri, saw, pulse, noise (or a hex control value); ad, sr,
//       pulse: hex; vibrato: depth; gap: ticks released before the next
//       event; pwm: pulse-width change a frame (signed); vdelay: frames
//       before the vibrato; slide: pitch change a frame (signed, 16ths of
//       the frequency register); hr: hard restart, frames (1-3) before the
//       next note the gate off and the release at its fastest; filter: the
//       chip's filter for its notes (res 0-15, cutoff hex 0-7ff, fsweep the
//       cutoff's change a frame); table: a wave table
//   sample <name> <file> [rate=hz] [gain=percent] [from=s] [len=s]
//       a sound (WAV, AU, MP3; others by ffmpeg) for the digi voices, made
//       8-bit at rate (default: the lowest that keeps it, at most 12000);
//       from, len: a part of the file (seconds; one hit of a kit)
//   d1: ... d4:  a digi voice (the audio DMA): name:ticks a sample,
//       a+b:ticks samples at once (tune45 mixes them into one), r:ticks
//       and ~:ticks (the sample rings on), | a bar line
//   wavetable <name> <step> ... [loop <n>]
//       a step a frame from the note's start: a waveform (tri, saw, pulse,
//       noise, a+b, or hex) and :semitones from the note (pulse:-12); after
//       the last, step n again (from 1) or the last held
//   tune <name> <frames per tick> loop|once [ticks a bar]
//       frames per tick may have a fraction (6.25); with ticks a bar, the
//       "|" marks are checked
//   v1: ... v12:  a voice: note:ticks (c4 is MIDI 60; c#4, eb4), r:ticks a
//       rest, ~:ticks the note held longer, @name an instrument,
//       arp:4,7 / arp:off an arpeggio, | a bar line; g4^3:ticks glides up
//       into the note from 3 semitones below (^-3 from above; ^3,2: a speed
//       of 2, 1/4 of the distance a frame; the default 3, 1/8)
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
    uint8_t hr = 0;                 // hard restart: frames before the next note
    uint8_t filt = 0;               // filter: mode bits 4-6 ($10 LP, $20 BP, $40 HP), resonance 0-3
    uint16_t cutoff = 0x400;
    int8_t fsweep = 0;
    int table = -1;                 // its wave table (TuneFile::tables), -1 none
};

struct TuneTable {
    std::string name;
    int line = 0;
    std::vector<std::pair<uint8_t, int8_t>> steps;   // a frame each: waveform, semitones
    int loop = -1;                  // the step to go on from after the last (-1: the last held)
};

struct TuneVoice {
    int line = 0;
    std::vector<uint8_t> stream;    // without the end byte
    unsigned ticks = 0;
    bool defaultInstrument = false; // plays before its first @instrument
    std::vector<int> instruments;   // file instruments it names, in order
    std::vector<std::pair<int, unsigned>> hits;   // a digi voice's: the hit, its tick
};

struct TuneSample {
    std::string name, path;
    int line = 0;
    uint32_t rate = 0;              // 0: the lowest that keeps it (at most 12000)
    int gain = 100;                 // per cent
    double from = 0, len = 0;       // the part of the file (seconds; len 0: to its end)
};

// A hit: samples sounding at once, mixed into one in the sample bank
struct TuneHit {
    std::vector<int> samples;       // TuneFile::samples, sorted
    uint32_t at = 0, len = 0, rate = 0;   // its place in the bank, bytes, Hz
};

struct Tune {
    std::string name;
    int line = 0;
    uint16_t fpt = 0x100;           // frames per tick, 8.8
    bool loop = false;
    unsigned barTicks = 0;
    std::vector<TuneVoice> voices;
    std::vector<TuneVoice> digi;    // d1: .. d4:
    unsigned ticks() const { return !voices.empty() ? voices[0].ticks : !digi.empty() ? digi[0].ticks : 0; }
};

struct TuneFile {
    std::vector<TuneInstrument> instruments;
    std::vector<TuneTable> tables;
    std::vector<TuneSample> samples;
    std::vector<TuneHit> hits;      // every hit the tunes play
    std::vector<uint8_t> bank;      // the hits' samples: 8-bit unsigned, at most 64 KB
    std::vector<Tune> tunes;
};

class TuneCompiler {
public:
    // an instrument's bytes in the data (header byte 5; 0 in a tune of
    // before the hard restart, filter and wave tables: 10)
    static constexpr uint8_t kInstrumentSize = 17;

    // Parse text (file names the diagnostics); false with errors() set
    // (and the samples read, mixed into the bank, beside file)
    bool parse(const std::string& text, const std::string& file, TuneFile& out);
    const std::vector<std::string>& errors() const { return errors_; }
    const std::vector<std::string>& warnings() const { return warnings_; }

    // The player's blob of a tune (its instruments renumbered: those it uses)
    static std::vector<uint8_t> blob(const TuneFile& f, const Tune& t);
    // The file's instruments a tune uses, as the blob numbers them
    static std::vector<int> usedInstruments(const TuneFile& f, const Tune& t);
    // The file's hits a tune plays, as the blob numbers them
    static std::vector<int> usedHits(const Tune& t);
    // A hit's name: its samples joined by +
    static std::string hitName(const TuneFile& f, int hit);
    // The audio DMA's rate register value of a rate in Hz (Hz * 2^24 / 40.5 MHz)
    static uint32_t dmaRate(uint32_t hz);

    // MIDI note of "c4", "c#4", "eb4" (c4 = 60), or -1
    static int noteNumber(const std::string& tok);
    static std::string noteName(int n);

private:
    std::vector<std::string> errors_, warnings_;
    void error(const std::string& file, int line, const std::string& msg);
    bool loadSamples(const std::string& file, TuneFile& f);
};
