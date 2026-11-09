// BasicPlay.hpp — BASIC 65's PLAY strings as a tune for the library's tune
// player (<audio_tune.h>, tune45's data): basic45 --compile's PLAY (#369)
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct PlayTune {
    // the descriptor the run time's __bas_play takes: the voices given (bit
    // n: voice n + 1), the instruments' count, the instrument table's offset
    // in the tune (2), an instrument's bytes, each voice's ticks (6 x 2: 0
    // none), each instrument's envelope (0-9), then the tune
    std::vector<uint8_t> data;
    std::vector<std::string> warnings;
    std::string error;          // (empty: converted)
    bool badValue = false;      // O7-O9, X2-X9: ?ILLEGAL QUANTITY when played
};

// voices[i]: voice i + 1's string (PETSCII), none for an empty argument
PlayTune convertPlay(const std::vector<std::optional<std::string>>& voices);
