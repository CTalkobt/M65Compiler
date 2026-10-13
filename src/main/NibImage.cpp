#include "NibImage.hpp"
#include "Gcr.hpp"
#include "GzipHelper.hpp"
#include "Lz77.hpp"
#include <cstring>
#include <fstream>

bool NibImage::isNibFile(const std::vector<uint8_t>& data) {
    if (data.size() >= NIB_HEADER_SIZE && std::memcmp(data.data(), "MNIB-1541-RAW", 13) == 0) return true;
    return data.size() == 84 * NIB_TRACK_SIZE;  // headerless: 84 half tracks
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
    if (!isNibFile(raw)) {
        // .nbz: an LZ77-compressed NIB
        std::vector<uint8_t> out;
        if (!lz77::decompress(raw, out, NIB_HEADER_SIZE + 84 * NIB_TRACK_SIZE) || !isNibFile(out)) return false;
        raw.swap(out);
    }

    D64Image::format("", "");
    tracks_.reset(1);
    // the reads: (half track, data) from the header, or 84 in order
    std::vector<std::pair<int, size_t>> reads;
    if (std::memcmp(raw.data(), "MNIB-1541-RAW", 13) == 0) {
        for (int i = 0; i < 120 && 0x10 + 2 * i < (int)NIB_HEADER_SIZE; i++) {
            int ht = raw[0x10 + 2 * i];
            if (ht == 0) break;
            reads.push_back({ht, NIB_HEADER_SIZE + i * NIB_TRACK_SIZE});
        }
    } else {
        for (int i = 0; i < 84; i++) reads.push_back({i + 2, i * NIB_TRACK_SIZE});
    }
    for (auto [ht, off] : reads) {
        if (ht % 2 || off + NIB_TRACK_SIZE > raw.size()) continue;  // half tracks, short files
        int track = ht / 2;
        if (track < 1 || track > totalTracks()) continue;
        for (const auto& s : gcr::decodeTrack(raw.data() + off, NIB_TRACK_SIZE, false)) {
            if (s.track != track || s.sector >= sectorsOnTrack(track)) continue;
            std::memcpy(sectorData(track, s.sector), s.data, 256);
        }
    }
    return true;
}
