// TuneMod.hpp — tune45 import of a MOD (ProTracker and its kin): the file
// read, and each sample weighed for the SID: which an instrument of
// tune45's can play (a waveform, a pulse width, the filter, an envelope)
// and which are too far from the SID to imitate, to keep as samples.
//
// A MOD: a 20-byte title; 31 sample headers of 30 bytes (15 in an old
// Soundtracker file, which has no tag) — name (22), length in words,
// finetune, volume 0-64, loop start and loop length in words (1: no
// loop); the song's length, a restart byte, 128 positions (the pattern at
// each), a 4-byte tag at 1080 (M.K., 4CHN, 6CHN, 8CHN, FLT4 ...); the
// patterns, 64 rows of a 4-byte cell a channel (the sample, 1-31; the
// Amiga period, the pitch; an effect and its byte); then the samples,
// 8-bit signed.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct ModSample {
    std::string name;
    std::vector<int8_t> data;
    int finetune = 0, volume = 64;
    double transpose = 0;           // semitones it sounds above a MOD sample at a period (XM, S3M, IT: their tuning)
    uint32_t loopStart = 0, loopLen = 0;   // bytes
    bool looped() const { return loopLen > 2 && loopStart + loopLen <= data.size(); }
    // its tuning in semitones (finetune in eighths, and the transpose)
    double tuneSemis() const { return finetune / 8.0 + transpose; }
};

// A cell as a MOD has it; the other formats' readers translate theirs to
// it: a note as the Amiga period it would have (C-2, as ProTracker names
// it, is 428; a note n semitones higher 428 / 2^(n/12), rounded), and the
// effect as ProTracker's (0-F, E0-EF), and 0x10: the speed, any (1-255;
// F takes 32 and over as the BPM)
struct ModCell {
    uint8_t sample = 0;             // 0: none (the channel's last)
    uint16_t period = 0;            // 0: no note
    uint8_t effect = 0, param = 0;
};

struct ModFile {
    std::string title, tag;         // tag: a MOD's, or the format (XM, S3M, IT)
    int channels = 4;
    int speed = 6, bpm = 125;       // at the start (a MOD's: 6 and 125)
    int songLen = 0;
    std::vector<uint8_t> order;     // the pattern at each position
    std::vector<std::vector<ModCell>> patterns;   // rows * channels each (row-major)
    std::vector<int> rows;          // each pattern's rows (a MOD's: 64)
    int rowsOf(size_t p) const { return p < rows.size() ? rows[p] : 64; }
    std::vector<ModSample> samples; // 31 (or 15)
};

// a MOD by its tag (at 1080), or by its name (.mod: an old 15-sample file)
bool isMod(const std::string& path, const std::vector<uint8_t>& file);
bool readMod(const std::vector<uint8_t>& file, ModFile& m, std::string& error);

// What the song does with a sample, and what the sample is
struct ModSampleReport {
    int index = 0;                  // from 1
    // the song's use of it
    int notes = 0;
    std::vector<uint16_t> periods;  // the distinct periods it is played at
    int channelsMask = 0;
    int vibrato = 0, porta = 0, arpeggio = 0, volume = 0, offset = 0;   // notes with those effects
    double medianPeriod = 428;
    // the sound (at the period it is played at most, Amiga PAL clock)
    bool tonal = false;
    double clarity = 0;             // the pitch's autocorrelation peak (0-1)
    double periodSamples = 0;       // its fundamental, in samples
    double aperiodic = 0;           // the energy not repeating at that period (0-1)
    double fit = 0;                 // how near the best SID waveform's harmonics are (cosine, 0-1)
    double stability = 1;           // the harmonics early against late (cosine)
    double pitchCents = 0;          // the pitch's spread over the sample
    int swells = 0;                 // loudness peaks after the first (several hits, tremolo)
    double attackMs = 0, decayMs = 0, sustain = 0;
    double centroidHz = 0;          // (an unpitched sound's brightness)
    std::string kind;               // "tone", or kick / snare / hat / percussion
    // the SID's nearest
    std::string wave;               // tri, saw, pulse
    int pulseWidth = 0x800;
    double lpTimes = 0;             // the low-pass cutoff, times the fundamental (0: none)
    int ad = 0, sr = 0;
    // the verdict: 0 easy .. 100 hardest
    int score = 0;
    std::string verdict;            // sid, sid~ (approximately), sample, unused, empty
    bool chosen = false;            // the verdict is the user's (a choices file)
    bool fitted = false;            // made a SID instrument so the kept samples fit in 64 KB
    std::vector<std::string> reasons;
};

std::vector<ModSampleReport> analyseMod(const ModFile& m);

// the rate a period plays a sample at (Hz, the Amiga's PAL clock)
double modRate(double period);

// the SID's stand-in for a sample, as a tune45 instrument line named name:
// the waveform, pulse width, low-pass and envelope the report found (for a
// drum: noise, or a falling pulse for a kick); and the MIDI note it plays
// for the sample at period
std::string modInstrumentLine(const ModSampleReport& r, const std::string& name);
int modSidNote(const ModSampleReport& r, double period);

// The user's choices (tune45 import --audition asks them): a line a sample,
// its number and "sid" or "sample" (; a comment)
bool readModChoices(const std::string& path, std::map<int, std::string>& c, std::string& error);
bool writeModChoices(const std::string& path, const ModFile& m, const std::map<int, std::string>& c, std::string& error);
// the choices as the verdicts (chosen set, the measured verdict kept in the reasons)
void applyModChoices(std::vector<ModSampleReport>& r, const std::map<int, std::string>& c);

// The song as tune45 text: tune45 import song.mod
struct ModImportOptions {
    std::string name;               // the tune's (a C identifier)
    std::string source;             // the MOD's file name (the text's first line)
    std::string samplesDir;         // where the kept samples' WAVs are written
    std::string samplesRef;         // that directory as the text names it (from the text's own)
    std::string choices;            // the choices file the verdicts came from ("": measured)
    std::map<int, double>* keptBytes = nullptr;   // out: each kept sample's bytes in the bank, about
                                                  // (its notes, at the rate tune45 will choose)
};
// The song's rows in the order it plays them (position jumps, pattern breaks,
// loops and delays followed; it stops where it would play a position again,
// the tune looping there) a tick each, at speed * 125 / BPM frames a tick
// (tempo: where they change). A channel is a SID voice for its notes on
// samples whose verdict is sid or sid~ (the report's instrument, hr=1), and
// a digi voice for those kept as samples (a WAV a note it is played at, a
// loop unrolled to the longest it is held); a channel with both has one of
// each. Arpeggios, tone portamento (a glide), vibrato (the instrument's),
// note cuts and C00 carry over; what does not is counted in a comment.
// Written: the WAVs; "" with error set.
std::string modImport(const ModFile& m, const std::vector<ModSampleReport>& r, const ModImportOptions& o,
                      std::vector<std::string>& written, std::string& error);

// the report as text: the song, a line a sample, the reasons, a summary
std::string modReport(const ModFile& m, const std::vector<ModSampleReport>& r);

// each sample of verdict "sample" (all the used ones with all) as a WAV in
// dir: 8-bit, at the rate of the period it is played at most, its loop in
// a smpl chunk; the files written listed in written
bool extractModSamples(const ModFile& m, const std::vector<ModSampleReport>& r, const std::string& dir, bool all,
                       std::vector<std::string>& written, std::string& error);
