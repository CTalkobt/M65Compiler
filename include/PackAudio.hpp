#pragma once
// PackAudio.hpp — pack45 --suggest-rate: how low a sample rate a sound
// can be kept at. A coder spends the same bytes on every sample, smooth or
// not, and LZ finds nothing to repeat in sampled sound: what makes a sound
// smaller is fewer samples, a lower rate, which loses what lies above its
// half. The energy spectrum says how much that is.
#include <cstdint>
#include <string>
#include <vector>

namespace pack45 {

struct Wav {
    uint32_t rate = 0;
    int channels = 0, bits = 0;
    std::vector<double> samples;   // mono (the channels' mean), -1..1
};

// a WAV file's samples: PCM of 8, 16, 24 or 32 bits, or 32-bit float
bool readWav(const std::vector<uint8_t>& file, Wav& w, std::string* err);

// the frequency (Hz) below which `keep` (0..1) of the sound's energy lies,
// of the energy below `band` Hz (what the rate it is played at can hold;
// 0: all of it)
double energyBelow(const Wav& w, double keep, double band = 0);

// the lowest of the usual rates (4000 .. 48000 Hz, at most the sound's own)
// whose low-pass (0.45 of the rate, a practical filter's pass band) is
// above f
uint32_t rateFor(double f, uint32_t own);

}  // namespace pack45
