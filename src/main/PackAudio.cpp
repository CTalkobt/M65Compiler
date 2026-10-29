// PackAudio.cpp — pack45 --suggest-rate (PackAudio.hpp)
#include "PackAudio.hpp"
#include <cmath>
#include <complex>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#include "../../third_party/minimp3/minimp3_ex.h"
#pragma GCC diagnostic pop

namespace pack45 {

namespace {

uint32_t le(const uint8_t* p, int n) {
    uint32_t v = 0;
    for (int k = n - 1; k >= 0; k--) v = v << 8 | p[k];
    return v;
}

// in place, n a power of two
void fft(std::vector<std::complex<double>>& a) {
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        std::complex<double> w1 = std::polar(1.0, -2 * M_PI / (double)len);
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w = 1;
            for (size_t k = 0; k < len / 2; k++, w *= w1) {
                std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
            }
        }
    }
}

}  // namespace

bool readWav(const std::vector<uint8_t>& f, Wav& w, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    if (f.size() < 12 || std::string(f.begin(), f.begin() + 4) != "RIFF" ||
        std::string(f.begin() + 8, f.begin() + 12) != "WAVE")
        return fail("not a WAV file");
    int tag = 0;
    size_t at = 12;
    while (at + 8 <= f.size()) {
        std::string id(f.begin() + at, f.begin() + at + 4);
        size_t len = le(&f[at + 4], 4), body = at + 8;
        if (body + len > f.size()) len = f.size() - body;   // (a cut-off file: what there is)
        if (id == "fmt " && len >= 16) {
            tag = (int)le(&f[body], 2);
            w.channels = (int)le(&f[body + 2], 2);
            w.rate = le(&f[body + 4], 4);
            w.bits = (int)le(&f[body + 14], 2);
            if (tag == 0xFFFE && len >= 26) tag = (int)le(&f[body + 24], 2);   // (extensible: its sub-format)
        } else if (id == "data") {
            if (!w.channels || !w.rate) return fail("no fmt chunk before the data");
            if (tag != 1 && !(tag == 3 && w.bits == 32)) return fail("format " + std::to_string(tag) + " (PCM or 32-bit float only)");
            if (tag == 1 && w.bits != 8 && w.bits != 16 && w.bits != 24 && w.bits != 32)
                return fail(std::to_string(w.bits) + "-bit samples");
            int bytes = w.bits / 8, frame = bytes * w.channels;
            for (size_t p = body; p + frame <= body + len; p += frame) {
                double sum = 0;
                for (int c = 0; c < w.channels; c++) {
                    const uint8_t* s = &f[p + c * bytes];
                    double v;
                    if (tag == 3) {
                        uint32_t u = le(s, 4);
                        float x;
                        std::memcpy(&x, &u, 4);
                        v = x;
                    } else if (bytes == 1) v = (s[0] - 128) / 128.0;
                    else {
                        int64_t x = le(s, bytes);
                        if (x >> (w.bits - 1)) x -= (int64_t)1 << w.bits;
                        v = (double)x / (double)((int64_t)1 << (w.bits - 1));
                    }
                    sum += v;
                }
                w.samples.push_back(sum / w.channels);
            }
            return true;
        }
        at = body + len + (len & 1);
    }
    return fail("no data chunk");
}

bool readAu(const std::vector<uint8_t>& f, Wav& w, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    auto be = [&](size_t at) { return (uint32_t)f[at] << 24 | f[at + 1] << 16 | f[at + 2] << 8 | f[at + 3]; };
    if (f.size() < 24 || std::memcmp(f.data(), ".snd", 4) != 0) return fail("not an AU file");
    size_t at = be(4), len = be(8);
    uint32_t enc = be(12);
    w.rate = be(16);
    w.channels = (int)be(20);
    if (!w.rate || w.channels < 1 || at > f.size()) return fail("a bad AU header");
    if (len == 0xFFFFFFFF || at + len > f.size()) len = f.size() - at;   // (unknown, or cut off: what there is)
    // mu-law and A-law (G.711) to linear, -32124..32124 and -32256..32256
    auto ulaw = [](uint8_t u) {
        u = ~u;
        int t = ((u & 0x0F) << 3) + 0x84;
        t <<= (u & 0x70) >> 4;
        return (u & 0x80) ? 0x84 - t : t - 0x84;
    };
    auto alaw = [](uint8_t a) {
        a ^= 0x55;
        int t = (a & 0x0F) << 4, seg = (a & 0x70) >> 4;
        t = seg ? (t + 0x108) << (seg - 1) : t + 8;
        return (a & 0x80) ? t : -t;
    };
    int bytes;
    switch (enc) {
    case 1: case 2: case 27: bytes = 1; break;
    case 3: bytes = 2; break;
    case 4: bytes = 3; break;
    case 5: case 6: bytes = 4; break;
    case 7: bytes = 8; break;
    default: return fail("AU encoding " + std::to_string(enc) + " (mu-law, A-law, PCM or float only)");
    }
    w.bits = enc == 1 || enc == 27 ? 8 : bytes * 8;
    int frame = bytes * w.channels;
    for (size_t p = at; p + frame <= at + len; p += frame) {
        double sum = 0;
        for (int c = 0; c < w.channels; c++) {
            const uint8_t* q = &f[p + c * bytes];
            uint64_t u = 0;
            for (int k = 0; k < bytes; k++) u = u << 8 | q[k];
            double v;
            if (enc == 1) v = ulaw(q[0]) / 32768.0;
            else if (enc == 27) v = alaw(q[0]) / 32768.0;
            else if (enc == 6) { uint32_t u32 = (uint32_t)u; float x; std::memcpy(&x, &u32, 4); v = x; }
            else if (enc == 7) { double x; std::memcpy(&x, &u, 8); v = x; }
            else {
                int64_t x = (int64_t)u;
                if (x >> (bytes * 8 - 1)) x -= (int64_t)1 << (bytes * 8);
                v = (double)x / (double)((int64_t)1 << (bytes * 8 - 1));
            }
            sum += v;
        }
        w.samples.push_back(sum / w.channels);
    }
    return true;
}

bool readMp3(const std::vector<uint8_t>& f, Wav& w, std::string* err) {
    mp3dec_t dec;
    mp3dec_file_info_t info;
    std::memset(&info, 0, sizeof info);
    if (mp3dec_load_buf(&dec, f.data(), f.size(), &info, nullptr, nullptr) || !info.samples || !info.channels) {
        free(info.buffer);
        if (err) *err = "no MP3 frames in it";
        return false;
    }
    w.rate = (uint32_t)info.hz;
    w.channels = info.channels;
    w.bits = 16;
    for (size_t k = 0; k + info.channels <= info.samples; k += info.channels) {
        double sum = 0;
        for (int c = 0; c < info.channels; c++) sum += info.buffer[k + c] / 32768.0;
        w.samples.push_back(sum / info.channels);
    }
    free(info.buffer);
    return true;
}

bool readFfmpeg(const std::string& path, Wav& w, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    int fd[2];
    if (pipe(fd)) return fail("no pipe for ffmpeg");
    pid_t pid = fork();
    if (pid < 0) return fail("cannot start ffmpeg");
    if (pid == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        const char* av[] = {"ffmpeg", "-v", "error", "-nostdin", "-i", path.c_str(), "-vn", "-f", "wav",
                            "-c:a", "pcm_s16le", "-", nullptr};
        execvp(av[0], const_cast<char* const*>(av));
        _exit(127);
    }
    close(fd[1]);
    std::vector<uint8_t> out;
    uint8_t buf[65536];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof buf)) > 0) out.insert(out.end(), buf, buf + n);
    close(fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        return fail("not a WAV, AU or MP3 file, and no ffmpeg on the PATH to read it (MP4, M4A, ...)");
    if (!WIFEXITED(status) || WEXITSTATUS(status)) return fail("ffmpeg could not read it");
    // (ffmpeg writes a WAV to a pipe with its sizes unknown: what there is)
    if (out.size() >= 8) out[4] = out[5] = out[6] = out[7] = 0xFF;
    return readWav(out, w, err);
}

bool isSoundFile(const std::string& path, const std::vector<uint8_t>& f) {
    if (f.size() >= 12 && !std::memcmp(f.data(), "RIFF", 4) && !std::memcmp(f.data() + 8, "WAVE", 4)) return true;
    if (f.size() >= 4 && !std::memcmp(f.data(), ".snd", 4)) return true;
    if (f.size() >= 3 && !std::memcmp(f.data(), "ID3", 3)) return true;
    size_t dot = path.find_last_of('.');
    std::string ext = dot == std::string::npos ? "" : path.substr(dot);
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    for (const char* e : {".mp3", ".mp4", ".m4a", ".aac", ".ogg", ".oga", ".opus", ".flac", ".au", ".snd", ".wav"})
        if (ext == e) return true;
    return false;
}

bool readSound(const std::string& path, const std::vector<uint8_t>& f, Wav& w, std::string* err) {
    if (f.size() >= 12 && !std::memcmp(f.data(), "RIFF", 4) && !std::memcmp(f.data() + 8, "WAVE", 4)) return readWav(f, w, err);
    if (f.size() >= 4 && !std::memcmp(f.data(), ".snd", 4)) return readAu(f, w, err);
    std::string ext = path.size() > 4 ? path.substr(path.size() - 4) : "";
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    bool mp3 = (f.size() >= 3 && !std::memcmp(f.data(), "ID3", 3)) || ext == ".mp3" ||
               (f.size() >= 2 && f[0] == 0xFF && (f[1] & 0xE6) == 0xE2);   // (a frame: layer III)
    if (mp3) return readMp3(f, w, err);
    return readFfmpeg(path, w, err);
}

std::vector<double> resample(const Wav& w, uint32_t rate) {
    const auto& a = w.samples;
    if (rate == w.rate || a.empty()) return a;
    // the low-pass, 255 taps (Blackman window), cut-off in cycles a sample
    double fc = 0.45 * std::min(rate, w.rate) / w.rate;
    const int taps = 255, half = taps / 2;
    std::vector<double> h(taps);
    double sum = 0;
    for (int k = 0; k < taps; k++) {
        double n = k - half, x = 2 * M_PI * fc * n;
        double sinc = n == 0 ? 1 : std::sin(x) / x;
        double win = 0.42 - 0.5 * std::cos(2 * M_PI * k / (taps - 1)) + 0.08 * std::cos(4 * M_PI * k / (taps - 1));
        sum += h[k] = 2 * fc * sinc * win;
    }
    for (double& v : h) v /= sum;
    size_t n = a.size();
    std::vector<double> lp(n);
    for (size_t i = 0; i < n; i++) {
        double y = 0;
        for (int k = 0; k < taps; k++) {
            long j = (long)i + k - half;
            if (j >= 0 && j < (long)n) y += h[k] * a[j];
        }
        lp[i] = y;
    }
    std::vector<double> out;
    double step = (double)w.rate / rate;
    for (double t = 0; t <= (double)(n - 1); t += step) {
        size_t i = (size_t)t;
        double f = t - (double)i;
        out.push_back(i + 1 < n ? lp[i] * (1 - f) + lp[i + 1] * f : lp[i]);
    }
    return out;
}

std::vector<uint8_t> toBytes(std::vector<double> s, bool isUnsigned, bool normalise) {
    double peak = 0;
    for (double v : s) peak = std::max(peak, std::fabs(v));
    double g = normalise && peak > 0 ? 1 / peak : 1;
    std::vector<uint8_t> out;
    out.reserve(s.size());
    for (double v : s) {
        long x = std::lround(v * g * 127);
        x = std::max(-128L, std::min(127L, x));
        out.push_back((uint8_t)(isUnsigned ? x + 128 : x & 0xFF));
    }
    return out;
}

double energyBelow(const Wav& w, double keep, double band) {
    size_t n = 1;
    while (n < w.samples.size()) n <<= 1;
    double mean = 0;
    for (double v : w.samples) mean += v;
    if (!w.samples.empty()) mean /= (double)w.samples.size();
    std::vector<std::complex<double>> a(n, 0.0);
    for (size_t i = 0; i < w.samples.size(); i++) a[i] = w.samples[i] - mean;
    fft(a);
    size_t top = n / 2;
    if (band > 0) top = std::min(top, (size_t)(band * (double)n / w.rate));
    std::vector<double> e(top + 1);
    double total = 0;
    for (size_t k = 0; k <= top; k++) total += e[k] = std::norm(a[k]);
    if (total <= 0) return 0;
    double run = 0;
    for (size_t k = 0; k <= top; k++) {
        run += e[k];
        if (run >= keep * total) return (double)k * w.rate / (double)n;
    }
    return (double)top * w.rate / (double)n;
}

uint32_t rateFor(double f, uint32_t own) {
    static const uint32_t rates[] = {4000, 5000, 6000, 8000, 10000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
    for (uint32_t r : rates)
        if (r >= own || 0.45 * r >= f) return std::min(r, own);
    return own;
}

}  // namespace pack45
