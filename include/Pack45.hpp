#pragma once
// Pack45.hpp — packed streams (#345): pack45, ln45 --pack-overlays, and the
// library's unpack() (lib/stdlib/unpack.c, <unpack.h>).
//
// Format 1, a byte-oriented LZ whose copies and fills are DMA jobs:
//   1 byte    the format (1)
//   4 bytes   where it unpacks to: a 28-bit address, low byte first
//   tokens, n = t & $3F:
//     $00-$3F  n + 1 bytes follow as they are
//     $40-$7F  n + 3 bytes of the one byte that follows (a fill)
//     $80-$BF  n + 3 bytes repeat from o + 1 back (1 byte o follows)
//     $C0-$FF  n + 3 bytes repeat from o back (2 bytes o follow, o >= 1)
//     $C0 $00 $00  the end
// A repeat never overlaps what it writes (its offset is at least its
// length): one forward DMA copy does it.
//
// In place: the stream can be loaded inside the area it unpacks into, at
// `inPlace` bytes past its start (or further), so the writing never
// overtakes the reading.
#include <cstdint>
#include <string>
#include <vector>

namespace pack45 {

constexpr uint8_t kFormat = 1;
constexpr int kHeader = 5;

struct Packed {
    std::vector<uint8_t> stream;   // header and tokens
    uint32_t inPlace = 0;          // the least offset from the destination to load it at, in place
};

// data packed to unpack at dest. best: the parse that makes the shortest
// stream (pack45 --best; slower) rather than the greedy one
Packed pack(const std::vector<uint8_t>& data, uint32_t dest, bool best = false);

// The decoder's model: the bytes a stream unpacks to (false: not a valid
// format-1 stream; err says why)
bool unpack(const std::vector<uint8_t>& stream, std::vector<uint8_t>& out, uint32_t* dest, std::string* err = nullptr);

// unpack in place in a model of memory: the stream copied at dest + offset
// in a buffer, decoded as the library does (forward DMA copies); true when
// the result is the data
bool checkInPlace(const std::vector<uint8_t>& data, const Packed& p, uint32_t offset);

}  // namespace pack45
