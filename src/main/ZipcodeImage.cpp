#include "ZipcodeImage.hpp"
#include <cstring>
#include <fstream>
#include <algorithm>

bool ZipcodeImage::isZipcodeFile(const std::string& path) {
    // Check if filename starts with "1!" or ends with a known pattern
    std::string name = path;
    auto slash = name.rfind('/');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    return name.size() >= 2 && name[0] == '1' && name[1] == '!';
}

std::string ZipcodeImage::partPath(const std::string& basePath, int partNum) {
    // Replace the leading digit: "1!name" → "2!name" etc.
    std::string dir, file;
    auto slash = basePath.rfind('/');
    if (slash != std::string::npos) {
        dir = basePath.substr(0, slash + 1);
        file = basePath.substr(slash + 1);
    } else {
        file = basePath;
    }
    if (file.size() >= 2 && file[1] == '!') {
        file[0] = '0' + partNum;
    }
    return dir + file;
}

bool ZipcodeImage::decodePart(const std::vector<uint8_t>& data, int startTrack,
                               int endTrack, int offset) {
    size_t pos = offset;

    // Sector interleave order per zone (from 1541 DOS)
    auto getSectorOrder = [this](int track) -> std::vector<int> {
        int ns = sectorsOnTrack(track);
        std::vector<int> order;
        // Standard 1541 interleave: 10 for data, varies by zone
        int interleave = 10;
        if (track >= 18 && track <= 24) interleave = 3; // dir zone
        else if (track >= 25 && track <= 30) interleave = 10;
        else if (track >= 31) interleave = 10;

        // Generate interleaved order
        std::vector<bool> used(ns, false);
        int s = 0;
        for (int i = 0; i < ns; i++) {
            while (used[s]) s = (s + 1) % ns;
            order.push_back(s);
            used[s] = true;
            s = (s + interleave) % ns;
        }
        return order;
    };

    for (int track = startTrack; track <= endTrack; track++) {
        int ns = sectorsOnTrack(track);
        auto sectorOrder = getSectorOrder(track);

        for (int si = 0; si < ns; si++) {
            if (pos >= data.size()) return false;
            int sector = (si < (int)sectorOrder.size()) ? sectorOrder[si] : si;
            uint8_t* dest = sectorData(track, sector);
            if (!dest) return false;

            // Read compression mode (2 bits from a mode byte)
            // Actually, each sector has its own mode byte
            uint8_t mode = data[pos++];
            int compMode = (mode >> 6) & 3;

            if (compMode == 0) {
                // Uncompressed: 256 raw bytes
                if (pos + 256 > data.size()) return false;
                std::memcpy(dest, data.data() + pos, 256);
                pos += 256;
            } else if (compMode == 1) {
                // Fill: single byte repeated 256 times
                if (pos >= data.size()) return false;
                uint8_t fill = data[pos++];
                std::memset(dest, fill, 256);
            } else if (compMode == 2) {
                // RLE: marker byte followed by data
                // marker byte is stored first, then encoded data
                if (pos >= data.size()) return false;
                uint8_t marker = data[pos++];
                int dpos = 0;
                while (dpos < 256 && pos < data.size()) {
                    uint8_t b = data[pos++];
                    if (b == marker) {
                        if (pos + 1 >= data.size()) break;
                        uint8_t count = data[pos++];
                        uint8_t val = data[pos++];
                        if (count == 0) {
                            // marker itself
                            dest[dpos++] = marker;
                        } else {
                            for (int r = 0; r < count && dpos < 256; r++)
                                dest[dpos++] = val;
                        }
                    } else {
                        dest[dpos++] = b;
                    }
                }
                // Fill remainder with zeros
                while (dpos < 256) dest[dpos++] = 0;
            } else {
                // Mode 3: unused, treat as uncompressed
                if (pos + 256 > data.size()) return false;
                std::memcpy(dest, data.data() + pos, 256);
                pos += 256;
            }
        }
    }
    return true;
}

bool ZipcodeImage::loadFromFile(const std::string& path) {
    if (!isZipcodeFile(path)) return false;

    // Initialize blank D64
    format("", "");

    // Track ranges for each part
    struct Part { int startTrack; int endTrack; int dataOffset; };
    Part parts[] = {
        {1, 8, 4},    // 1!: skip 2-byte load addr + 2-byte disk ID
        {9, 16, 2},   // 2!: skip 2-byte load addr
        {17, 25, 2},  // 3!: skip 2-byte load addr
        {26, 35, 2},  // 4!: skip 2-byte load addr
    };

    for (int p = 0; p < 4; p++) {
        std::string partFile = partPath(path, p + 1);
        std::ifstream f(partFile, std::ios::binary | std::ios::ate);
        if (!f) {
            // Try without path prefix
            if (p > 0) return false;
            return false;
        }
        auto size = f.tellg();
        f.seekg(0);
        std::vector<uint8_t> data((size_t)size);
        f.read(reinterpret_cast<char*>(data.data()), size);

        if (!decodePart(data, parts[p].startTrack, parts[p].endTrack, parts[p].dataOffset))
            return false;
    }

    return true;
}
