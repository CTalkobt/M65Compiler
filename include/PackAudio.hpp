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

// a Sun/NeXT .au file's: mu-law, A-law, PCM of 8, 16, 24 or 32 bits, float
bool readAu(const std::vector<uint8_t>& file, Wav& w, std::string* err);

// an MP3's (minimp3, third_party/minimp3)
bool readMp3(const std::vector<uint8_t>& file, Wav& w, std::string* err);

// any sound ffmpeg reads (MP4/M4A, and whatever else), decoded by it to a
// WAV: ffmpeg must be on the PATH
bool readFfmpeg(const std::string& path, Wav& w, std::string* err);

// a sound file, not samples: WAV, AU or MP3 by its first bytes, or by its
// name one ffmpeg reads (.mp3 .mp4 .m4a .aac .ogg .oga .opus .flac)
bool isSoundFile(const std::string& path, const std::vector<uint8_t>& file);

// a sound by what it is (its first bytes): WAV, AU, MP3, else ffmpeg's
bool readSound(const std::string& path, const std::vector<uint8_t>& file, Wav& w, std::string* err);

// the samples at another rate: a windowed-sinc low-pass at 0.45 of the
// lower of the two rates, then interpolated
std::vector<double> resample(const Wav& w, uint32_t rate);

// to 8-bit samples for the audio DMA: signed (two's complement), or
// unsigned (offset by 128). normalise: the loudest sample to full scale
std::vector<uint8_t> toBytes(std::vector<double> samples, bool isUnsigned, bool normalise);

// the frequency (Hz) below which `keep` (0..1) of the sound's energy lies,
// of the energy below `band` Hz (what the rate it is played at can hold;
// 0: all of it)
double energyBelow(const Wav& w, double keep, double band = 0);

// the lowest of the usual rates (4000 .. 48000 Hz, at most the sound's own)
// whose low-pass (0.45 of the rate, a practical filter's pass band) is
// above f
uint32_t rateFor(double f, uint32_t own);

}  // namespace pack45
