// BasicPlay.hpp — BASIC 65's PLAY strings as streams of the library's tune
// player (<audio_tune.h>): basic45 --compile's PLAY (#369, #478)
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct PlayTune {
    // the descriptor the run time's __bas_play takes: the voices given (bit
    // n: voice n + 1), then each voice's stream (6 x 2: its offset in the
    // program's tune, PlayBank::blob)
    std::vector<uint8_t> data;
    std::vector<std::string> warnings;
    std::string error;          // (empty: converted)
    bool badValue = false;      // O7-O9, X2-X9: ?ILLEGAL QUANTITY when played
};

// The program's PLAY strings in one tune: a header (nine voices: BASIC's 1-3
// on SID 1, 4-6 on SID 3), the ten envelopes as its instruments (the run
// time puts ENVELOPE's values in at each PLAY), each string's stream. Each
// voice starts empty; a PLAY starts the voices it gives on their streams
// (tune_voice_play) while the others go on.
class PlayBank {
public:
    // voices[i]: voice i + 1's string (PETSCII), none for an empty argument
    PlayTune add(const std::vector<std::optional<std::string>>& voices);
    std::vector<uint8_t> blob() const;
    static constexpr int kHeader = 8 + 2 * 9, kInstrument = 17;
    static constexpr int kStreams = kHeader + 10 * kInstrument;   // (the first stream: an empty one)
private:
    std::vector<uint8_t> streams_ = {0xFF};
};
