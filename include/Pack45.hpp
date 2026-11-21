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
//
// Format 2, an image in up to 16 colours (#666; texture_demo's delta coding):
//   1 byte    the format (2)
//   4 bytes   where it unpacks to
//   1 byte    w / 2 (1-128: w is even, at most 256)
//   2 bytes   h, low byte first
//   1 byte    N, the colours (1-16)
//   1 byte    the prediction: 0 the pixel above, 1 the one to its left
//             (column 0 from above; row 0's "above" is rank 0)
//   N bytes   the order: rank r's palette entry (0-15)
//   N bytes   the code lengths of the symbols 0..N-1 (0: not used; at most 8)
//   the codes, row by row, MSB first: a canonical Huffman code of each
//   pixel's (rank - prediction) mod N; then zero bytes up to what the
//   decoder reads ahead (it takes a byte while it holds 8 bits or fewer)
// It unpacks to h rows of w / 2 bytes, 4 bits a pixel, the left one the low
// nibble, each the pixel's palette entry. In place: a row is written after
// it is decoded, so the stream can lie inside the area as far in as
// `inPlace` (the most a row's end gets ahead of the reading).
#include <cstdint>
#include <string>
#include <vector>

namespace pack45 {

constexpr uint8_t kFormat = 1;
constexpr uint8_t kFormatImage = 2;
constexpr int kHeader = 5;
constexpr int kImageHeader = 10;

// a stream's format byte: one unpack() decodes
inline bool isFormat(uint8_t f) { return f == kFormat || f == kFormatImage; }

struct Packed {
    std::vector<uint8_t> stream;   // header and tokens
    uint32_t inPlace = 0;          // the least offset from the destination to load it at, in place
};

// data packed to unpack at dest. best: the parse that makes the shortest
// stream (pack45 --best; slower) rather than the greedy one
Packed pack(const std::vector<uint8_t>& data, uint32_t dest, bool best = false);

// An image for format 2: w x h pixels, each a palette entry 0-15 (rows first),
// and the order of the colours it uses (rank -> entry; empty: by entry)
struct Image {
    int w = 0, h = 0;
    std::vector<uint8_t> px;
    std::vector<uint8_t> order;
};

// the bytes an image unpacks to: h rows of w / 2, the left pixel the low nibble
std::vector<uint8_t> imageBytes(const Image& img);

// img coded as format 2 to unpack at dest: predict 0 (above), 1 (left), or -1
// both, the smaller kept (*chosen: which). err: why not (w odd or over 256,
// a pixel over 15, more than 16 colours, an order without one of them)
bool packImage(const Image& img, uint32_t dest, int predict, Packed& out, int* chosen, std::string* err);

// The decoder's model: the bytes a stream unpacks to (false: not a valid
// stream of format 1 or 2; err says why)
bool unpack(const std::vector<uint8_t>& stream, std::vector<uint8_t>& out, uint32_t* dest, std::string* err = nullptr);

// unpack in place in a model of memory: the stream copied at dest + offset
// in a buffer, decoded as the library does (forward DMA copies); true when
// the result is the data
bool checkInPlace(const std::vector<uint8_t>& data, const Packed& p, uint32_t offset);

}  // namespace pack45
