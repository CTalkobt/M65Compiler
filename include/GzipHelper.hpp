#pragma once
#include <vector>
#include <string>
#include <cstdint>

// Minimal gzip compress/decompress wrapper.
// Uses system zlib when available; can be replaced with bundled miniz later.

namespace gzip {

// Decompress a gzip-compressed buffer. Returns empty on error.
std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed);

// Compress a buffer to gzip format. Returns empty on error.
std::vector<uint8_t> compress(const std::vector<uint8_t>& data);

// Check if a buffer starts with gzip magic bytes (1F 8B)
inline bool isGzipped(const std::vector<uint8_t>& data) {
    return data.size() >= 2 && data[0] == 0x1F && data[1] == 0x8B;
}

// Check if a filename ends with .gz
bool hasGzExtension(const std::string& path);

// Strip .gz extension from path
std::string stripGzExtension(const std::string& path);

} // namespace gzip
