#include "NibImage.hpp"
#include "GzipHelper.hpp"
#include <cstring>
#include <fstream>

bool NibImage::isNibFile(const std::vector<uint8_t>& data) {
    // NIB files are exactly 688128 bytes (84 half-tracks × 8192)
    // or close to it. No header/magic — detect by size.
    return data.size() == NIB_IMAGE_SIZE ||
           (data.size() > 680000 && data.size() < 700000 &&
            data.size() % NIB_TRACK_SIZE == 0);
}

bool NibImage::loadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto fileSize = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> raw((size_t)fileSize);
    f.read(reinterpret_cast<char*>(raw.data()), fileSize);
    if (!f.good()) return false;

    if (gzip::isGzipped(raw)) {
        raw = gzip::decompress(raw);
        if (raw.empty()) return false;
    }

    if (!isNibFile(raw)) return false;

    // Initialize blank D64
    format("", "");

    // Decode each full track (skip half-tracks)
    int numHalfTracks = (int)(raw.size() / NIB_TRACK_SIZE);
    for (int ht = 0; ht < numHalfTracks; ht += 2) {
        int realTrack = (ht / 2) + 1;
        if (realTrack > 35) break;

        const uint8_t* trackData = raw.data() + ht * NIB_TRACK_SIZE;
        decodeTrack(trackData, NIB_TRACK_SIZE, realTrack);
    }

    return true;
}
