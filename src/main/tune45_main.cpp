// tune45 — music for the tune player (<audio_tune.h>)
//
//   tune45 convert music.txt -o music.c     C source and header (or --bin, --asm)
//   tune45 list music.txt                   each tune's size and length
//   tune45 play music.txt [--tune name]     on the PC (reSIDfp), or a .wav
//   tune45 sample sound.pak --rate 8000     sampled sound as the audio DMA plays it
// (create and import: see doc/bin/tune45.md)
#include "TuneCompiler.hpp"
#include "TunePack.hpp"
#include "TunePlayer.hpp"
#include "TuneAudio.hpp"
#include "TuneCreate.hpp"
#include "TuneMidi.hpp"
#include "TuneMod.hpp"
#include "TuneTracker.hpp"
#include "TuneSid.hpp"
#include "BasicTooling.hpp"
#include "Pack45.hpp"
#include "PackAudio.hpp"
#include <csignal>
#include <climits>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>     // (readlink, rmdir: not pulled in by other headers with musl)
#include <thread>
#include <chrono>
#include "Version.hpp"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include "Logger.hpp"
#include <sstream>
#include <algorithm>
#include <map>
#include <cmath>
#include <ctime>
#include <cstdlib>

namespace {

void usage(std::ostream& o) {
    o << "Usage: tune45 <command> [options] <music.txt>\n"
         "Commands:\n"
         "  convert   the player's data: -o music.c (C source and music.h), --bin -o music.bin\n"
         "            (one blob for far memory; music.h gives each tune's offset), --asm -o music.s45\n"
         "  list      each tune: voices, bytes, ticks, seconds (PAL / NTSC), instruments\n"
         "  play      play the tunes on the PC (reSIDfp: four SIDs, stereo) through pw-play,\n"
         "            paplay or aplay, or into a .wav file\n"
         "  create    compose a tune (the text format): --genre, --tempo, --length, ...\n"
         "  import    a Standard MIDI File, a MOD, XM, S3M, IT or C64 .sid as text (convert and play take a .mid too)\n"
         "  sample    play sampled sound as the MEGA65's audio DMA does: 8-bit samples, packed\n"
         "            by pack45 or as they are (--rate), or a WAV, AU, MP3 (MP4 ... by ffmpeg)\n"
         "            made so first (as pack45 --sound)\n"
         "Options:\n"
         "  -o <file>          output file\n"
         "  --bin, --asm       convert: a binary blob / ca45 source instead of C\n"
         "  --coded            each tune's data coded where that is smaller (the player reads it as it\n"
         "                     plays, nothing unpacked; the program links its reader, tune_codec: about\n"
         "                     1.7 KB, and more time a frame); --coded-all: every tune's (default: plain)\n"
         "  --header <file>    convert: the header's name (default: the output's, .h)\n"
         "  --load-at <addr>   convert --bin: a 2-byte load address first (a KERNAL LOAD skips it)\n"
         "  --tune <name>      play: that tune (again for more; default: all, in order)\n"
         "  --loops <n>        play: times through a looping tune (default 1)\n"
         "  --wav <file>       play: into a WAV file instead of the speakers\n"
         "  --ntsc             play: an NTSC machine (60 Hz, its SID clock)\n"
         "  --6581             play: the MOS 6581 SID (default: the 8580)\n"
         "  --rate <hz>        sample: the samples' rate (a sound file: default the rate\n"
         "                     pack45 --suggest-rate names)\n"
         "  --max-rate <hz>    sample: a sound file: the highest rate to suggest\n"
         "  --unsigned         sample: unsigned samples (offset by 128; default signed)\n"
         "  --samples <file>   convert --bin: the digi voices' samples (to load into chip RAM)\n"
         "  --samples-at <a>   trace: where the samples are (tune_samples)\n"
         "  --digi-first <n>   trace: the digi voices' first channel (tune_digi_channels)\n"
         "  --mono             play: SIDs 1-2 only (without the mirror on SIDs 3-4)\n"
         "  --emulator         play: on the MEGA65 in xemu (a player program with the tunes)\n"
         "  --emulator-path <f> the xemu-xmega65 binary (default: on PATH)\n"
         "  --send             play: on a MEGA65 (etherload -r, else m65 -F -r)\n"
         "  --send-cmd <cmd>   the command for --send; {prg} is the program\n"
         "  --watch            play: again whenever the text file changes (Ctrl-C stops)\n"
         "  --prg <file>       play: keep the player program (--emulator / --send) there\n"
         "  --genre <g>        create: pop, rock, blues, jazz, classical, ambient, chiptune\n"
         "                     (default: one at random)\n"
         "  --tempo <t>        create: beats a minute, or slow / medium / fast (default: the genre's)\n"
         "  --length <n>       create: bars (rounded up to whole sections of 8, blues 12), or\n"
         "                     seconds as 60s (default 32 bars)\n"
         "  --voices <n>       create: 3 lead, bass, chords as an arpeggio (default); 4 and drums;\n"
         "                     6 three chord voices\n"
         "  --seed <n>         create: the same tune again (the seed is in the text's first line)\n"
         "  --key <k>          create: c, c#, d, eb ... b (default: at random)\n"
         "  --once, --loop     create: the tune ends / loops (default: loop)\n"
         "  --name <n>         create: the tune's name (default: the genre)\n"
         "  --append <file>    create: add the tune (and its instruments) to a music file\n"
         "  --play             create: play it afterwards (play's options apply)\n"
         "  --tracks <a,b,..>  import: the tracks to make voices of (default: the first with notes)\n"
         "  --channels [a,..]  import: channels instead of tracks (a format 0 file: always)\n"
         "  --drums            import: channel 10 too\n"
         "  --ticks-per-beat <n> import: the grid, ticks a quarter note (default 4: 16ths)\n"
         "  --transpose <n>    import: semitones\n"
         "  --instruments <f>  import: instrument lines (voice1.., gm<program> for a program change)\n"
         "  --report           import of a MOD: each sample weighed for the SID (an instrument, or kept as a sample)\n"
         "  --extract <dir>    import of a MOD: the samples too far from the SID as WAVs (--all: every used one)\n"
         "  --audition [n]     import of a MOD: each sample (or n): the original, the SID's stand-in, then both\n"
         "                     (the original left, the SID right), asking sid or sample after each (at a\n"
         "                     terminal, or --ask); --wav <file> to a file\n"
         "  --song <n>, --seconds <s>  import of a .sid: the song (default its start song), how long (180) when\n"
         "                     it has no loop: where its registers play again as before it loops (|:)\n"
         "  --search <s>       import of a .sid: how far to look for its loop (900 s)\n"
         "  --no-loop          import of a .sid: no loop looked for: --seconds of it, played once\n"
         "  --no-patterns      import of a .sid: its repeats written out (no patterns)\n"
         "  --choices <file>   import of a MOD: the choices --audition saves (default: beside the MOD,\n"
         "                     .choices); with --report and --extract: the verdicts\n"
         "  -v ... -vvvv       log to stderr ([tune45:INFO] ...): the tunes read, the files written,\n"
         "                     the player built (-v); each tune and the commands run (-vv); -v0 off\n"
         "  -V, --version      version\n"
         "  -h, --help         this help\n"
         "The text format: doc/bin/tune45.md (pfest's music.txt format).\n";
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

std::string stem(const std::string& path) {
    size_t slash = path.find_last_of('/');
    size_t dot = path.find_last_of('.');
    return dot != std::string::npos && (slash == std::string::npos || dot > slash) ? path.substr(0, dot) : path;
}

std::string baseName(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

bool isIdent(const std::string& s) {
    if (s.empty() || std::isdigit((unsigned char)s[0])) return false;
    for (char c : s) if (!std::isalnum((unsigned char)c) && c != '_') return false;
    return true;
}

// 16 a line: lead starts a line, sep separates, lineEnd ends all but the last
std::string bytes(const std::vector<uint8_t>& b, const char* lead, const char* sep, const char* lineEnd) {
    std::string s;
    char buf[8];
    for (size_t k = 0; k < b.size(); k++) {
        if (k % 16 == 0) s += (k ? std::string(lineEnd) + "\n" : "") + std::string(lead);
        snprintf(buf, sizeof buf, "%s%u", k % 16 ? sep : "", b[k]);
        s += buf;
    }
    return s;
}

int convert(const TuneFile& f, const std::string& src, const std::string& outPath, const std::string& mode,
            std::string header, long loadAt, const std::string& samplesPath) {
    for (const auto& t : f.tunes)
        if (!isIdent(t.name)) {
            std::cerr << src << ":" << t.line << ": error: a tune's name must be a C identifier: " << t.name << "\n";
            return 1;
        }
    if (header.empty()) header = stem(outPath) + ".h";
    std::string guardNote = "/* made by tune45 from " + baseName(src) + " - do not edit */\n";
    std::ofstream h(header);
    if (!h) { std::cerr << "tune45: cannot write " << header << "\n"; return 1; }
    h << guardNote << "#pragma once\n\n#define NTUNES " << f.tunes.size() << "\n";
    for (size_t k = 0; k < f.tunes.size(); k++) h << "#define TUNE_" << upper(f.tunes[k].name) << " " << k << "\n";
    // each tune's ticks once through (tune_seek: a looping tune's
    // position), and a bar's (tune_queue)
    for (const auto& t : f.tunes) {
        h << "#define TUNE_" << upper(t.name) << "_TICKS " << t.ticks() << "\n";
        if (t.barTicks) h << "#define TUNE_" << upper(t.name) << "_BAR " << t.barTicks << "\n";
    }
    // the digi voices' samples: one bank, in chip RAM (tune_samples)
    if (!f.bank.empty()) h << "#define TUNE_SAMPLES_SIZE " << f.bank.size() << "\n";
    // coded tunes: the player's reader of them linked and set up wherever
    // the program starts a tune (tune_codec before it)
    bool coded = false;
    for (const auto& t : f.tunes) coded |= (TuneCompiler::blob(f, t)[0] & 0x20) != 0;
    if (coded)
        h << "\n/* coded tunes: the player reads them with tune_codec's routines, set up\n"
             "   wherever a tune starts */\n"
             "#include <audio_tune.h>\n"
             "#define tune_play(t) (tune_codec(), tune_play(t))\n"
             "#define tune_play_far(a) (tune_codec(), tune_play_far(a))\n"
             "#define tune_queue(t, b) (tune_codec(), tune_queue((t), (b)))\n"
             "#define tune_queue_far(a, b) (tune_codec(), tune_queue_far((a), (b)))\n";

    if (mode == "bin") {
        // [tunes][offset (2) each] then the blobs; offsets from the start
        std::vector<uint8_t> out(1 + 2 * f.tunes.size(), 0);
        out[0] = (uint8_t)f.tunes.size();
        h << "\n/* " << baseName(outPath) << ": tune_play_far(<its address> + TUNE_<NAME>_AT) */\n";
        for (size_t k = 0; k < f.tunes.size(); k++) {
            size_t at = out.size();
            out[1 + 2 * k] = (uint8_t)(at & 0xFF);
            out[2 + 2 * k] = (uint8_t)(at >> 8);
            auto b = TuneCompiler::blob(f, f.tunes[k]);
            out.insert(out.end(), b.begin(), b.end());
            char buf[16];
            snprintf(buf, sizeof buf, "0x%04zx", at);
            h << "#define TUNE_" << upper(f.tunes[k].name) << "_AT " << buf << "\n";
        }
        if (out.size() > 0xFFFF) { std::cerr << "tune45: over 64 KB of tunes\n"; return 1; }
        if (!f.bank.empty()) {
            // the samples: their own file, to load into chip RAM (tune_samples)
            if (samplesPath.empty()) { std::cerr << "tune45: the tunes have digi voices: --samples <file> for their samples\n"; return 1; }
            std::ofstream so(samplesPath, std::ios::binary);
            if (loadAt >= 0) {
                char la[2] = {(char)(loadAt & 0xFF), (char)((loadAt >> 8) & 0xFF)};
                so.write(la, 2);
            }
            so.write((const char*)f.bank.data(), (std::streamsize)f.bank.size());
            if (!so) { std::cerr << "tune45: cannot write " << samplesPath << "\n"; return 1; }
            h << "/* " << baseName(samplesPath) << ": in chip RAM, not across a 64 KB boundary: tune_samples(<its address>) */\n";
        }
        h << "#define TUNES_SIZE " << out.size() << "\n";
        std::ofstream o(outPath, std::ios::binary);
        if (loadAt >= 0) {      // a load address first, as a KERNAL LOAD reads it
            char la[2] = {(char)(loadAt & 0xFF), (char)((loadAt >> 8) & 0xFF)};
            o.write(la, 2);
        }
        o.write((const char*)out.data(), (std::streamsize)out.size());
        if (!o) { std::cerr << "tune45: cannot write " << outPath << "\n"; return 1; }
        return 0;
    }
    h << "\n";
    for (const auto& t : f.tunes)
        h << "extern const unsigned char tune_" << t.name << "[" << TuneCompiler::blob(f, t).size() << "];\n";
    h << "extern const unsigned char *const tunes[NTUNES];   /* tune_play(tunes[TUNE_...]) */\n";
    if (!f.bank.empty())
        h << "extern const unsigned char tune_sample_bank[TUNE_SAMPLES_SIZE];   /* tune_samples((unsigned int)tune_sample_bank) */\n";
    std::ofstream o(outPath);
    if (!o) { std::cerr << "tune45: cannot write " << outPath << "\n"; return 1; }
    if (mode == "asm") {
        o << "; made by tune45 from " << baseName(src) << " - do not edit\n\n.segment \"data\"\n";
        for (const auto& t : f.tunes) o << ".global _tune_" << t.name << "\n";
        o << ".global _tunes\n";
        for (const auto& t : f.tunes) o << "\n_tune_" << t.name << ":\n" << bytes(TuneCompiler::blob(f, t), "    .byte ", ", ", "") << "\n";
        o << "\n_tunes:\n";
        for (const auto& t : f.tunes) o << "    .word _tune_" << t.name << "\n";
        if (!f.bank.empty()) o << "\n.global _tune_sample_bank\n_tune_sample_bank:\n" << bytes(f.bank, "    .byte ", ", ", "") << "\n";
    } else {
        o << guardNote << "#include \"" << baseName(header) << "\"\n";
        for (const auto& t : f.tunes) {
            auto b = TuneCompiler::blob(f, t);
            o << "\nconst unsigned char tune_" << t.name << "[" << b.size() << "] = {\n" << bytes(b, "    ", ", ", ",") << ",\n};\n";
        }
        o << "\nconst unsigned char *const tunes[NTUNES] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) o << (k ? ", " : " ") << "tune_" << f.tunes[k].name;
        o << " };\n";
        if (!f.bank.empty())
            o << "\nconst unsigned char tune_sample_bank[TUNE_SAMPLES_SIZE] = {\n" << bytes(f.bank, "    ", ", ", ",") << ",\n};\n";
    }
    return o ? 0 : 1;
}

time_t mtime(const std::string& p);

struct PlayOptions {
    std::vector<std::string> tunes;
    unsigned loops = 1, frames = 0, seek = 0;
    uint32_t samplesAt = 0;      // trace: tune_samples' address
    uint8_t digiFirst = 0;       // trace: tune_digi_channels
    std::string wav, emulatorPath, sendCmd, prg;
    bool ntsc = false, mos6581 = false, mono = false, emulator = false, send = false, watch = false;
};

// The tunes named (all when none), or an error
bool chosen(const TuneFile& f, const PlayOptions& o, std::vector<const Tune*>& out) {
    if (o.tunes.empty()) { for (const auto& t : f.tunes) out.push_back(&t); return true; }
    for (const auto& n : o.tunes) {
        const Tune* found = nullptr;
        for (const auto& t : f.tunes) if (t.name == n) found = &t;
        if (!found) { std::cerr << "tune45: no tune " << n << "\n"; return false; }
        out.push_back(found);
    }
    return true;
}

// tune45 sample: the 8-bit samples (a pack45 stream unpacked, the file as it
// is, or a sound file made into them as pack45 --sound does) played at their
// rate, each held until the next as the audio DMA does, to 48 kHz
int playSample(const std::string& path, uint32_t rate, uint32_t maxRate, bool isUnsigned, const std::string& wav) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { std::cerr << "tune45: cannot read " << path << "\n"; return 1; }
    std::vector<uint8_t> f((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> samples;
    std::string what;
    // a pack45 stream, with its load address in front or without
    for (size_t skip : {(size_t)2, (size_t)0}) {
        if (f.size() < skip + pack45::kHeader + 3 || f[skip] != pack45::kFormat) continue;
        std::vector<uint8_t> s(f.begin() + (long)skip, f.end()), out;
        if (pack45::unpack(s, out, nullptr)) {
            samples = out;
            what = "packed, " + std::to_string(f.size()) + " bytes";
            break;
        }
    }
    if (samples.empty() && pack45::isSoundFile(path, f)) {
        pack45::Wav w;
        std::string err;
        if (!pack45::readSound(path, f, w, &err)) { std::cerr << "tune45: " << path << ": " << err << "\n"; return 1; }
        if (!rate) {
            uint32_t top = maxRate && maxRate < w.rate ? maxRate : w.rate;
            rate = pack45::rateFor(pack45::energyBelow(w, 0.999, top < w.rate ? 0.45 * top : 0), top);
        }
        samples = pack45::toBytes(pack45::resample(w, rate), isUnsigned, false);
        char line[96];
        snprintf(line, sizeof line, "%u Hz, %d channel%s", (unsigned)w.rate, w.channels, w.channels == 1 ? "" : "s");
        what = line;
    } else if (samples.empty()) {
        samples = f;
        what = "as they are";
    }
    if (!rate) { std::cerr << "tune45: " << path << ": at what rate? --rate <hz> (pack45 --sound printed it)\n"; return 1; }
    if (samples.empty()) { std::cerr << "tune45: " << path << ": no samples\n"; return 1; }
    const int out = 48000;
    AudioSink sink;
    if (!sink.open(wav, out)) { std::cerr << "tune45: " << sink.error() << "\n"; return 1; }
    char line[96];
    snprintf(line, sizeof line, "%zu samples at %u Hz, %.2f s", samples.size(), (unsigned)rate, (double)samples.size() / rate);
    std::cout << "playing " << path << " (" << what << "): " << line << " (via " << sink.via() << ")" << std::endl;
    std::vector<int16_t> buf;
    double step = (double)rate / out;
    for (double t = 0; t < (double)samples.size(); t += step) {
        uint8_t b = samples[(size_t)t];
        int v = isUnsigned ? b - 128 : (int8_t)b;
        buf.push_back((int16_t)(v * 256));
        buf.push_back((int16_t)(v * 256));
        if (buf.size() >= 8192) { sink.put(buf); buf.clear(); }
    }
    sink.put(buf);
    sink.close();
    return 0;
}

// tune45 import song.mod --audition: each sample the song uses (or one) at
// up to four of the notes it is played at: the original as the audio DMA
// plays it (8-bit, its loop, each sample held to the next), the SID's
// stand-in (the report's instrument, through the player's model and
// reSIDfp), then both at once, the original left and the SID right; each
// as loud as the other (the sound compared, not the level).

// A sample's two parts, mono, as long as each other (false: the
// instrument line did not parse, its errors printed)
bool auditionParts(const ModFile& m, const ModSampleReport& R, const PlayOptions& o, std::vector<double>& orig,
                   std::vector<double>& sid) {
    const int out = 48000;
    const ModSample& S = m.samples[(size_t)R.index - 1];
    // the notes: the lowest, the highest and between (at most four)
    std::vector<uint16_t> ps;
    size_t n = R.periods.size();
    for (size_t k = 0; k < std::min<size_t>(n, 4); k++) ps.push_back(R.periods[n - 1 - (n <= 4 ? k : k * (n - 1) / 3)]);
    // the original, and the SID's tune: a note a period, as long as the
    // sample sounds (a loop: 0.8 s), 40 ms between
    orig.clear();
    std::string text = modInstrumentLine(R, "a") + "\ntune t 1 once\nv1: @a";
    size_t loopEnd = S.loopStart + S.loopLen;
    for (uint16_t p : ps) {
        double rate = modRate(p);
        double d = S.looped() ? 0.8 : std::max(0.25, std::min(1.5, S.data.size() / rate));
        int ticks = (int)std::lround(d * 50);
        d = ticks / 50.0;
        double pos = 0, step = rate / out;
        for (int i = 0; i < (int)(d * out); i++, pos += step) {
            if (S.looped() && pos >= loopEnd) pos = S.loopStart + std::fmod(pos - loopEnd, (double)S.loopLen);
            orig.push_back(pos < S.data.size() ? S.data[(size_t)pos] / 128.0 * S.volume / 64.0 : 0);
        }
        orig.insert(orig.end(), (size_t)(0.04 * out), 0.0);
        text += " " + TuneCompiler::noteName(modSidNote(R, p)) + ":" + std::to_string(ticks) + " r:2";
    }
    TuneFile f;
    TuneCompiler tc;
    if (!tc.parse(text + "\n", "audition", f)) {
        for (const auto& e : tc.errors()) std::cerr << e << "\n";
        return false;
    }
    TuneAudio audio(o.ntsc, o.mos6581, out);
    TunePlayer player([&](uint8_t i, uint8_t v) { audio.write(i, v); });
    player.setNtsc(o.ntsc);
    player.setMirror(true);
    for (uint8_t c = 0; c < 4; c++) {
        audio.write((uint8_t)(c * 0x20 + 0x18), 15);
        player.setShadow((uint8_t)(c * 0x20 + 0x18), 15);
    }
    std::vector<int16_t> buf;
    for (int k = 0; k < 25; k++) audio.frame(buf);   // (the volume's click settles: left out)
    buf.clear();
    player.play(TuneCompiler::blob(f, f.tunes[0]));
    while (player.playing()) { player.update(); audio.frame(buf); }
    for (int k = 0; k < 15; k++) audio.frame(buf);   // (the release)
    sid.clear();
    double x1 = buf.empty() ? 0 : buf[0] / 32768.0, y1 = 0;
    for (size_t k = 0; k < buf.size(); k += 2) {     // the SID's offset taken out (a DC blocker)
        double x = buf[k] / 32768.0;
        y1 = x - x1 + 0.999 * y1;
        x1 = x;
        sid.push_back(y1);
    }
    // the same loudness: each to an RMS of 0.15, its peak at most 0.95
    auto level = [](std::vector<double>& v) {
        double e = 0, pk = 0;
        for (double x : v) { e += x * x; pk = std::max(pk, std::fabs(x)); }
        double rms = v.empty() ? 0 : std::sqrt(e / v.size());
        if (rms < 1e-6) return;
        double g = std::min(0.15 / rms, 0.95 / pk);
        for (double& x : v) x *= g;
    };
    size_t len = std::max(orig.size(), sid.size());
    orig.resize(len, 0.0);
    sid.resize(len, 0.0);
    level(orig);
    level(sid);
    return true;
}

// the parts as 16-bit stereo: 1 the original, 2 the SID, 3 both (the
// original left, the SID right); 0 the three, 0.4 s apart, and a second after
std::vector<int16_t> auditionAudio(const std::vector<double>& orig, const std::vector<double>& sid, int which) {
    const int out = 48000;
    std::vector<int16_t> o16;
    auto put = [&](double l, double r) {
        o16.push_back((int16_t)std::lround(std::max(-1.0, std::min(1.0, l)) * 32767));
        o16.push_back((int16_t)std::lround(std::max(-1.0, std::min(1.0, r)) * 32767));
    };
    auto pause = [&](int ms) { for (int k = 0; k < out * ms / 1000; k++) put(0, 0); };
    for (int part = 1; part <= 3; part++) {
        if (which && part != which) continue;
        for (size_t k = 0; k < orig.size(); k++)
            put(part == 2 ? sid[k] : orig[k], part == 1 ? orig[k] : sid[k]);
        pause(which ? 200 : part < 3 ? 400 : 1000);
    }
    return o16;
}

// played now, to the end (false: no player, the error printed)
bool playNow(const std::vector<int16_t>& s) {
    AudioSink k;
    if (!k.open("", 48000)) { std::cerr << "tune45: " << k.error() << "\n"; return false; }
    k.put(s);
    k.close();   // (waits for the player)
    return true;
}

// ask: after each sample, sid or sample (saved to choices as they are
// given; those already there are the defaults). Without a terminal (or
// with --ask) the answers are read from the input; with --wav the
// audition is written there first and nothing is played.
int audition(const ModFile& m, const std::vector<ModSampleReport>& r, int only, const PlayOptions& o, bool ask,
             const std::string& choicesPath) {
    std::vector<size_t> list;
    for (size_t k = 0; k < r.size(); k++)
        if ((!only || r[k].index == only) && (r[k].verdict == "sid" || r[k].verdict == "sid~" || r[k].verdict == "sample"))
            list.push_back(k);
    if (list.empty()) {
        std::cerr << "tune45: " << (only ? "sample " + std::to_string(only) + " is not one the song plays" : std::string("no sample the song plays")) << "\n";
        return 1;
    }
    auto line = [&](const ModSampleReport& R) {
        char b[256];
        snprintf(b, sizeof b, "%2d  %-22.22s %-6s %3d  SID: %s", R.index, m.samples[(size_t)R.index - 1].name.c_str(),
                 R.verdict.c_str(), R.score, modInstrumentLine(R, "a").substr(13).c_str());
        return std::string(b);
    };
    std::vector<std::vector<double>> origs(list.size()), sids(list.size());
    for (size_t i = 0; i < list.size(); i++)
        if (!auditionParts(m, r[list[i]], o, origs[i], sids[i])) return 1;
    // a file, or the speakers without asking
    if (!o.wav.empty() || !ask) {
        AudioSink sink;
        if (!sink.open(o.wav, 48000)) { std::cerr << "tune45: " << sink.error() << "\n"; return 1; }
        std::cout << "each sample: the original, the SID, then both (the original left, the SID right) (via " << sink.via() << ")\n";
        for (size_t i = 0; i < list.size(); i++) {
            std::cout << line(r[list[i]]) << std::endl;
            sink.put(auditionAudio(origs[i], sids[i], 0));
        }
        sink.close();
        if (!ask) return 0;
    }
    std::map<int, std::string> choices;
    std::string err;
    std::ifstream probe(choicesPath);
    if (probe && !readModChoices(choicesPath, choices, err)) { std::cerr << "tune45: " << err << "\n"; return 1; }
    bool play = o.wav.empty();
    std::cout << "For each sample: the original, the SID, then both (the original left, the SID right); then:\n"
                 "  Enter as shown  d the SID  s keep the sample  r again  1 original  2 SID  3 both  b back  q quit\n"
                 "(the choices go to " << choicesPath << " as they are made)\n";
    size_t i = 0;
    bool replay = true;
    while (i < list.size()) {
        const ModSampleReport& R = r[list[i]];
        auto it = choices.find(R.index);
        std::string def = it != choices.end() ? it->second : R.verdict == "sample" ? "sample" : "sid";
        if (replay) {
            std::cout << "\n" << line(R) << std::endl;
            if (play && !playNow(auditionAudio(origs[i], sids[i], 0))) return 1;
        }
        replay = false;
        std::cout << "  " << R.index << ": [" << def << "] d s r 1 2 3 b q > " << std::flush;
        std::string a;
        if (!std::getline(std::cin, a)) { std::cout << "\n"; break; }
        a.erase(0, a.find_first_not_of(" \t"));
        char c = a.empty() ? '\n' : (char)std::tolower((unsigned char)a[0]);
        if (c == 'q') break;
        if (c == 'r') { replay = true; continue; }
        if (c == '1' || c == '2' || c == '3') {
            if (play && !playNow(auditionAudio(origs[i], sids[i], c - '0'))) return 1;
            continue;
        }
        if (c == 'b') { if (i) i--; replay = true; continue; }
        if (c != '\n' && c != 'd' && c != 's') { std::cout << "  (d, s, r, 1, 2, 3, b or q)\n"; continue; }
        choices[R.index] = c == 'd' ? "sid" : c == 's' ? "sample" : def;
        if (!writeModChoices(choicesPath, m, choices, err)) { std::cerr << "tune45: " << err << "\n"; return 1; }
        i++;
        replay = true;
    }
    int ns = 0, nk = 0;
    for (const auto& [n, w] : choices) (w == "sid" ? ns : nk)++;
    std::cout << "\n" << choicesPath << ": " << ns << " SID instrument" << (ns == 1 ? "" : "s") << ", " << nk << " kept as sample"
              << (nk == 1 ? "" : "s") << " (the report with them: --report --choices " << choicesPath << ")\n";
    return 0;
}

// The audio DMA as the digi voices drive it, for play: a channel started
// (its control register on) reads the bank from its current address to its
// end at its rate, each sample held to the next, at its volume on its side
// (channels 0-1 left, 2-3 right) and the other side's; the samples are at
// address 0
struct DigiMix {
    const std::vector<uint8_t>* bank = nullptr;
    uint8_t regs[0x50] = {};
    struct Ch { bool on = false, loop = false; double pos = 0, step = 0; uint32_t end = 0, loopAt = 0; } ch[4];
    int rate = 48000;
    void write(uint8_t i, uint8_t v) {
        if (i >= 0x50) return;
        regs[i] = v;
        if (i < 0x10 || (i - 0x10) % 16) return;
        int c = (i - 0x10) / 16;
        const uint8_t* r = &regs[0x10 + c * 16];
        ch[c].on = (v & 0x80) != 0;
        if (!ch[c].on) return;
        uint32_t cur = r[0xA] | (r[0xB] << 8) | ((uint32_t)r[0xC] << 16);
        ch[c].pos = cur;
        ch[c].end = (cur & 0xFF0000) | r[7] | (r[8] << 8);
        ch[c].loop = (v & 0x40) != 0;   // (looping: from the end back to 1-3)
        ch[c].loopAt = r[1] | (r[2] << 8) | ((uint32_t)r[3] << 16);
        ch[c].step = (r[4] | (r[5] << 8) | ((uint32_t)r[6] << 16)) * (40500000.0 / 16777216.0) / rate;
    }
    // added to n stereo frames (L, R ...)
    void mix(int16_t* lr, size_t n) {
        for (size_t k = 0; k < n; k++)
            for (int c = 0; c < 4; c++) {
                Ch& h = ch[c];
                if (!h.on) continue;
                uint32_t at = (uint32_t)h.pos;
                if (at >= h.end && h.loop && h.loopAt < h.end) { h.pos = h.loopAt + (h.pos - h.end); at = (uint32_t)h.pos; }
                if (at >= h.end || !bank || at >= bank->size()) { h.on = false; continue; }
                int s = (*bank)[at] - 128;
                int own = s * regs[0x10 + c * 16 + 9] / 2, other = s * regs[0x0C + c] / 2;   // (half scale: room for the SIDs)
                int l = lr[2 * k] + (c < 2 ? own : other), rr = lr[2 * k + 1] + (c < 2 ? other : own);
                lr[2 * k] = (int16_t)std::max(-32768, std::min(32767, l));
                lr[2 * k + 1] = (int16_t)std::max(-32768, std::min(32767, rr));
                h.pos += h.step;
            }
    }
};

int play(const std::string& src, TuneFile f, const PlayOptions& o) {
    std::vector<const Tune*> list;
    if (!chosen(f, o, list)) return 1;
    time_t seen = mtime(src);
    TuneAudio audio(o.ntsc, o.mos6581);
    TunePlayer player([&](uint8_t i, uint8_t v) { audio.write(i, v); });
    DigiMix digi;
    digi.bank = &f.bank;
    digi.rate = audio.rate();
    player.setDigiWrite([&](uint8_t i, uint8_t v) { digi.write(i, v); });
    player.setNtsc(o.ntsc);
    player.setMirror(!o.mono);
    // the volume up (as sid_init: the player leaves it to the program)
    for (uint8_t c = 0; c < 4; c++) {
        audio.write((uint8_t)(c * 0x20 + 0x18), 15);
        player.setShadow((uint8_t)(c * 0x20 + 0x18), 15);
    }
    AudioSink sink;
    if (!sink.open(o.wav, audio.rate())) { std::cerr << "tune45: " << sink.error() << "\n"; return 1; }
    const double hz = o.ntsc ? 60.0 : 50.0;
    std::vector<int16_t> buf;
    for (size_t ti = 0; ti < list.size() || (o.watch && (ti = 0, !list.empty())); ti++) {
        const Tune* t = list[ti];
        unsigned ticks = t->ticks();
        double frames = t->frames(ticks) * (o.ntsc ? 60.0 / 50.0 : 1.0);
        unsigned total = t->loop ? (unsigned)(frames * o.loops + 0.5) : 0;
        char line[160];
        snprintf(line, sizeof line, "playing %s: %.1f s%s (via %s)\n", t->name.c_str(), frames / hz,
                 t->loop ? (o.loops > 1 ? ", looping" : ", once round") : "", sink.via().c_str());
        std::cout << line << std::flush;
        player.play(TuneCompiler::blob(f, *t));
        bool changed = false;
        for (unsigned n = 0; (t->loop ? n < total || o.watch : player.playing()); n++) {
            player.update();
            size_t was = buf.size();
            audio.frame(buf);
            digi.mix(buf.data() + was, (buf.size() - was) / 2);
            if (buf.size() >= 4096) { sink.put(buf); buf.clear(); }
            // --watch: the text changed: the tunes again, from the start
            if (o.watch && n % 25 == 0 && mtime(src) != seen) { changed = true; break; }
        }
        if (changed) {
            seen = mtime(src);
            std::string text;
            TuneCompiler c;
            TuneFile nf;
            if (readFile(src, text) && c.parse(text, src, nf)) {
                f = nf;
                list.clear();
                if (!chosen(f, o, list)) return 1;
                std::cout << "tune45: " << src << " changed\n";
            } else {
                for (const auto& e : c.errors()) std::cerr << e << "\n";
            }
            player.stop();
            buf.clear();
            ti = (size_t)-1;   /* (from the first tune again) */
            continue;
        }
        player.stop();
        for (unsigned n = 0; n < (unsigned)hz; n++) audio.frame(buf);   // a second for the release (the samples stopped)
        sink.put(buf);
        buf.clear();
    }
    sink.close();
    return 0;
}

// The model's registers after each frame (the tests compare them with the
// real player's): a line a frame, the SIDs' 128 bytes and the audio DMA's
// $50 (from $D710) in hex
int trace(const TuneFile& f, const PlayOptions& o) {
    std::vector<const Tune*> list;
    if (!chosen(f, o, list) || list.empty()) return 1;
    uint8_t regs[128 + 0x50] = {};
    TunePlayer player([&](uint8_t i, uint8_t v) { regs[i & 0x7F] = v; });
    player.setDigiWrite([&](uint8_t i, uint8_t v) { if (i < 0x50) regs[128 + i] = v; });
    player.setSamples(o.samplesAt);
    player.setDigiChannels(o.digiFirst);
    for (uint8_t c = 0; c < 4; c++) {   // (as sid_init leaves them: the volume up)
        regs[c * 0x20 + 0x18] = 15;
        player.setShadow((uint8_t)(c * 0x20 + 0x18), 15);
    }
    player.setNtsc(o.ntsc);
    player.setMirror(!o.mono);
    player.play(TuneCompiler::blob(f, *list[0]));
    player.seek(o.seek);
    for (unsigned n = 0; n < (o.frames ? o.frames : 100); n++) {
        player.update();
        for (uint8_t b : regs) { char h[4]; snprintf(h, sizeof h, "%02x", b); std::cout << h; }
        std::cout << "\n";
    }
    return 0;
}

// The directory tune45 is in (cc45, ln45 are next to it)
std::string toolDir() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n <= 0) return "";
    buf[n] = 0;
    std::string p = buf;
    return p.substr(0, p.find_last_of('/') + 1);
}

bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }

time_t mtime(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0 ? st.st_mtime : 0; }

std::string quote(const std::string& s) {
    std::string q = "'";
    for (char c : s) { if (c == '\'') q += "'\\''"; else q += c; }
    return q + "'";
}

// The player program (lib tuneplay.o45) with the tunes: prg, or "" with a message
std::string buildPlayer(const TuneFile& f, const std::string& src, const PlayOptions& o, const std::string& prg) {
    std::string bin = toolDir(), lib;
    for (const char* d : {"../lib/build/", "../lib/cc45/"})
        if (exists(bin + d + "tuneplay.o45")) { lib = bin + d; break; }
    if (lib.empty()) { std::cerr << "tune45: no tuneplay.o45 (lib/build or lib/cc45 next to " << bin << ")\n"; return ""; }
    char tmpl[] = "/tmp/tune45-XXXXXX";
    const char* tmp = mkdtemp(tmpl);
    if (!tmp) { std::cerr << "tune45: no temporary directory\n"; return ""; }
    std::string dir = tmp, c = dir + "/tunes.c", obj = dir + "/tunes.o45";
    {
        std::ofstream out(c);
        unsigned start = 0;
        if (!o.tunes.empty())
            for (size_t k = 0; k < f.tunes.size(); k++) if (f.tunes[k].name == o.tunes[0]) start = (unsigned)k;
        out << "/* made by tune45 play */\n";
        for (const auto& t : f.tunes) {
            auto b = TuneCompiler::blob(f, t);
            out << "static const unsigned char tune_" << t.name << "[" << b.size() << "] = {\n" << bytes(b, "    ", ", ", ",") << ",\n};\n";
        }
        out << "const unsigned char *const tunes[" << f.tunes.size() << "] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) out << (k ? ", " : " ") << "tune_" << f.tunes[k].name;
        out << " };\nconst char *const tune_names[" << f.tunes.size() << "] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) out << (k ? ", " : " ") << "\"" << f.tunes[k].name << "\"";
        out << " };\nconst unsigned char tune_count = " << f.tunes.size() << ";\nconst unsigned char tune_start = " << start
            << ";\nconst char tune_file[] = \"" << baseName(src) << "\";\n";
        out << "const unsigned int tune_sample_size = " << f.bank.size() << ";\nconst unsigned char tune_sample_bank["
            << std::max<size_t>(1, f.bank.size()) << "] = {\n" << (f.bank.empty() ? "    0" : bytes(f.bank, "    ", ", ", ",")) << "\n};\n";
    }
    std::string out = prg.empty() ? dir + "/tuneplay.prg" : prg;
    std::string cmd = quote(bin + "cc45") + " -O2 -c " + quote(c) + " -o " + quote(obj) + " && " +
                      quote(bin + "ln45") + " -basic -o " + quote(out) + " " + quote(lib + "tuneplay.o45") + " " + quote(obj) +
                      " " + quote(lib + "c45.lib") + " >/dev/null";
    LOG_INFO("tune45", "building the player program " << out);
    LOG_DEBUG("tune45", "command: " << cmd);
    int rc = system(cmd.c_str());
    remove(c.c_str());
    remove(obj.c_str());
    if (rc != 0) { std::cerr << "tune45: building the player program failed\n"; rmdir(dir.c_str()); return ""; }
    return out;
}

// --emulator / --send: the player program to xemu or a MEGA65; with --watch
// again whenever src changes
int playOnMega65(const std::string& src, TuneFile f, const PlayOptions& o) {
    std::string prg = buildPlayer(f, src, o, o.prg);
    if (prg.empty()) return 1;
    auto send = [&](const std::string& p) {
        std::string cmd = o.sendCmd;
        if (cmd.empty()) {
            if (!EmulatorLauncher::findOnPath("etherload").empty()) cmd = "etherload -r {prg}";
            else if (!EmulatorLauncher::findOnPath("m65").empty()) cmd = "m65 -F -r {prg}";
            else { std::cerr << "tune45: neither etherload nor m65 is on PATH (--send-cmd)\n"; return false; }
        }
        size_t at = cmd.find("{prg}");
        if (at != std::string::npos) cmd.replace(at, 5, quote(p));
        else cmd += " " + quote(p);
        std::cout << "Sending: " << cmd << "\n";
        return system(cmd.c_str()) == 0;
    };
    if (!o.watch) {
        bool ok = o.send ? send(prg) : EmulatorLauncher().launch(prg, o.emulatorPath);
        if (o.prg.empty()) { std::string d = prg.substr(0, prg.find_last_of('/')); remove(prg.c_str()); rmdir(d.c_str()); }
        return ok ? 0 : 1;
    }
    int pid = -1;
    time_t seen = mtime(src);
    for (;;) {
        if (!prg.empty()) {
            if (o.send) send(prg);
            else {
                if (pid > 0) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); }
                pid = EmulatorLauncher::launchAsync(prg, o.emulatorPath);
            }
        }
        while (mtime(src) == seen) std::this_thread::sleep_for(std::chrono::milliseconds(500));
        seen = mtime(src);
        std::string text;
        TuneCompiler c;
        if (!readFile(src, text) || !c.parse(text, src, f)) {
            for (const auto& e : c.errors()) std::cerr << e << "\n";
            prg.clear();
            continue;
        }
        prg = buildPlayer(f, src, o, o.prg.empty() ? prg : o.prg);
        std::cout << "tune45: " << src << " changed\n";
    }
}

// The text of a file: tune45 text, or a MIDI file imported
bool loadMusic(const std::string& path, const MidiImportOptions& mo, std::string& text) {
    std::string raw;
    if (!readFile(path, raw)) { std::cerr << "tune45: cannot read " << path << "\n"; return false; }
    if (raw.compare(0, 4, "MThd") != 0) { text = raw; return true; }
    std::string err;
    MidiImportOptions o = mo;
    if (o.name == "song") {
        std::string b = baseName(stem(path));
        for (auto& c : b) if (!std::isalnum((unsigned char)c)) c = '_';
        if (!b.empty() && !std::isdigit((unsigned char)b[0])) o.name = b;
    }
    text = midiImport(std::vector<unsigned char>(raw.begin(), raw.end()), o, err);
    if (text.empty()) { std::cerr << "tune45: " << path << ": " << err << "\n"; return false; }
    return true;
}

std::vector<int> numbers(const std::string& s) {
    std::vector<int> v;
    std::stringstream ss(s);
    std::string x;
    while (std::getline(ss, x, ',')) if (!x.empty()) v.push_back(atoi(x.c_str()));
    return v;
}

struct CreateCli {
    CreateOptions o;
    std::string tempo, length, key, append;
    bool play = false, check = false;
    int count = 20;
};

// tune45 create: the tune's text to -o / --append / stdout, and --play it
int create(CreateCli& c, const std::string& out, const PlayOptions& po) {
    if (!c.key.empty()) {
        int n = TuneCompiler::noteNumber(c.key + "4");
        if (n < 0) { std::cerr << "tune45: not a key: " << c.key << "\n"; return 1; }
        c.o.key = n % 12;
    }
    if (c.check) {
        // the genres, count seeds each: musicgen's melody checks, and the text must parse
        int fails = 0, n = 0;
        for (const auto& g : tuneGenres()) {
            if (!c.o.genre.empty() && g != c.o.genre) continue;
            for (int seed = 1; seed <= c.count; seed++) {
                CreateOptions o = c.o;
                o.genre = g;
                o.seed = (uint32_t)seed;
                CreateResult r = tuneCreate(o);
                std::string why = tuneCheck(r);
                TuneFile f;
                TuneCompiler tc;
                if (!tc.parse(r.text, g + " seed " + std::to_string(seed), f)) why += tc.errors()[0] + "; ";
                n++;
                if (!why.empty()) { fails++; std::cout << g << " seed " << seed << ": " << why << "\n"; }
            }
        }
        std::cout << n << " tunes composed, " << fails << " failed the checks\n";
        return fails ? 1 : 0;
    }
    // the tempo and length
    int styleBpm = 0;
    {
        CreateOptions probe = c.o;
        if (probe.genre.empty()) probe.genre = tuneGenres()[0];
        static const std::map<std::string, int> bpm = {{"pop", 112}, {"rock", 128}, {"blues", 92}, {"jazz", 132},
                                                       {"classical", 100}, {"ambient", 70}, {"chiptune", 150}};
        auto it = bpm.find(probe.genre);
        styleBpm = it != bpm.end() ? it->second : 112;
    }
    if (!c.o.seed) c.o.seed = (uint32_t)time(nullptr) & 0xFFFF;
    if (c.o.genre.empty()) {   // (the genre drawn from the seed, so --seed gives the tune again)
        c.o.genre = tuneGenres()[c.o.seed % tuneGenres().size()];
        static const std::map<std::string, int> bpm = {{"pop", 112}, {"rock", 128}, {"blues", 92}, {"jazz", 132},
                                                       {"classical", 100}, {"ambient", 70}, {"chiptune", 150}};
        styleBpm = bpm.at(c.o.genre);
    }
    if (!c.tempo.empty()) {
        if (c.tempo == "slow") c.o.bpm = styleBpm * 4 / 5;
        else if (c.tempo == "medium") c.o.bpm = styleBpm;
        else if (c.tempo == "fast") c.o.bpm = styleBpm * 5 / 4;
        else if ((c.o.bpm = atoi(c.tempo.c_str())) < 30 || c.o.bpm > 300) {
            std::cerr << "tune45: a tempo is 30-300 beats a minute, or slow / medium / fast\n";
            return 1;
        }
    }
    if (!c.length.empty()) {
        int v = atoi(c.length.c_str());
        if (v <= 0) { std::cerr << "tune45: not a length: " << c.length << "\n"; return 1; }
        if (c.length.back() == 's') {
            int b = c.o.bpm ? c.o.bpm : styleBpm;
            c.o.bars = std::max(1, (int)std::lround(v * b / 60.0 / 4.0));
        } else {
            c.o.bars = v;
        }
    }
    CreateResult r = tuneCreate(c.o);
    if (!r.error.empty()) { std::cerr << "tune45: " << r.error << "\n"; return 1; }
    std::string text = r.text;
    if (!c.append.empty()) {
        // the instruments it does not have yet, then the tune
        std::string old;
        readFile(c.append, old);
        TuneFile of;
        TuneCompiler tc;
        tc.parse(old, c.append, of);
        std::string add;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("instrument ", 0) == 0) {
                std::istringstream w(line);
                std::string kw, name;
                w >> kw >> name;
                bool have = false;
                for (const auto& i : of.instruments) if (i.name == name) have = true;
                if (have) continue;
            }
            add += line + "\n";
        }
        std::ofstream o(c.append, std::ios::app);
        o << (old.empty() || old.back() == '\n' ? "" : "\n") << "\n" << add;
        if (!o) { std::cerr << "tune45: cannot write " << c.append << "\n"; return 1; }
        std::cout << "tune45: " << r.genre << " added to " << c.append << "\n";
    } else if (!out.empty()) {
        std::ofstream o(out);
        o << text;
        if (!o) { std::cerr << "tune45: cannot write " << out << "\n"; return 1; }
    } else if (!c.play) {
        std::cout << text;
    }
    if (c.play) {
        TuneFile f;
        TuneCompiler tc;
        if (!tc.parse(text, "(created)", f)) { for (const auto& e : tc.errors()) std::cerr << e << "\n"; return 1; }
        std::string src = !out.empty() ? out : !c.append.empty() ? c.append : "";
        if (po.emulator || po.send) return playOnMega65(src.empty() ? "created.txt" : src, f, po);
        return play(src, f, po);
    }
    return 0;
}

int list(const TuneFile& f) {
    size_t total = 0;
    for (const auto& t : f.tunes) {
        auto b = TuneCompiler::blob(f, t);
        total += b.size();
        unsigned ticks = t.ticks();
        double sec = t.frames(ticks) / 50.0;   // PAL frames; NTSC plays as long
        char buf[160];
        std::string form = (b[0] & 0x20) ? "  coded" : "";
        if (!TuneCompiler::dataMode) {   // (plain: what coding would make of it)
            auto c = tunepack::pack(b);
            if (!c.empty() && c.size() < b.size()) form = "  (coded " + std::to_string(c.size()) + ")";
        }
        snprintf(buf, sizeof buf, "%-12s %2zu voices %5zu bytes  %5u ticks  %6.1f s  %.2f frames/tick  %s%s\n",
                 t.name.c_str(), t.voices.size() + t.digi.size(), b.size(), ticks, sec, t.fpt / 256.0, t.loop ? "loop" : "once",
                 form.c_str());
        std::cout << buf;
        std::cout << "             voices:";
        for (size_t v = 0; v < t.voices.size(); v++) std::cout << " v" << v + 1 << " " << t.voices[v].stream.size() + 1;
        for (size_t v = 0; v < t.digi.size(); v++) std::cout << " d" << v + 1 << " " << t.digi[v].stream.size() + 1;
        std::cout << " bytes\n             instruments:";
        for (int i : TuneCompiler::usedInstruments(f, t)) std::cout << " " << f.instruments[(size_t)i].name;
        std::cout << "\n";
        if (!t.digi.empty()) {
            std::cout << "             hits:";
            for (int h : TuneCompiler::usedHits(t)) {
                char hb[96];
                snprintf(hb, sizeof hb, " %s (%u bytes, %u Hz)", TuneCompiler::hitName(f, h).c_str(),
                         (unsigned)f.hits[(size_t)h].len, (unsigned)f.hits[(size_t)h].rate);
                std::cout << hb;
            }
            std::cout << "\n";
        }
    }
    std::cout << f.tunes.size() << " tunes, " << total << " bytes";
    if (!f.bank.empty()) std::cout << "; samples " << f.bank.size() << " bytes";
    std::cout << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Logger::setTool("tune45");
    argc = Logger::takeFlags(argc, argv);   // -v ... -vvvv anywhere
    if (argc < 2) { usage(std::cerr); return 1; }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { usage(std::cout); return 0; }
    if (cmd == "-V" || cmd == "--version") { std::cout << "tune45 " << SUITE_VERSION << "\n"; return 0; }
    std::string out, mode = "c", header, input;
    long loadAt = -1;              // --bin: a load address first (--load-at)
    PlayOptions po;
    CreateCli cc;
    MidiImportOptions mo;
    bool loopFlag = false;   // (import: --loop; a tune made of a MIDI file ends by default)
    uint32_t sampleRate = 0, sampleMax = 0;   // sample: --rate, --max-rate
    std::string samplesOut;                  // convert --bin: --samples <file>
    bool sampleUnsigned = false;
    bool modReportFlag = false, modAll = false;   // import of a MOD: --report, --extract <dir>, --all, --audition [n]
    int modAudition = -1;
    SidImportOptions sio;   // import of a .sid: --song, --seconds
    bool modAsk = false;
    std::string modChoices;
    std::string modExtract;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) out = argv[++i];
        else if (a == "--tune" && i + 1 < argc) po.tunes.push_back(argv[++i]);
        else if (a == "--loops" && i + 1 < argc) po.loops = (unsigned)std::max(1, atoi(argv[++i]));
        else if (a == "--frames" && i + 1 < argc) po.frames = (unsigned)std::max(1, atoi(argv[++i]));
        else if (a == "--seek" && i + 1 < argc) po.seek = (unsigned)std::max(0, atoi(argv[++i]));
        else if (a == "--wav" && i + 1 < argc) po.wav = argv[++i];
        else if (a == "--ntsc") po.ntsc = true;
        else if (a == "--6581") po.mos6581 = true;
        else if (a == "--rate" && i + 1 < argc) sampleRate = (uint32_t)std::max(0L, strtol(argv[++i], nullptr, 0));
        else if (a == "--max-rate" && i + 1 < argc) sampleMax = (uint32_t)std::max(0L, strtol(argv[++i], nullptr, 0));
        else if (a == "--unsigned") sampleUnsigned = true;
        else if (a == "--samples" && i + 1 < argc) samplesOut = argv[++i];
        else if (a == "--samples-at" && i + 1 < argc) po.samplesAt = (uint32_t)strtoul(argv[++i], nullptr, 0);
        else if (a == "--digi-first" && i + 1 < argc) po.digiFirst = (uint8_t)std::min(3, std::max(0, atoi(argv[++i])));
        else if (a == "--mono") po.mono = true;
        else if (a == "--emulator") po.emulator = true;
        else if (a == "--emulator-path" && i + 1 < argc) { po.emulator = true; po.emulatorPath = argv[++i]; }
        else if (a == "--send") po.send = true;
        else if (a == "--send-cmd" && i + 1 < argc) { po.send = true; po.sendCmd = argv[++i]; }
        else if (a == "--watch") po.watch = true;
        else if (a == "--prg" && i + 1 < argc) po.prg = argv[++i];
        else if (a == "--genre" && i + 1 < argc) cc.o.genre = argv[++i];
        else if (a == "--tempo" && i + 1 < argc) cc.tempo = argv[++i];
        else if (a == "--length" && i + 1 < argc) cc.length = argv[++i];
        else if (a == "--voices" && i + 1 < argc) cc.o.voices = atoi(argv[++i]);
        else if (a == "--seed" && i + 1 < argc) cc.o.seed = (uint32_t)strtoul(argv[++i], nullptr, 0);
        else if (a == "--key" && i + 1 < argc) cc.key = argv[++i];
        else if (a == "--once") cc.o.loop = false;
        else if (a == "--loop") cc.o.loop = loopFlag = true;
        else if (a == "--name" && i + 1 < argc) cc.o.name = argv[++i];
        else if (a == "--append" && i + 1 < argc) cc.append = argv[++i];
        else if (a == "--play") cc.play = true;
        else if (a == "--check") cc.check = true;
        else if (a == "--count" && i + 1 < argc) cc.count = atoi(argv[++i]);
        else if (a == "--tracks" && i + 1 < argc) mo.sources = numbers(argv[++i]);
        else if (a == "--channels") {
            mo.channels = true;
            if (i + 1 < argc && std::isdigit((unsigned char)argv[i + 1][0])) mo.sources = numbers(argv[++i]);
        }
        else if (a == "--drums") mo.drums = true;
        else if (a == "--ticks-per-beat" && i + 1 < argc) mo.ticksPerBeat = std::max(1, atoi(argv[++i]));
        else if (a == "--transpose" && i + 1 < argc) mo.transpose = atoi(argv[++i]);
        else if (a == "--instruments" && i + 1 < argc) {
            if (!readFile(argv[++i], mo.instruments)) { std::cerr << "tune45: cannot read " << argv[i] << "\n"; return 1; }
        }
        else if (a == "--report") modReportFlag = true;
        else if (a == "--extract" && i + 1 < argc) modExtract = argv[++i];
        else if (a == "--all") modAll = true;
        else if (a == "--ask") modAsk = true;
        else if (a == "--song" && i + 1 < argc) sio.song = std::max(1, atoi(argv[++i]));
        else if (a == "--seconds" && i + 1 < argc) sio.seconds = std::max(0.1, atof(argv[++i]));
        else if (a == "--no-patterns") sio.patterns = false;
        else if (a == "--search" && i + 1 < argc) sio.search = std::max(1.0, atof(argv[++i]));
        else if (a == "--no-loop") sio.findLoop = false;
        else if (a == "--choices" && i + 1 < argc) modChoices = argv[++i];
        else if (a == "--audition") {
            modAudition = 0;
            if (i + 1 < argc && std::isdigit((unsigned char)argv[i + 1][0])) modAudition = atoi(argv[++i]);
        }
        else if (a == "--coded") TuneCompiler::dataMode = 1;
        else if (a == "--coded-all") TuneCompiler::dataMode = 2;
        else if (a == "--bin") mode = "bin";
        else if (a == "--asm") mode = "asm";
        else if (a == "--header" && i + 1 < argc) header = argv[++i];
        else if (a == "--load-at" && i + 1 < argc) loadAt = strtol(argv[++i], nullptr, 0) & 0xFFFF;
        else if (a == "-h" || a == "--help") { usage(std::cout); return 0; }
        else if (!a.empty() && a[0] == '-') { std::cerr << "tune45: unknown option " << a << "\n"; return 1; }
        else input = a;
    }
    if (cmd == "create") return create(cc, out, po);
    if (cmd == "sample") {
        if (input.empty()) { std::cerr << "tune45: no sample file\n"; return 1; }
        return playSample(input, sampleRate, sampleMax, sampleUnsigned, po.wav);
    }
    if (cmd == "import") {
        if (input.empty()) { std::cerr << "tune45: no MIDI, MOD, XM, S3M, IT or SID file\n"; return 1; }
        std::string raw;
        if (!readFile(input, raw)) { std::cerr << "tune45: cannot read " << input << "\n"; return 1; }
        std::vector<uint8_t> bytes(raw.begin(), raw.end());
        if (raw.compare(0, 4, "PSID") == 0 || raw.compare(0, 4, "RSID") == 0) {
            // a C64 tune: its player run, the registers it writes as text
            PsidFile pf;
            std::string err;
            if (!readPsid(bytes, pf, err)) { std::cerr << "tune45: " << input << ": " << err << "\n"; return 1; }
            sio.name = cc.o.name;
            sio.source = baseName(input);
            std::string text = sidImport(pf, sio, err);
            if (text.empty()) { std::cerr << "tune45: " << input << ": " << err << "\n"; return 1; }
            if (out.empty()) std::cout << text;
            else {
                std::ofstream of(out);
                of << text;
                if (!of) { std::cerr << "tune45: cannot write " << out << "\n"; return 1; }
            }
            TuneFile chk;
            TuneCompiler tc;
            if (!tc.parse(text, out.empty() ? "./-" : out, chk)) {
                std::cerr << "tune45: the text is written, but convert will say:\n";
                for (const auto& e : tc.errors()) std::cerr << "  " << e << "\n";
                return 1;
            }
            return 0;
        }
        bool tracker = isTrackerModule(bytes);
        if (raw.compare(0, 4, "MThd") != 0 && (tracker || isMod(input, bytes))) {
            // a MOD, or an XM, S3M or IT read as one
            ModFile m;
            std::string err;
            if (!(tracker ? readTracker(bytes, m, err) : readMod(bytes, m, err))) {
                std::cerr << "tune45: " << input << ": " << err << "\n";
                return 1;
            }
            auto r = analyseMod(m);
            // the choices: given, or (asking) beside the MOD
            std::string choicesPath = modChoices;
            if (choicesPath.empty()) {
                choicesPath = input;
                size_t dot = choicesPath.find_last_of('.'), slash = choicesPath.find_last_of('/');
                if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) choicesPath.erase(dot);
                choicesPath += ".choices";
            }
            bool ask = modAudition >= 0 && (modAsk || (isatty(0) && po.wav.empty()));
            if (!modChoices.empty() && !ask) {
                std::map<int, std::string> c;
                if (!readModChoices(modChoices, c, err)) { std::cerr << "tune45: " << err << "\n"; return 1; }
                applyModChoices(r, c);
            }
            if (!modReportFlag && modExtract.empty() && modAudition < 0) {
                // the song as text; the kept samples' WAVs beside it, in <name>_samples
                ModImportOptions io;
                io.name = cc.o.name;
                io.source = baseName(input);
                io.choices = modChoices;
                // (to the output: beside the MOD, named as from here)
                std::string base = out.empty() ? stem(input) : stem(out);
                io.samplesDir = base + "_samples";
                io.samplesRef = out.empty() ? io.samplesDir : baseName(base) + "_samples";
                std::vector<std::string> written;
                std::map<int, double> kept;
                io.keptBytes = &kept;
                std::string text = modImport(m, r, io, written, err);
                if (text.empty()) { std::cerr << "tune45: " << input << ": " << err << "\n"; return 1; }
                // the best guess (no choices): kept samples over the 64 KB bank
                // made SID instruments, the biggest first, the drums last
                auto bankBytes = [&]() { double t = 0; for (const auto& [k, v] : kept) t += v; return t; };
                std::vector<std::string> fitted;
                while (modChoices.empty() && bankBytes() > 0x10000) {
                    int pick = -1;
                    for (int pass = 0; pass < 2 && pick < 0; pass++)
                        for (const auto& [k, v] : kept)
                            if ((pass == 1 || r[(size_t)k].kind == "tone") && (pick < 0 || v > kept[pick])) pick = k;
                    if (pick < 0) break;
                    char line[160];
                    snprintf(line, sizeof line, "%d %s (%.0f KB)", pick + 1, m.samples[(size_t)pick].name.c_str(), kept[pick] / 1024);
                    fitted.push_back(line);
                    r[(size_t)pick].verdict = "sid~";
                    r[(size_t)pick].fitted = true;
                    kept.clear();
                    std::vector<std::string> before = written;
                    written.clear();
                    text = modImport(m, r, io, written, err);
                    if (text.empty()) { std::cerr << "tune45: " << input << ": " << err << "\n"; return 1; }
                    for (const auto& f : before)   // (the files of the samples made the SID's)
                        if (std::find(written.begin(), written.end(), f) == written.end()) std::remove(f.c_str());
                }
                if (!fitted.empty()) {
                    std::cerr << "tune45: the kept samples did not fit in 64 KB: made SID instruments of";
                    for (size_t k = 0; k < fitted.size(); k++) std::cerr << (k ? ", " : " ") << fitted[k];
                    std::cerr << " (--audition to choose)\n";
                }
                if (out.empty()) std::cout << text;
                else {
                    std::ofstream of(out);
                    of << text;
                    if (!of) { std::cerr << "tune45: cannot write " << out << "\n"; return 1; }
                }
                if (!written.empty())
                    std::cerr << "tune45: " << written.size() << " sample file" << (written.size() == 1 ? "" : "s") << " in " << io.samplesDir << "\n";
                // the text as convert reads it (the samples' bank included)
                TuneFile chk;
                TuneCompiler tc;
                if (!tc.parse(text, out.empty() ? "./-" : out, chk)) {
                    std::cerr << "tune45: the text is written, but convert will say:\n";
                    for (const auto& e : tc.errors()) std::cerr << "  " << e << "\n";
                    if (!kept.empty()) {
                        std::cerr << "the kept samples, about:";
                        for (const auto& [k, v] : kept) std::cerr << " " << k + 1 << " " << (int)(v / 1024 + 0.5) << " KB";
                        std::cerr << " (make some SID instruments with --audition, or shorten them: len= on a sample line)\n";
                    }
                    return 1;
                }
                // (a hit cut short by the next: as the MOD has it)
                int cutShort = 0;
                for (const auto& w : tc.warnings())
                    if (w.find("is cut short by the next hit") != std::string::npos) cutShort++;
                    else std::cerr << w << "\n";
                if (cutShort)
                    std::cerr << "tune45: samples cut short by the next hit on their channel, as in the MOD (tune45 list says where)\n";
                return 0;
            }
            if (modReportFlag) std::cout << modReport(m, r);
            if (!modExtract.empty()) {
                std::vector<std::string> written;
                if (!extractModSamples(m, r, modExtract, modAll, written, err)) { std::cerr << "tune45: " << err << "\n"; return 1; }
                for (const auto& w : written) std::cout << "wrote " << w << "\n";
            }
            if (modAudition >= 0) return audition(m, r, modAudition, po, ask, choicesPath);
            return 0;
        }
        if (!cc.o.name.empty()) mo.name = cc.o.name;
        mo.loop = loopFlag;
        mo.voices = std::max(1, std::min(12, cc.o.voices));
        std::string text;
        if (!loadMusic(input, mo, text)) return 1;
        TuneFile chk;
        TuneCompiler tc;
        if (!tc.parse(text, input, chk)) { for (const auto& e : tc.errors()) std::cerr << e << "\n"; return 1; }
        if (out.empty()) { std::cout << text; return 0; }
        std::ofstream o(out);
        o << text;
        return o ? 0 : 1;
    }
    if (cmd != "convert" && cmd != "list" && cmd != "play" && cmd != "trace") { std::cerr << "tune45: unknown command " << cmd << "\n"; usage(std::cerr); return 1; }
    if (input.empty()) { std::cerr << "tune45: no input file\n"; return 1; }
    std::string text;
    if (!loadMusic(input, mo, text)) return 1;
    TuneFile f;
    TuneCompiler c;
    if (!c.parse(text, input, f)) {
        for (const auto& e : c.errors()) std::cerr << e << "\n";
        return 1;
    }
    for (const auto& w : c.warnings()) std::cerr << w << "\n";
    LOG_INFO("tune45", input << ": " << f.tunes.size() << " tune(s), " << f.instruments.size() << " instrument(s)");
    for (const auto& t : f.tunes)
        LOG_DEBUG("tune45", "tune \"" << t.name << "\" (line " << t.line << "): " << t.voices.size() << " voice(s), "
                            << (t.loop ? "loops" : "ends") << ", " << TuneCompiler::blob(f, t).size() << " bytes of data");
    if (cmd == "list") return list(f);
    if (cmd == "play") return po.emulator || po.send ? playOnMega65(input, f, po) : play(input, f, po);
    if (cmd == "trace") return trace(f, po);
    if (out.empty()) out = stem(input) + (mode == "bin" ? ".bin" : mode == "asm" ? ".s45" : ".c");
    LOG_INFO("tune45", "writing " << out << " (" << (mode == "c" ? "C" : mode) << ")");
    return convert(f, input, out, mode, header, loadAt, samplesOut);
}
