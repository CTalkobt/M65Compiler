// TuneMod.cpp — tune45 import of a MOD: the file, and its samples weighed
// for the SID (TuneMod.hpp)
#include "TuneMod.hpp"
#include "TuneCompiler.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sys/stat.h>

namespace {

const double kPaula = 3546894.6;    // the Amiga's (PAL) clock / 2: a period's rate is this / period

int be16(const std::vector<uint8_t>& f, size_t at) { return at + 1 < f.size() ? f[at] << 8 | f[at + 1] : 0; }

// the tag's channels (0: not a tag)
int tagChannels(const std::string& t) {
    if (t == "M.K." || t == "M!K!" || t == "M&K!" || t == "N.T." || t == "FLT4" || t == "4CHN") return 4;
    if (t == "FLT8" || t == "OKTA" || t == "CD81") return 8;
    if (t.size() == 4 && std::isdigit((unsigned char)t[0]) && t.substr(1) == "CHN") return t[0] - '0';
    if (t.size() == 4 && std::isdigit((unsigned char)t[0]) && std::isdigit((unsigned char)t[1]) && t.substr(2) == "CH")
        return (t[0] - '0') * 10 + (t[1] - '0');
    if (t.size() == 4 && t.substr(0, 3) == "TDZ" && std::isdigit((unsigned char)t[3])) return t[3] - '0';
    return 0;
}

double clamp01(double v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

std::string ptNote(uint16_t period) {
    static const char* n[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    int i = (int)std::lround(12 * std::log2(856.0 / period));
    int o = 1 + (int)std::floor(i / 12.0);
    i = ((i % 12) + 12) % 12;
    return std::string(n[i]) + std::to_string(o);
}

// the MIDI note a sample of fundamental Ts samples sounds at, played at period p
double soundingNote(double ts, double period) { return 69 + 12 * std::log2(kPaula / (period * ts) / 440.0); }

// --- the song's use of the samples --------------------------------------

struct Hit { double t; int ch, sample; uint16_t period; double end; };

struct Use {
    std::vector<ModSampleReport> r;   // the use counted
    std::vector<Hit> hits;            // each note, in time (end: the next on its channel, or the sample's end)
    int speed = 6, bpm = 125;         // the first
};

Use walk(const ModFile& m) {
    Use u;
    u.r.resize(m.samples.size());
    for (size_t k = 0; k < u.r.size(); k++) u.r[k].index = (int)k + 1;
    std::vector<int> cur((size_t)m.channels, 0);
    std::vector<int> open((size_t)m.channels, -1);   // the channel's sounding hit
    std::vector<std::set<uint16_t>> periods(m.samples.size());
    std::vector<std::vector<uint16_t>> all(m.samples.size());
    int speed = m.speed, bpm = m.bpm;
    u.speed = speed;
    u.bpm = bpm;
    bool first = true;
    double t = 0;
    int startRow = 0;
    for (int pos = 0; pos < m.songLen && pos < (int)m.order.size(); pos++) {
        size_t pat = m.order[(size_t)pos];
        if (pat >= m.patterns.size()) continue;
        if (startRow >= m.rowsOf(pat)) startRow = 0;
        const auto& P = m.patterns[pat];
        int row = startRow, nrows = m.rowsOf(pat);
        startRow = 0;
        for (; row < nrows; row++) {
            bool brk = false;
            for (int ch = 0; ch < m.channels; ch++) {
                const ModCell& c = P[(size_t)(row * m.channels + ch)];
                if (c.sample && c.sample <= m.samples.size()) cur[(size_t)ch] = c.sample;
                int s = cur[(size_t)ch] - 1;
                bool note = c.period && s >= 0 && c.effect != 3 && c.effect != 5;   // (3, 5: a slide to it)
                if (note) {
                    auto& R = u.r[(size_t)s];
                    R.notes++;
                    R.channelsMask |= 1 << std::min(ch, 30);
                    // (the period it plays at, the sample's tuning in)
                    uint16_t per = (uint16_t)std::max(1L, std::lround(c.period * std::pow(2.0, -m.samples[(size_t)s].tuneSemis() / 12)));
                    periods[(size_t)s].insert(per);
                    all[(size_t)s].push_back(per);
                    if (open[(size_t)ch] >= 0) u.hits[(size_t)open[(size_t)ch]].end = std::min(u.hits[(size_t)open[(size_t)ch]].end, t);
                    const ModSample& S = m.samples[(size_t)s];
                    double len = S.looped() ? 1e9 : S.data.size() / (kPaula / per);
                    open[(size_t)ch] = (int)u.hits.size();
                    u.hits.push_back({t, ch, s, per, t + len});
                }
                if (s >= 0) {
                    auto& R = u.r[(size_t)s];
                    switch (c.effect) {
                    case 0x0: if (c.param) R.arpeggio++; break;
                    case 0x1: case 0x2: case 0x3: R.porta++; break;
                    case 0x5: R.porta++; R.volume++; break;
                    case 0x4: R.vibrato++; break;
                    case 0x6: R.vibrato++; R.volume++; break;
                    case 0x7: case 0xA: case 0xC: R.volume++; break;
                    case 0x9: R.offset++; break;
                    case 0xE: if ((c.param >> 4) == 0xA || (c.param >> 4) == 0xB) R.volume++; break;
                    default: break;
                    }
                }
                if (c.effect == 0xF && c.param) {
                    if (c.param < 32) speed = c.param; else bpm = c.param;
                    if (first && t == 0) { u.speed = speed; u.bpm = bpm; }
                }
                if (c.effect == 0xD) { brk = true; startRow = (c.param >> 4) * 10 + (c.param & 15); }
                if (c.effect == 0xB) brk = true;   // (a jump: the song is taken in order, once)
            }
            t += speed * 2.5 / bpm;
            if (brk) break;
        }
        first = false;
    }
    for (auto& h : u.hits) h.end = std::min(h.end, t);
    for (size_t k = 0; k < u.r.size(); k++) {
        u.r[k].periods.assign(periods[k].begin(), periods[k].end());
        if (!all[k].empty()) {
            auto v = all[k];
            std::nth_element(v.begin(), v.begin() + (long)v.size() / 2, v.end());
            u.r[k].medianPeriod = v[v.size() / 2];
        }
    }
    return u;
}

// --- the sound ----------------------------------------------------------

struct Signal {
    std::vector<double> x;
    size_t loopStart = 0, loopLen = 0;
    bool cyclic = false;            // the region is a short loop: read round it
    double at(double t, size_t start, size_t len) const {   // linear, in [start, start + len)
        if (cyclic) {
            t = std::fmod(t - start, (double)len);
            if (t < 0) t += len;
            size_t i = (size_t)t;
            double f = t - i;
            return x[start + i] * (1 - f) + x[start + (i + 1) % len] * f;
        }
        if (t < 0) t = 0;
        size_t i = (size_t)t;
        if (i + 1 >= x.size()) return x.back();
        double f = t - i;
        return x[i] * (1 - f) + x[i + 1] * f;
    }
};

// the pitch: the normalised square difference function's first key maximum
// (McLeod): lag in samples, and its height
bool pitch(const Signal& s, size_t start, size_t len, double& lag, double& clarity, double lo = 2, double hi = 0) {
    size_t maxLag = s.cyclic ? len : std::min(len / 2, (size_t)2048);
    if (hi > 0) maxLag = std::min(maxLag, (size_t)std::ceil(hi));
    if (len < 16 || maxLag < 3) return false;
    std::vector<double> y(s.x.begin() + (long)start, s.x.begin() + (long)(start + len));
    double mean = 0;
    for (double v : y) mean += v;
    mean /= len;
    for (double& v : y) v -= mean;   // (an offset is not a period)
    std::vector<double> n(maxLag + 1, 0.0);
    for (size_t tau = 1; tau <= maxLag; tau++) {
        double acf = 0, m = 0;
        if (s.cyclic) {
            for (size_t i = 0; i < len; i++) {
                double a = y[i], b = y[(i + tau) % len];
                acf += a * b;
                m += a * a + b * b;
            }
        } else {
            for (size_t i = 0; i + tau < len; i++) {
                double a = y[i], b = y[i + tau];
                acf += a * b;
                m += a * a + b * b;
            }
        }
        n[tau] = m > 1e-12 ? 2 * acf / m : 0;
    }
    // key maxima: the highest of each positive lobe after the first fall below 0
    std::vector<std::pair<size_t, double>> keys;
    size_t tau = 1;
    while (tau <= maxLag && n[tau] > 0) tau++;
    while (tau <= maxLag) {
        while (tau <= maxLag && n[tau] <= 0) tau++;
        size_t best = 0;
        double bv = -1;
        while (tau <= maxLag && n[tau] > 0) {
            if (n[tau] > bv) { bv = n[tau]; best = tau; }
            tau++;
        }
        if (best && best >= lo) keys.push_back({best, bv});
    }
    if (s.cyclic && (keys.empty() || keys.back().first != len)) keys.push_back({len, 1.0});
    if (keys.empty()) return false;
    double top = 0;
    for (const auto& k : keys) top = std::max(top, k.second);
    for (const auto& k : keys)
        if (k.second >= 0.9 * top) {
            double l = (double)k.first;
            size_t i = k.first;
            if (i > 1 && i < maxLag) {   // a parabola through the peak and its neighbours
                double a = n[i - 1], b = n[i], c = n[i + 1], d = a - 2 * b + c;
                if (std::fabs(d) > 1e-12) l += 0.5 * (a - c) / d;
            }
            lag = l;
            clarity = k.second;
            return true;
        }
    return false;
}

const int kBins = 256;

// one period, the mean of the periods in [start, start + len) (at most 16),
// and the share of the energy that does not repeat
std::vector<double> fold(const Signal& s, size_t start, size_t len, double ts, double& aperiodic) {
    std::vector<double> avg(kBins, 0.0);
    int periods = s.cyclic ? std::max(1, (int)std::lround(len / ts)) : std::min(16, (int)((len - 1) / ts));
    aperiodic = 1;
    if (periods < 1) return avg;
    for (int m = 0; m < periods; m++)
        for (int j = 0; j < kBins; j++) avg[(size_t)j] += s.at(start + (m + j / (double)kBins) * ts, start, len);
    double mean = 0;
    for (auto& v : avg) { v /= periods; mean += v; }
    mean /= kBins;
    double total = 0, rest = 0;
    for (int m = 0; m < periods; m++)
        for (int j = 0; j < kBins; j++) {
            double v = s.at(start + (m + j / (double)kBins) * ts, start, len) - mean;
            total += v * v;
            double d = v - (avg[(size_t)j] - mean);
            rest += d * d;
        }
    for (auto& v : avg) v -= mean;
    aperiodic = total > 1e-12 ? clamp01(rest / total) : 1;
    return avg;
}

// harmonics 1..k of one period
std::vector<double> harmonics(const std::vector<double>& p, int k) {
    std::vector<double> a((size_t)k, 0.0);
    for (int h = 1; h <= k; h++) {
        std::complex<double> c = 0;
        for (int j = 0; j < kBins; j++) c += p[(size_t)j] * std::polar(1.0, -2 * M_PI * h * j / kBins);
        a[(size_t)h - 1] = std::abs(c);
    }
    return a;
}

double cosine(const std::vector<double>& a, const std::vector<double>& b) {
    double ab = 0, aa = 0, bb = 0;
    for (size_t k = 0; k < a.size() && k < b.size(); k++) { ab += a[k] * b[k]; aa += a[k] * a[k]; bb += b[k] * b[k]; }
    return aa > 0 && bb > 0 ? ab / std::sqrt(aa * bb) : 0;
}

struct Template { std::string wave; int pw; double lp; std::vector<double> a; };

std::vector<Template> templates(int k) {
    std::vector<Template> t;
    auto add = [&](const std::string& w, int pw, std::vector<double> a) {
        t.push_back({w, pw, 0, a});
        for (double m : {1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0}) {   // the SID's low-pass: 12 dB an octave
            std::vector<double> f = a;
            for (int h = 1; h <= k; h++) f[(size_t)h - 1] /= std::sqrt(1 + std::pow(h / m, 4));
            t.push_back({w, pw, m, f});
        }
    };
    std::vector<double> tri((size_t)k), saw((size_t)k);
    for (int h = 1; h <= k; h++) {
        tri[(size_t)h - 1] = h % 2 ? 1.0 / (h * h) : 0;
        saw[(size_t)h - 1] = 1.0 / h;
    }
    t.push_back({"tri", 0x800, 0, tri});
    add("saw", 0x800, saw);
    for (int w = 1; w <= 10; w++) {   // 5% .. 50%
        std::vector<double> p((size_t)k);
        for (int h = 1; h <= k; h++) p[(size_t)h - 1] = std::fabs(std::sin(M_PI * h * w / 20.0)) / h;
        add("pulse", (int)std::lround(4096 * w / 20.0), p);
    }
    return t;
}

int nearest(double ms, const double* table) {
    int best = 0;
    for (int k = 1; k < 16; k++)
        if (std::fabs(std::log(ms / table[k])) < std::fabs(std::log(ms / table[best]))) best = k;
    return best;
}

bool hasWord(const std::string& name, const std::vector<std::string>& words) {
    std::string l;
    for (char c : name) l += (char)std::tolower((unsigned char)c);
    std::vector<std::string> tokens;
    std::string w;
    for (char c : l + " ") {
        if (std::isalnum((unsigned char)c)) w += c;
        else { if (!w.empty()) tokens.push_back(w); w.clear(); }
    }
    for (const auto& x : words) {
        if (x.size() <= 3) { if (std::find(tokens.begin(), tokens.end(), x) != tokens.end()) return true; }
        else if (l.find(x) != std::string::npos) return true;
    }
    return false;
}

std::string fmt(const char* f, double a) { char b[96]; snprintf(b, sizeof b, f, a); return b; }

void analyse(const ModSample& S, ModSampleReport& R) {
    if (S.data.size() < 4) { R.verdict = "empty"; return; }
    if (!R.notes) { R.verdict = "unused"; return; }
    const double rate = kPaula / R.medianPeriod;
    Signal s;
    size_t end = S.looped() ? S.loopStart + S.loopLen : S.data.size();
    for (size_t i = 0; i < end; i++) s.x.push_back(S.data[i] / 128.0);
    // the loudness, 20 ms a frame
    size_t frame = std::max<size_t>(8, (size_t)std::lround(rate * 0.02));
    std::vector<double> rms;
    for (size_t i = 0; i < end; i += frame) {
        double e = 0;
        size_t n = std::min(frame, end - i);
        for (size_t k = 0; k < n; k++) e += s.x[i + k] * s.x[i + k];
        rms.push_back(std::sqrt(e / n));
    }
    size_t pk = (size_t)(std::max_element(rms.begin(), rms.end()) - rms.begin());
    double peak = rms[pk];
    double fms = frame * 1000.0 / rate;
    if (peak < 1e-4) { R.verdict = "empty"; return; }
    size_t att = 0;
    while (att < rms.size() && rms[att] < 0.9 * peak) att++;
    R.attackMs = att * fms + 2;
    if (S.looped()) {
        double e = 0;
        for (size_t i = S.loopStart; i < end; i++) e += s.x[i] * s.x[i];
        R.sustain = clamp01(std::sqrt(e / S.loopLen) / peak);
    }
    size_t d = pk;
    double floorLevel = std::max(R.sustain * 1.1, 0.05) * peak;
    while (d < rms.size() && rms[d] > floorLevel) d++;
    R.decayMs = (d - pk) * fms;
    double low = peak;
    for (size_t k = pk + 1; k < rms.size(); k++) {
        low = std::min(low, rms[k]);
        if (rms[k] > 1.6 * low && rms[k] > 0.3 * peak) { R.swells++; low = rms[k]; }
    }
    static const double attackMs[16] = {2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000};
    static const double decayMs[16] = {6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000};
    int an = nearest(R.attackMs, attackMs);
    int dn = R.decayMs < 3 ? 0 : nearest(R.decayMs, decayMs);
    int sn = (int)std::lround(R.sustain * 15);
    if (sn >= 15) dn = 0;
    int rn = S.looped() ? 3 : dn;
    R.ad = an << 4 | dn;
    R.sr = sn << 4 | rn;

    // the pitch: of a short loop round it; else after the attack (or in a long loop)
    size_t start, len;
    if (S.looped() && S.loopLen <= 512) { s.cyclic = true; start = S.loopStart; len = S.loopLen; }
    else if (S.looped()) {   // a long loop: its end, and before it when it is short
        len = std::min<size_t>(end, 4096);
        start = end - len;
    }
    else {
        start = std::min(end, (pk + 1) * frame);
        if (start + 512 > end) start = end > 4096 ? end - 4096 : 0;
        len = std::min<size_t>(end - start, 4096);
    }
    double lag = 0, clarity = 0;
    bool got = pitch(s, start, len, lag, clarity);
    R.clarity = got ? clarity : 0;
    R.periodSamples = got ? lag : 0;
    std::vector<double> a;
    int k = 0;
    if (got && lag >= 2.5) {
        auto p = fold(s, start, len, lag, R.aperiodic);
        k = std::max(1, std::min(32, (int)(lag / 2.2)));
        a = harmonics(p, k);
    }
    R.tonal = got && lag >= 2.5 && clarity >= 0.75 && R.aperiodic <= 0.55;
    // brightness (an unpitched sound's kind)
    {
        size_t n = std::min<size_t>(1024, end);
        size_t at = std::min(pk * frame, end - n);
        double num = 0, den = 0;
        for (size_t b = 1; b < n / 2; b++) {
            std::complex<double> c = 0;
            for (size_t i = 0; i < n; i++)
                c += s.x[at + i] * (0.5 - 0.5 * std::cos(2 * M_PI * i / (n - 1))) * std::polar(1.0, -2 * M_PI * (double)b * i / n);
            double e = std::norm(c);
            num += e * b;
            den += e;
        }
        R.centroidHz = den > 0 ? num / den * rate / n : 0;
    }
    static const std::vector<std::string> drums = {"kick", "bd", "bassdrum", "snare", "sd", "snr", "hat", "hihat", "hh", "cymbal",
                                                   "crash", "ride", "tom", "clap", "perc", "drum", "conga", "bongo", "rim", "cowbell"};
    static const std::vector<std::string> voices = {"vox", "voice", "choir", "aah", "ooh", "vocal", "speech", "sing", "laugh", "scream"};
    static const std::vector<std::string> rich = {"string", "orch", "bell", "piano", "guitar", "brass", "sax", "trumpet"};
    static const std::vector<std::string> plain = {"bass", "lead", "square", "sqr", "saw", "sine", "flute", "organ", "chip", "synth"};
    bool drumName = hasWord(S.name, drums);
    if (!R.tonal || (drumName && R.clarity < 0.9)) {
        bool held = S.looped() && R.sustain > 0.5 && !drumName;
        R.kind = held ? "texture" : hasWord(S.name, {"kick", "bd", "bassdrum"}) ? "kick" : hasWord(S.name, {"snare", "sd", "snr", "clap"}) ? "snare"
               : hasWord(S.name, {"hat", "hihat", "hh", "cymbal", "crash", "ride"}) ? "hat"
               : R.centroidHz < 250 ? "kick" : R.centroidHz < 2500 ? "snare" : "hat";
        R.score = 65 + (S.data.size() / rate > 0.5 ? 10 : 0);
        R.reasons.push_back(fmt("no steady pitch (clarity %.2f", R.clarity) + fmt(", %.0f%% not repeating)", R.aperiodic * 100) +
                            (held ? ": a held noise, pad or texture (looped)" : ": a drum or a noise, " + R.kind + "-like"));
        R.reasons.push_back(held ? "the SID's noise waveform is far from it"
                            : R.kind == "hat" ? "the SID could stand in: noise, a fast decay"
                            : R.kind == "kick" ? "the SID could stand in: a pulse falling (slide=, or a glide)"
                            : "the SID could stand in, roughly: noise and a pulse (a wave table)");
        R.verdict = "sample";
        return;
    }
    R.kind = "tone";
    // the SID's nearest waveform
    double best = -1, plainBest = -1;
    Template bt, pt;
    for (const auto& t : templates(k)) {
        double c = cosine(a, t.a);
        if (c > best) { best = c; bt = t; }
        if (t.lp == 0 && c > plainBest) { plainBest = c; pt = t; }
    }
    if (plainBest >= best - 0.01) bt = pt, best = plainBest;
    R.fit = best;
    R.wave = bt.wave;
    R.pulseWidth = bt.pw;
    R.lpTimes = bt.lp;
    // the harmonics early (after the attack) against late (the loop, or
    // further on), and the pitch over the sample
    {
        Signal o = s;
        o.cyclic = false;
        size_t e = std::min(end, (pk + 1) * frame);
        size_t need = (size_t)(4 * lag) + 2;
        size_t late = S.looped() ? S.loopStart : e + (end - e) * 6 / 10;
        size_t elen = std::min<size_t>(4096, late > e ? late - e : 0);
        size_t llen = std::min<size_t>(4096, end - late);
        if (elen >= need && llen >= need) {
            double ae, al;
            auto pe = fold(o, e, elen, lag, ae), pl = fold(o, late, llen, lag, al);
            if (ae < 0.6 && al < 0.6) R.stability = cosine(harmonics(pe, k), harmonics(pl, k));
        }
    }
    if (!s.cyclic) {
        std::vector<double> cents;
        Signal o = s;
        size_t w = std::max<size_t>(1024, (size_t)(4 * lag));
        for (size_t at = (pk + 1) * frame; at + w <= end; at += w / 2) {
            double l, c;
            if (pitch(o, at, w, l, c, lag * 0.8, lag * 1.25) && c > 0.7) cents.push_back(1200 * std::log2(l / lag));
        }
        if (cents.size() > 2) {
            double mu = 0, v = 0;
            for (double c : cents) mu += c;
            mu /= cents.size();
            for (double c : cents) v += (c - mu) * (c - mu);
            R.pitchCents = std::sqrt(v / cents.size());
        }
    }
    // the score
    double sc = 0;
    double pf = clamp01((0.97 - R.fit) / 0.17) * 45;
    sc += pf;
    char b[160];
    if (bt.wave == "pulse") snprintf(b, sizeof b, "harmonics nearest a pulse of %d%%", (int)std::lround(bt.pw * 100.0 / 4096));
    else snprintf(b, sizeof b, "harmonics nearest a %s", bt.wave == "tri" ? "triangle" : "saw");
    std::string near = b;
    if (bt.lp > 0) near += fmt(" through the low-pass at %gx the note", bt.lp);
    near += fmt(" (fit %.2f)", R.fit);
    R.reasons.push_back(pf > 10 ? near + ": a timbre the SID's waveforms are far from" : near);
    double pn = clamp01((R.aperiodic - 0.05) / 0.3) * 30;
    sc += pn;
    if (pn > 5) R.reasons.push_back(fmt("%.0f%% of it does not repeat (breath, noise, a chord or a voice)", R.aperiodic * 100));
    double ps = clamp01((0.95 - R.stability) / 0.3) * 15;
    sc += ps;
    if (ps > 4) R.reasons.push_back(fmt("its timbre changes as it sounds (%.2f early to late)", R.stability) +
                                    (S.looped() ? "" : ": a filter sweep (fsweep=) may follow it"));
    double pp = clamp01((R.pitchCents - 15) / 60) * 10;
    sc += pp;
    if (pp > 2) R.reasons.push_back(fmt("its pitch wanders %.0f cents (vibrato or a slide in the sample)", R.pitchCents));
    if (R.swells) {
        sc += std::min(R.swells, 3) * 5;
        R.reasons.push_back(std::to_string(R.swells) + " swell" + (R.swells > 1 ? "s" : "") +
                            " in its loudness (several hits, tremolo or an echo in the sample)");
    }
    if (R.volume > R.notes / 2) {
        sc += 5;
        R.reasons.push_back("the song changes its volume often (" + std::to_string(R.volume) + " rows): a SID voice has no volume of its own");
    }
    if (hasWord(S.name, voices)) { sc += 20; R.reasons.push_back("its name says a voice"); }
    else if (hasWord(S.name, rich)) sc += 5;
    else if (hasWord(S.name, plain)) sc -= 5;
    R.score = (int)std::lround(std::max(0.0, std::min(100.0, sc)));
    R.verdict = R.score <= 25 ? "sid" : R.score <= 55 ? "sid~" : "sample";
}

std::string clean(const std::string& n) {
    std::string o;
    for (char c : n) o += std::isprint((unsigned char)c) ? c : ' ';
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
}

}  // namespace

bool isMod(const std::string& path, const std::vector<uint8_t>& f) {
    if (f.size() >= 1084 && tagChannels(std::string(f.begin() + 1080, f.begin() + 1084))) return true;
    std::string l;
    for (char c : path) l += (char)std::tolower((unsigned char)c);
    return f.size() >= 600 && l.size() > 4 && l.substr(l.size() - 4) == ".mod";
}

bool readMod(const std::vector<uint8_t>& f, ModFile& m, std::string& error) {
    if (f.size() < 600) { error = "too short for a MOD"; return false; }
    std::string tag = f.size() >= 1084 ? std::string(f.begin() + 1080, f.begin() + 1084) : "";
    int ch = tagChannels(tag);
    int ns = ch ? 31 : 15;
    m = ModFile();
    m.tag = ch ? tag : "(15 samples)";
    m.channels = ch ? ch : 4;
    m.title = clean(std::string(f.begin(), f.begin() + 20).c_str());
    size_t at = 20;
    for (int k = 0; k < ns; k++, at += 30) {
        ModSample s;
        s.name = clean(std::string(f.begin() + (long)at, f.begin() + (long)at + 22).c_str());
        s.data.resize((size_t)be16(f, at + 22) * 2);
        int ft = f[at + 24] & 15;
        s.finetune = ft > 7 ? ft - 16 : ft;
        s.volume = std::min<int>(64, f[at + 25]);
        s.loopStart = (uint32_t)be16(f, at + 26) * (ch ? 2 : 1);   // (an old file's: bytes)
        s.loopLen = (uint32_t)be16(f, at + 28) * 2;
        m.samples.push_back(s);
    }
    m.songLen = std::min<int>(128, f[at]);
    m.order.assign(f.begin() + (long)at + 2, f.begin() + (long)at + 130);
    at += 130 + (ch ? 4 : 0);
    int np = 0;
    for (uint8_t o : m.order) np = std::max(np, o + 1);
    size_t psize = 64 * 4 * (size_t)m.channels;
    if (at + np * psize > f.size()) { error = "the patterns go past the file's end (not a MOD?)"; return false; }
    for (int p = 0; p < np; p++, at += psize) {
        std::vector<ModCell> cells(64 * (size_t)m.channels);
        for (size_t c = 0; c < cells.size(); c++) {
            const uint8_t* b = &f[at + c * 4];
            cells[c].sample = (uint8_t)((b[0] & 0xF0) | (b[2] >> 4));
            cells[c].period = (uint16_t)((b[0] & 0x0F) << 8 | b[1]);
            cells[c].effect = b[2] & 0x0F;
            cells[c].param = b[3];
        }
        m.patterns.push_back(cells);
        m.rows.push_back(64);
    }
    for (auto& s : m.samples) {
        size_t n = std::min(s.data.size(), f.size() > at ? f.size() - at : 0);
        for (size_t i = 0; i < n; i++) s.data[i] = (int8_t)f[at + i];
        s.data.resize(n);   // (a file cut short: what there is)
        at += n;
    }
    return true;
}

std::vector<ModSampleReport> analyseMod(const ModFile& m) {
    Use u = walk(m);
    for (size_t k = 0; k < m.samples.size(); k++) analyse(m.samples[k], u.r[k]);
    return u.r;
}

double modRate(double period) { return kPaula / period; }

int modSidNote(const ModSampleReport& r, double period) {
    if (r.kind == "tone" && r.periodSamples > 0)
        return std::max(0, std::min(127, (int)std::lround(soundingNote(r.periodSamples, period))));
    // a drum: the tracker's note on the SID's (C-2 as c4), a hat higher
    int n = 60 + (int)std::lround(12 * std::log2(428.0 / period));
    if (r.kind == "hat") n += 36;
    else if (r.kind == "kick") n -= 12;
    return std::max(0, std::min(127, n));
}

std::string modInstrumentLine(const ModSampleReport& r, const std::string& name) {
    char b[200];
    if (r.kind == "tone") {
        snprintf(b, sizeof b, "instrument %s %s %02x %02x %03x 0 0", name.c_str(), r.wave.c_str(), r.ad, r.sr, r.pulseWidth);
        std::string l = b;
        if (r.lpTimes > 0 && r.periodSamples > 0) {
            // the cutoff at lpTimes the fundamental of the note played most
            // (the 8580's: about 5.9 Hz a step)
            double hz = r.lpTimes * modRate(r.medianPeriod) / r.periodSamples;
            int cut = std::max(1, std::min(0x7FF, (int)std::lround(hz / 5.86)));
            snprintf(b, sizeof b, " filter=lp res=0 cutoff=%x", cut);
            l += b;
        }
        return l;
    }
    // the drums' stand-ins (as the report's reasons say)
    int dn = r.ad & 15;
    if (r.kind == "kick") snprintf(b, sizeof b, "instrument %s pulse 00 %x0 0800 0 0 slide=-40", name.c_str(), std::max(dn, 6));
    else if (r.kind == "texture") snprintf(b, sizeof b, "instrument %s noise %02x %02x 0800 0 0", name.c_str(), r.ad, r.sr);
    else snprintf(b, sizeof b, "instrument %s noise 00 %x0 0800 0 0", name.c_str(), std::max(dn, r.kind == "hat" ? 3 : 6));
    return b;
}

bool readModChoices(const std::string& path, std::map<int, std::string>& c, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot read " + path; return false; }
    std::string line;
    int ln = 0;
    while (std::getline(in, line)) {
        ln++;
        line = line.substr(0, line.find(';'));
        int n;
        char w[16];
        if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
        if (sscanf(line.c_str(), "%d %15s", &n, w) != 2 || n < 1 || n > 31 || (std::string(w) != "sid" && std::string(w) != "sample")) {
            error = path + ":" + std::to_string(ln) + ": expected <sample> sid|sample, not " + line;
            return false;
        }
        c[n] = w;
    }
    return true;
}

bool writeModChoices(const std::string& path, const ModFile& m, const std::map<int, std::string>& c, std::string& error) {
    std::ofstream out(path);
    out << "; tune45 import's choices for " << (m.title.empty() ? "(untitled)" : m.title)
        << ": a sample and sid (a SID instrument) or sample (kept as a sample)\n";
    for (const auto& [n, w] : c) {
        std::string name = n <= (int)m.samples.size() ? m.samples[(size_t)n - 1].name : "";
        out << n << " " << w << (name.empty() ? "" : "   ; " + name) << "\n";
    }
    if (!out) { error = "cannot write " + path; return false; }
    return true;
}

void applyModChoices(std::vector<ModSampleReport>& r, const std::map<int, std::string>& c) {
    for (auto& R : r) {
        auto it = c.find(R.index);
        if (it == c.end() || R.verdict == "unused" || R.verdict == "empty") continue;
        bool sid = it->second == "sid";
        if (sid == (R.verdict != "sample")) { R.chosen = true; continue; }   // (as measured)
        R.reasons.push_back("your choice: " + it->second + " (measured: " + R.verdict + ", " + std::to_string(R.score) + ")");
        R.verdict = sid ? "sid" : "sample";
        R.chosen = true;
    }
}

std::string modReport(const ModFile& m, const std::vector<ModSampleReport>& r) {
    Use u = walk(m);
    std::string o;
    char b[512];
    snprintf(b, sizeof b, "%s: %s, %d channels, %d positions, %zu patterns, speed %d at %d BPM\n\n",
             m.title.empty() ? "(untitled)" : m.title.c_str(), m.tag.c_str(), m.channels, m.songLen, m.patterns.size(), u.speed, u.bpm);
    o += b;
    o += " #  name                    bytes  loop          notes  played       verdict score  the SID's nearest\n";
    int nsid = 0, napprox = 0, nsample = 0;
    size_t sampleBytes = 0, pitchedBytes = 0;
    std::vector<std::string> pitched;
    for (const auto& R : r) {
        const ModSample& S = m.samples[(size_t)R.index - 1];
        if (R.verdict == "empty") continue;
        std::string loop = S.looped() ? std::to_string(S.loopStart) + "+" + std::to_string(S.loopLen) : "-";
        std::string played = "-";
        if (!R.periods.empty()) {
            std::string lo = ptNote(R.periods.back()), hi = ptNote(R.periods.front());   // (a higher note: a smaller period)
            played = lo == hi ? lo : lo + ".." + hi;
        }
        std::string nearS;
        if ((R.verdict == "sid" || R.verdict == "sid~") && R.kind != "tone") {
            nearS = modInstrumentLine(R, "x").substr(13) + " (a stand-in)";
        } else if (R.verdict == "sid" || R.verdict == "sid~") {
            snprintf(b, sizeof b, "pulse %03x", R.pulseWidth);
            nearS = R.wave == "pulse" ? std::string(b) : R.wave;
            if (R.lpTimes > 0) nearS += fmt(" lp %gx", R.lpTimes);
            snprintf(b, sizeof b, ", ad %02x sr %02x", R.ad, R.sr);
            nearS += b;
            double hiN = soundingNote(R.periodSamples, R.periods.front()), loN = soundingNote(R.periodSamples, R.periods.back());
            int l = (int)std::lround(loN), h = (int)std::lround(hiN);
            if (l >= 0 && h <= 127) nearS += ", sounds " + TuneCompiler::noteName(l) + (l == h ? "" : ".." + TuneCompiler::noteName(h));
        } else if (R.verdict == "sample") nearS = R.kind == "tone" ? "(keep the sample)" : "(keep the sample: " + R.kind + ")";
        snprintf(b, sizeof b, "%2d  %-22.22s %6zu  %-12s %6d  %-12s %-7s %5s  %s\n", R.index, S.name.c_str(), S.data.size(),
                 loop.c_str(), R.notes, played.c_str(), (R.verdict + (R.chosen ? "*" : "")).c_str(),
                 R.verdict == "unused" ? "" : std::to_string(R.score).c_str(), nearS.c_str());
        o += b;
        if (R.verdict == "sid") nsid++;
        if (R.verdict == "sid~") napprox++;
        if (R.verdict == "sample") {
            nsample++;
            sampleBytes += S.data.size();
            if (R.kind == "tone" && R.periods.size() > 1) {
                pitched.push_back(std::to_string(R.index) + " (" + std::to_string(R.periods.size()) + " notes)");
                pitchedBytes += S.data.size() * R.periods.size();
            }
        }
    }
    for (const auto& R : r)
        if (R.chosen) { o += "(* your choice: a choices file)\n"; break; }
    o += "\nWhy:\n";
    for (const auto& R : r) {
        if (R.reasons.empty()) continue;
        snprintf(b, sizeof b, "%2d  %s\n", R.index, R.verdict.c_str());
        o += b;
        // what was measured (at the period it is played at most)
        double hz = kPaula / R.medianPeriod;
        if (R.periodSamples > 0)
            snprintf(b, sizeof b, "      measured: pitch clarity %.2f, a period of %.1f samples (%.0f Hz at %s), %.0f%% not repeating,\n"
                     "        timbre early/late %.2f, pitch spread %.0f cents, attack %.0f ms, decay %.0f ms, sustain %.2f\n",
                     R.clarity, R.periodSamples, hz / R.periodSamples, ptNote((uint16_t)R.medianPeriod).c_str(), R.aperiodic * 100,
                     R.stability, R.pitchCents, R.attackMs, R.decayMs, R.sustain);
        else
            snprintf(b, sizeof b, "      measured: no pitch found; brightness %.0f Hz at %s, attack %.0f ms, decay %.0f ms, sustain %.2f\n",
                     R.centroidHz, ptNote((uint16_t)R.medianPeriod).c_str(), R.attackMs, R.decayMs, R.sustain);
        o += b;
        for (const auto& w : R.reasons) o += "      " + w + "\n";
    }
    // the samples kept: how many sound at once (the digi voices they need)
    std::vector<std::pair<double, int>> ev;
    for (const auto& h : u.hits)
        if (r[(size_t)h.sample].verdict == "sample" && h.end > h.t) { ev.push_back({h.t, 1}); ev.push_back({h.end, -1}); }
    std::sort(ev.begin(), ev.end());
    int now = 0, most = 0;
    for (const auto& e : ev) { now += e.second; most = std::max(most, now); }
    o += "\n";
    snprintf(b, sizeof b, "SID instruments: %d (%d of them approximately); samples kept: %d, %zu bytes as in the MOD\n",
             nsid + napprox, napprox, nsample, sampleBytes);
    o += b;
    if (nsample) {
        snprintf(b, sizeof b, "The kept samples sound at most %d at once: %s\n", most,
                 most <= 4 ? "digi voices enough (the audio DMA has 4 channels)" : "more than the audio DMA's 4 channels");
        o += b;
    }
    if (!pitched.empty()) {
        o += "Kept samples played at several notes:";
        for (const auto& p : pitched) o += " " + p;
        snprintf(b, sizeof b, "\n  (a digi voice plays a hit at one rate: a hit a note, %zu bytes, until the player has pitched hits)\n",
                 pitchedBytes);
        o += b;
    }
    return o;
}

namespace {
void le(std::vector<uint8_t>& b, uint32_t v, int n) { for (int k = 0; k < n; k++) b.push_back((uint8_t)(v >> (8 * k))); }
}

namespace {

// an 8-bit WAV of a sample's bytes, at rate, its loop (if any) and the note
// it sounds at in a smpl chunk
bool writeWav8(const std::string& path, const std::vector<int8_t>& data, uint32_t rate, bool looped, uint32_t loopStart,
               uint32_t loopLen, int unity, std::string& error) {
    std::vector<uint8_t> pcm;
    for (int8_t v : data) pcm.push_back((uint8_t)(v + 128));
    std::vector<uint8_t> smpl;
    le(smpl, 0, 4); le(smpl, 0, 4);
    le(smpl, (uint32_t)std::lround(1e9 / rate), 4);
    le(smpl, (uint32_t)unity, 4);
    le(smpl, 0, 4); le(smpl, 0, 4); le(smpl, 0, 4);
    le(smpl, looped ? 1 : 0, 4);
    le(smpl, 0, 4);
    if (looped) {
        le(smpl, 0, 4); le(smpl, 0, 4);
        le(smpl, loopStart, 4);
        le(smpl, loopStart + loopLen - 1, 4);
        le(smpl, 0, 4); le(smpl, 0, 4);
    }
    std::vector<uint8_t> w = {'R', 'I', 'F', 'F'};
    uint32_t size = 4 + (8 + 16) + (8 + (uint32_t)pcm.size() + (pcm.size() & 1)) + (8 + (uint32_t)smpl.size());
    le(w, size, 4);
    for (char c : std::string("WAVEfmt ")) w.push_back((uint8_t)c);
    le(w, 16, 4); le(w, 1, 2); le(w, 1, 2); le(w, rate, 4); le(w, rate, 4); le(w, 1, 2); le(w, 8, 2);
    for (char c : std::string("data")) w.push_back((uint8_t)c);
    le(w, (uint32_t)pcm.size(), 4);
    w.insert(w.end(), pcm.begin(), pcm.end());
    if (pcm.size() & 1) w.push_back(0);
    for (char c : std::string("smpl")) w.push_back((uint8_t)c);
    le(w, (uint32_t)smpl.size(), 4);
    w.insert(w.end(), smpl.begin(), smpl.end());
    std::ofstream out(path, std::ios::binary);
    out.write((const char*)w.data(), (std::streamsize)w.size());
    if (!out) { error = "cannot write " + path; return false; }
    return true;
}

// NN_name: a sample's file name, without the extension
std::string fileStem(const ModSampleReport& R, const ModSample& S) {
    std::string name;
    for (char c : S.name) name += std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '_';
    while (name.find("__") != std::string::npos) name.erase(name.find("__"), 1);
    while (!name.empty() && name.back() == '_') name.pop_back();
    while (!name.empty() && name[0] == '_') name.erase(0, 1);
    char fn[64];
    snprintf(fn, sizeof fn, "%02d_%s", R.index, name.empty() ? "sample" : name.substr(0, 40).c_str());
    return fn;
}

int unityNote(const ModSampleReport& R, double period) {
    if (R.tonal && R.periodSamples > 0) return std::max(0, std::min(127, (int)std::lround(soundingNote(R.periodSamples, period))));
    return 60;
}

}  // namespace

bool extractModSamples(const ModFile& m, const std::vector<ModSampleReport>& r, const std::string& dir, bool all,
                       std::vector<std::string>& written, std::string& error) {
    mkdir(dir.c_str(), 0777);
    for (const auto& R : r) {
        if (R.verdict == "empty" || R.verdict == "unused") continue;
        if (!all && R.verdict != "sample") continue;
        const ModSample& S = m.samples[(size_t)R.index - 1];
        std::string path = dir + "/" + fileStem(R, S) + ".wav";
        if (!writeWav8(path, S.data, (uint32_t)std::lround(kPaula / R.medianPeriod), S.looped(), S.loopStart, S.loopLen,
                       unityNote(R, R.medianPeriod), error))
            return false;
        written.push_back(path);
    }
    return true;
}

namespace {

std::string fmtNum(double v) {
    char b[32];
    snprintf(b, sizeof b, "%.2f", v);
    std::string s = b;
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
    return s;
}

struct SidRow {
    int kind = 2;                   // 0 a note, 1 held, 2 a rest
    int note = 0, sample = -1, glide = 0, gk = 3, vib = 0;
    std::string tokens;
};

struct DigiRow {
    std::string hit;                // "s08:c4": sample 8 at that note, or ""
    bool stop = false;              // the channel stopped (a note cut, the channel's SID note)
    int vol = -1;                   // the volume from here (0-64; -1 as it was)
};

// a tracker period as the note tune45 names (C-2, period 428, as c4)
int periodNote(double period) { return std::max(0, std::min(127, (int)std::lround(60 + 12 * std::log2(428.0 / period)))); }

}  // namespace

std::string modImport(const ModFile& m, const std::vector<ModSampleReport>& r, const ModImportOptions& o,
                      std::vector<std::string>& written, std::string& error) {
    // the rows in the order the song plays them
    struct Row { size_t pat; int row, pos, speed, bpm, delay; bool patStart; };
    std::vector<Row> rows;
    {
        std::set<std::pair<int, int>> seen;
        int pos = 0, row = 0, speed = m.speed, bpm = m.bpm;
        while (pos < m.songLen && pos < (int)m.order.size() && rows.size() < 100000) {
            if (!seen.insert({pos, row}).second) break;   // (played before: the song loops here)
            size_t pat = m.order[(size_t)pos];
            if (pat >= m.patterns.size()) { pos++; row = 0; continue; }
            int nextPos = pos + 1, nextRow = 0, loopRow = 0, loopCount = 0, nrows = m.rowsOf(pat);
            bool first = true;
            if (row >= nrows) row = 0;
            for (int rr = row; rr < nrows; rr++) {
                int delay = 0, brk = -1, jump = -1, loopTo = -1;
                for (int ch = 0; ch < m.channels; ch++) {
                    const ModCell& c = m.patterns[pat][(size_t)(rr * m.channels + ch)];
                    if (c.effect == 0xF && c.param) { if (c.param < 32) speed = c.param; else bpm = c.param; }
                    if (c.effect == 0xD) brk = (c.param >> 4) * 10 + (c.param & 15);
                    if (c.effect == 0xB) jump = c.param;
                    if (c.effect == 0xE) {
                        int x = c.param >> 4, y = c.param & 15;
                        if (x == 0xE) delay = y;
                        if (x == 0x6) {
                            if (!y) loopRow = rr;
                            else if (!loopCount) { loopCount = y; loopTo = loopRow; }
                            else if (--loopCount) loopTo = loopRow;
                        }
                    }
                }
                rows.push_back({pat, rr, pos, speed, bpm, delay, first});
                first = false;
                if (loopTo >= 0) { rr = loopTo - 1; continue; }
                if (brk >= 0 || jump >= 0) { nextPos = jump >= 0 ? jump : pos + 1; nextRow = brk >= 0 ? brk : 0; break; }
            }
            pos = nextPos;
            row = nextRow;
        }
    }
    if (rows.empty()) { error = "the song plays no rows"; return ""; }

    // a tick a row (and each of a pattern delay's): what each channel does
    const int nc = m.channels;
    std::vector<std::vector<SidRow>> sid((size_t)nc);
    std::vector<std::vector<DigiRow>> digi((size_t)nc);
    std::vector<double> fpt;        // each tick's frames
    std::vector<bool> barAt;        // a pattern starts at the tick
    struct Ch { int cur = 0; bool sid = false; int note = -1; size_t noteAt = 0; int vibDepth = 0, porta = 3; bool arp = false;
                int arpA = 0, arpB = 0; double digiEnd = 0; int vol = 64, volSaid = 64; };
    std::vector<Ch> chs((size_t)nc);
    std::set<int> keptUsed;         // the kept samples the song plays
    std::map<std::string, int> lost;
    double now = 0;
    for (const Row& w : rows) {
        double dt = w.speed * 2.5 / w.bpm;
        for (int rep = 0; rep <= w.delay; rep++) {
            fpt.push_back(w.speed * 125.0 / w.bpm);
            barAt.push_back(w.patStart && rep == 0);
            for (int ch = 0; ch < nc; ch++) {
                Ch& c = chs[(size_t)ch];
                SidRow sr;
                DigiRow dr;
                sr.kind = c.sid ? 1 : 2;
                if (rep == 0) {
                    const ModCell& cell = m.patterns[w.pat][(size_t)(w.row * nc + ch)];
                    if (cell.sample && cell.sample <= m.samples.size()) c.cur = cell.sample;
                    int s = c.cur - 1;
                    const ModSampleReport* R = s >= 0 ? &r[(size_t)s] : nullptr;
                    bool playable = R && R->verdict != "unused" && R->verdict != "empty";
                    bool isSid = playable && (R->verdict == "sid" || R->verdict == "sid~");
                    bool porta = cell.period && (cell.effect == 3 || cell.effect == 5);
                    bool note = cell.period && playable && !porta;
                    bool cut = (cell.effect == 0xC && cell.param == 0) || (cell.effect == 0xE && (cell.param >> 4) == 0xC);
                    double per = playable ? cell.period * std::pow(2.0, -m.samples[(size_t)s].tuneSemis() / 12) : cell.period;
                    if (cell.effect == 3 && cell.param) c.porta = cell.param;
                    if (note && isSid) {
                        sr.kind = 0;
                        sr.note = modSidNote(*R, per);
                        sr.sample = s;
                        c.sid = true;
                        c.note = sr.note;
                        c.noteAt = sid[(size_t)ch].size();
                        if (c.digiEnd > now) dr.stop = true;
                        c.digiEnd = 0;
                    } else if (note) {
                        char hn[16];
                        snprintf(hn, sizeof hn, "s%02d", R->index);
                        dr.hit = std::string(hn) + ":" + TuneCompiler::noteName(periodNote(cell.period));
                        keptUsed.insert(s);
                        const ModSample& S = m.samples[(size_t)s];
                        c.digiEnd = now + (S.looped() ? 1e9 : S.data.size() / modRate(per));
                        if (c.sid) { sr.kind = 2; c.sid = false; }
                        if (cell.effect == 9) lost["sample offsets"]++;
                    } else if (porta && c.sid && isSid) {
                        int target = modSidNote(*R, per);
                        if (target != c.note && std::abs(target - c.note) <= 127) {
                            sr.kind = 0;
                            sr.note = target;
                            sr.sample = s;
                            sr.glide = target - c.note;
                            sr.gk = c.porta >= 32 ? 1 : c.porta >= 16 ? 2 : c.porta >= 8 ? 3 : c.porta >= 4 ? 4 : 5;
                            c.note = target;
                            c.noteAt = sid[(size_t)ch].size();
                        }
                    } else if (cut || (note && !playable)) {
                        if (c.sid) { sr.kind = 2; c.sid = false; }
                        if (c.digiEnd > now) { dr.stop = true; c.digiEnd = 0; }
                    }
                    // the volume (a digi voice's: vol:; a SID voice has none): a
                    // sample's own at its number, C, slides (the row's end)
                    if (playable && cell.sample) c.vol = m.samples[(size_t)s].volume;
                    int slide = 0;
                    if (cell.effect == 0xC) c.vol = std::min<int>(64, cell.param);
                    if (cell.effect == 0xA || cell.effect == 0x5 || cell.effect == 0x6)
                        slide = ((cell.param >> 4) ? (cell.param >> 4) : -(cell.param & 15)) * (w.speed - 1);
                    if (cell.effect == 0xE && (cell.param >> 4) == 0xA) slide = cell.param & 15;
                    if (cell.effect == 0xE && (cell.param >> 4) == 0xB) slide = -(cell.param & 15);
                    c.vol = std::max(0, std::min(64, c.vol + slide));
                    if (c.vol != c.volSaid && (c.digiEnd > now || !dr.hit.empty())) { dr.vol = c.vol; c.volSaid = c.vol; }
                    if (porta && !c.sid) lost["slides to a note not sounding on the SID"]++;
                    // arpeggio: this row's (off where a row has none)
                    if (c.sid) {
                        bool want = cell.effect == 0 && cell.param;
                        int a = cell.param >> 4, b = cell.param & 15;
                        if (want && (!c.arp || a != c.arpA || b != c.arpB)) {
                            sr.tokens += "arp:" + std::to_string(a) + "," + std::to_string(b) + " ";
                            c.arp = true; c.arpA = a; c.arpB = b;
                        } else if (!want && c.arp) {
                            sr.tokens += "arp:off ";
                            c.arp = false;
                        }
                    }
                    // vibrato: the note's instrument's (the deepest asked)
                    if ((cell.effect == 4 || cell.effect == 6) && c.sid) {
                        if (cell.effect == 4 && (cell.param & 15)) c.vibDepth = cell.param & 15;
                        int d = std::max(1, c.vibDepth);
                        if (sr.kind == 0) sr.vib = d;
                        else if (c.noteAt < sid[(size_t)ch].size()) sid[(size_t)ch][c.noteAt].vib = std::max(sid[(size_t)ch][c.noteAt].vib, d);
                    }
                    switch (cell.effect) {
                    case 0x1: case 0x2: lost["pitch slides"]++; break;
                    case 0x5: case 0x6: case 0xA: if (c.sid) lost["volume changes on SID voices"]++; break;
                    case 0x7: lost["tremolo"]++; break;
                    case 0xC: if (cell.param && c.sid) lost["volume changes on SID voices"]++; break;
                    case 0xE:
                        if (((cell.param >> 4) == 0xA || (cell.param >> 4) == 0xB) && c.sid) lost["volume changes on SID voices"]++;
                        if ((cell.param >> 4) == 0x9) lost["retriggers"]++;
                        if ((cell.param >> 4) == 0xD) lost["note delays"]++;
                        if ((cell.param >> 4) == 0x1 || (cell.param >> 4) == 0x2) lost["pitch slides"]++;
                        break;
                    default: break;
                    }
                }
                sid[(size_t)ch].push_back(sr);
                digi[(size_t)ch].push_back(dr);
            }
            now += dt;
        }
    }
    const size_t ticks = fpt.size();

    // the voices: a channel's SID notes, a channel's samples
    std::vector<int> sidCh, digiCh;
    for (int ch = 0; ch < nc; ch++) {
        bool anyS = false, anyD = false;
        for (const auto& x : sid[(size_t)ch]) anyS |= x.kind == 0;
        for (const auto& x : digi[(size_t)ch]) anyD |= !x.hit.empty();
        if (anyS) sidCh.push_back(ch);
        if (anyD) digiCh.push_back(ch);
    }
    if (sidCh.size() > 12) { error = std::to_string(sidCh.size()) + " channels play SID notes: the player has 12 voices"; return ""; }
    // more than 4 channels of samples: the rest folded onto d1-d4 (a hit
    // where the voice has none at that row; else left out, counted)
    std::vector<std::vector<DigiRow>> dv;
    for (size_t k = 0; k < digiCh.size(); k++) {
        const auto& src = digi[(size_t)digiCh[k]];
        if (k < 4) { dv.push_back(src); continue; }
        auto& dst = dv[k % 4];
        for (size_t t = 0; t < ticks; t++) {
            if (src[t].hit.empty()) continue;
            if (dst[t].hit.empty()) { dst[t].hit = src[t].hit; dst[t].stop = false; dst[t].vol = src[t].vol; }
            else lost["hits of more sample channels than the audio DMA's 4"]++;
        }
    }

    // the instruments: a SID sample's (with vibrato, a variant a depth)
    std::map<int, std::set<int>> inst;   // sample: its vibrato depths (0: none)
    for (int ch : sidCh)
        for (const auto& x : sid[(size_t)ch]) if (x.kind == 0) inst[x.sample].insert(x.vib);
    auto instName = [](int s, int vib) {
        char b[16];
        snprintf(b, sizeof b, vib ? "s%02dv%d" : "s%02d", s + 1, vib);
        return std::string(b);
    };

    // the text
    std::string t;
    char b[512];
    auto ident = [](std::string n) {
        for (auto& c : n) if (!std::isalnum((unsigned char)c)) c = '_';
        while (n.find("__") != std::string::npos) n.erase(n.find("__"), 1);
        while (!n.empty() && n.back() == '_') n.pop_back();
        while (!n.empty() && n[0] == '_') n.erase(0, 1);
        if (n.empty() || std::isdigit((unsigned char)n[0])) n = "mod_" + n;
        for (auto& c : n) c = (char)std::tolower((unsigned char)c);
        return n;
    };
    std::string name = ident(o.name.empty() ? m.title : o.name);
    snprintf(b, sizeof b, "; %s: tune45 text of %s (tune45 import: a MOD, %s, %d channels, %d positions)\n",
             m.title.empty() ? "(untitled)" : m.title.c_str(), o.source.c_str(), m.tag.c_str(), nc, m.songLen);
    t += b;
    t += "; Its samples: SID instruments (s01...), or kept as samples on the digi voices (the\n"
         ";  WAVs in " + o.samplesRef + "/, one a note it is played at), by " +
         (o.choices.empty() ? std::string("the measured verdicts (tune45 import --report)") : "the choices in " + o.choices) + ".\n"
         "; A row is a tick; v voices are channels' SID notes, d voices channels' samples.\n";
    if (!lost.empty()) {
        t += "; Not carried over:";
        bool first = true;
        for (const auto& [what, n] : lost) { t += (first ? " " : ", ") + std::to_string(n) + " " + what; first = false; }
        t += "\n";
    }
    t += "\n";
    for (const auto& [s, vibs] : inst) {
        const ModSampleReport& R = r[(size_t)s];
        for (int v : vibs) {
            std::string l = modInstrumentLine(R, instName(s, v));
            if (v) {   // the vibrato: the line's 7th word
                std::vector<std::string> w;
                size_t at = 0;
                while (at < l.size()) {
                    size_t e = l.find(' ', at);
                    if (e == std::string::npos) e = l.size();
                    w.push_back(l.substr(at, e - at));
                    at = e + 1;
                }
                if (w.size() > 6) w[6] = std::to_string(v);
                l.clear();
                for (size_t k = 0; k < w.size(); k++) l += (k ? " " : "") + w[k];
            }
            snprintf(b, sizeof b, "%-56s ; %d %s (%s%s, %d)%s\n", (l + " hr=1").c_str(), R.index,
                     m.samples[(size_t)s].name.c_str(), R.verdict.c_str(), R.chosen ? " chosen" : "", R.score,
                     R.fitted ? ": made the SID's so the samples fit in 64 KB" : "");
            t += b;
        }
    }
    // the kept samples: a WAV each, at the rate of its note (note=: the
    // note played most, the sample's tuning in); its hits at their notes
    // (pitched: one copy); a loop as loop= (the WAV's smpl chunk too)
    if (!keptUsed.empty()) {
        mkdir(o.samplesDir.c_str(), 0777);
        t += "\n";
    }
    for (int k : keptUsed) {
        const ModSampleReport& R = r[(size_t)k];
        const ModSample& S = m.samples[(size_t)k];
        int n0 = (int)std::lround(60 + 12 * std::log2(428.0 / R.medianPeriod) - S.tuneSemis());
        double rateD = modRate(428.0 * std::pow(2.0, -(n0 - 60) / 12.0) * std::pow(2.0, -S.tuneSemis() / 12));
        uint32_t rate = (uint32_t)std::lround(rateD);
        std::string file = fileStem(R, S) + ".wav";
        if (!writeWav8(o.samplesDir + "/" + file, S.data, rate, S.looped(), S.loopStart, S.loopLen,
                       unityNote(R, R.medianPeriod), error))
            return "";
        written.push_back(o.samplesDir + "/" + file);
        std::string opts = " note=" + TuneCompiler::noteName(n0);
        if (S.looped()) {
            char lb[32];
            snprintf(lb, sizeof lb, " loop=%.4f", S.loopStart / (double)rate);
            opts += lb;
        }
        if (R.kind == "hat" && rate >= 16000) opts += " rate=16000";
        if (o.keptBytes) (*o.keptBytes)[k] += S.data.size() * std::min(1.0, (R.kind == "hat" ? 16000.0 : 12000.0) / rate);
        char hn[16];
        snprintf(hn, sizeof hn, "s%02d", R.index);
        snprintf(b, sizeof b, "%-56s ; %d %s (%s%s, %d)\n", ("sample " + std::string(hn) + " " + o.samplesRef + "/" + file + opts).c_str(),
                 R.index, S.name.c_str(), R.verdict.c_str(), R.chosen ? " chosen" : "", R.score);
        t += b;
    }
    t += "\n";
    t += "; voices:";
    for (size_t k = 0; k < sidCh.size(); k++) t += " v" + std::to_string(k + 1) + " channel " + std::to_string(sidCh[k] + 1) + ",";
    for (size_t k = 0; k < dv.size(); k++) t += " d" + std::to_string(k + 1) + " channel " + std::to_string(digiCh[k] + 1) + ",";
    if (t.back() == ',') t.pop_back();
    t += "\n";
    std::string tuneLine = "tune " + name + " " + fmtNum(fpt[0]) + " loop\n";
    // a voice's text: segments, a chunk a MOD pattern
    std::vector<std::pair<std::string, std::vector<std::string>>> vlist;
    auto voiceText = [&](const std::string& head, auto segStart, auto event) {
        std::vector<std::string> chunks;
        size_t k = 0;
        while (k < ticks) {
            if (barAt[k] || chunks.empty()) chunks.push_back("");
            size_t e = k + 1;
            while (e < ticks && !segStart(e) && !barAt[e]) e++;
            chunks.back() += event(k, e - k);
            k = e;
        }
        for (auto& c : chunks) c.erase(0, 1);   // (each event's leading space)
        vlist.push_back({head, chunks});
    };
    std::vector<size_t> tempoAt;
    for (size_t k = 1; k < ticks; k++) if (std::fabs(fpt[k] - fpt[k - 1]) > 1e-9) tempoAt.push_back(k);
    auto tempoHere = [&](size_t k) { return std::binary_search(tempoAt.begin(), tempoAt.end(), k); };
    auto tempoTok = [&](size_t k, bool first) { return first && tempoHere(k) ? "tempo:" + fmtNum(fpt[k]) + " " : std::string(); };
    if (sidCh.empty()) {   // (the tempo needs a v voice: one of rests)
        voiceText("v1:", [&](size_t k) { return tempoHere(k); },
                       [&](size_t k, size_t n) { return " " + tempoTok(k, true) + "r:" + std::to_string(n); });
    }
    for (size_t vi = 0; vi < sidCh.size(); vi++) {
        const auto& tr = sid[(size_t)sidCh[vi]];
        std::string cur;
        bool v1 = vi == 0;
        voiceText("v" + std::to_string(vi + 1) + ":",
            [&](size_t k) {
                return tr[k].kind == 0 || !tr[k].tokens.empty() || (v1 && tempoHere(k)) || (tr[k].kind == 2) != (tr[k - 1].kind == 2);
            },
            [&](size_t k, size_t n) {
                const SidRow& x = tr[k];
                std::string s = " " + tempoTok(k, v1) + x.tokens;
                if (x.kind == 0) {
                    std::string in = instName(x.sample, x.vib);
                    if (in != cur) { s += "@" + in + " "; cur = in; }
                    s += TuneCompiler::noteName(x.note);
                    if (x.glide) s += "^" + std::to_string(x.glide) + (x.gk != 3 ? "," + std::to_string(x.gk) : "");
                } else s += x.kind == 1 ? "~" : "r";
                return s + ":" + std::to_string(n);
            });
    }
    for (size_t di = 0; di < dv.size(); di++) {
        const auto& tr = dv[di];
        voiceText("d" + std::to_string(di + 1) + ":", [&](size_t k) { return !tr[k].hit.empty() || tr[k].stop || tr[k].vol >= 0; },
            [&](size_t k, size_t n) {
                const DigiRow& x = tr[k];
                std::string s = x.vol >= 0 ? " vol:" + std::to_string(x.vol) : "";
                return s + " " + (!x.hit.empty() ? x.hit : x.stop ? "-" : "r") + ":" + std::to_string(n);
            });
    }
    // a chunk played more than once (4 tokens or more): a pattern, called
    // (*p3; again at once: *p3*2); the rest as they are, a line a chunk
    std::map<std::string, int> seen;
    for (const auto& v : vlist)
        for (const auto& c : v.second)
            if (std::count(c.begin(), c.end(), ' ') >= 3) seen[c]++;
    std::map<std::string, std::string> pat;
    std::string defs;
    for (const auto& v : vlist)
        for (const auto& c : v.second)
            if (seen.count(c) && seen[c] > 1 && !pat.count(c)) {
                pat[c] = "p" + std::to_string(pat.size() + 1);
                defs += "pattern " + pat[c] + ": " + c + "\n";
            }
    if (!defs.empty()) t += "; the MOD's patterns played more than once (a voice's part of one each)\n" + defs;
    t += tuneLine;
    for (const auto& [head, chunks] : vlist) {
        t += head;
        for (size_t k = 0; k < chunks.size();) {
            if (k) t += " | \\\n   ";
            auto it = pat.find(chunks[k]);
            if (it == pat.end()) { t += " " + chunks[k++]; continue; }
            size_t n = 1;
            while (k + n < chunks.size() && chunks[k + n] == chunks[k] && n < 255) n++;
            t += " *" + it->second + (n > 1 ? "*" + std::to_string(n) : "");
            k += n;
        }
        t += "\n";
    }
    return t;
}
