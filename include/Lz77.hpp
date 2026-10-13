// Lz77 — the LZ77 coder of Marcus Geelnard's Basic Compression Library
// (lz.c), which nibtools uses for .nbz (a compressed .nib).
//
// Stream: a marker byte, then symbols. A symbol other than the marker is a
// literal. The marker followed by $00 is a literal marker byte; otherwise it
// is followed by a length and an offset, each a variable-size number (7 bits
// per byte, high bit set on all but the last byte, most significant first):
// copy `length` bytes from `offset` bytes back.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace lz77 {

// false on a malformed stream or when the output would pass maxOut
bool decompress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, size_t maxOut);

}  // namespace lz77
