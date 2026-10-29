// PackAudio.cpp — pack45 --suggest-rate (PackAudio.hpp)
#include "PackAudio.hpp"
#include <cmath>
#include <complex>
#include <cstring>

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
