#include "G64Image.hpp"
#include "D71Image.hpp"
#include "GzipHelper.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

// GCR 5-bit → 4-bit decode table
// Index: 5-bit GCR value (0-31), Value: 4-bit nybble (0xFF = invalid)
const uint8_t G64Image::gcrDecode[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 00-07
    0xFF, 0x08, 0x00, 0x01, 0xFF, 0x0C, 0x04, 0x05, // 08-0F
    0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x0F, 0x06, 0x07, // 10-17
    0xFF, 0x09, 0x0A, 0x0B, 0xFF, 0x0D, 0x0E, 0xFF, // 18-1F
};

bool G64Image::isG64File(const std::vector<uint8_t>& data) {
    if (data.size() < 12) return false;
    return std::memcmp(data.data(), "GCR-1541", 8) == 0 ||
           std::memcmp(data.data(), "GCR-1571", 8) == 0;
}

bool G64Image::decodeGcrGroup(const uint8_t* gcr5, uint8_t* out4) {
    // Extract 4 five-bit groups from 5 bytes (40 bits total)
    // Byte layout: [aaaaa bbb][bb ccccc] [ddddd eee][ee fffff] [ggggg hhh]
    // Wait — 5 bytes = 40 bits = 8 nybbles × 5 bits, producing 4 bytes

    uint8_t g0 = (gcr5[0] >> 3) & 0x1F;
    uint8_t g1 = ((gcr5[0] << 2) | (gcr5[1] >> 6)) & 0x1F;
    uint8_t g2 = (gcr5[1] >> 1) & 0x1F;
    uint8_t g3 = ((gcr5[1] << 4) | (gcr5[2] >> 4)) & 0x1F;
    uint8_t g4 = ((gcr5[2] << 1) | (gcr5[3] >> 7)) & 0x1F;
    uint8_t g5 = (gcr5[3] >> 2) & 0x1F;
    uint8_t g6 = ((gcr5[3] << 3) | (gcr5[4] >> 5)) & 0x1F;
    uint8_t g7 = gcr5[4] & 0x1F;

    uint8_t n0 = gcrDecode[g0], n1 = gcrDecode[g1];
    uint8_t n2 = gcrDecode[g2], n3 = gcrDecode[g3];
    uint8_t n4 = gcrDecode[g4], n5 = gcrDecode[g5];
    uint8_t n6 = gcrDecode[g6], n7 = gcrDecode[g7];

    if (n0 == 0xFF || n1 == 0xFF || n2 == 0xFF || n3 == 0xFF ||
        n4 == 0xFF || n5 == 0xFF || n6 == 0xFF || n7 == 0xFF)
        return false;

    out4[0] = (n0 << 4) | n1;
    out4[1] = (n2 << 4) | n3;
    out4[2] = (n4 << 4) | n5;
    out4[3] = (n6 << 4) | n7;
    return true;
}

int G64Image::findSync(const uint8_t* data, int size, int startBit) {
    // Find 10+ consecutive 1-bits starting from startBit
    int consecutive = 0;
    for (int bit = startBit; bit < size * 8; bit++) {
        int byteIdx = bit / 8;
        int bitIdx = 7 - (bit % 8);
        if ((data[byteIdx] >> bitIdx) & 1) {
            consecutive++;
            if (consecutive >= 10) {
                // Find end of sync (first 0 bit)
                while (bit + 1 < size * 8) {
                    bit++;
                    byteIdx = bit / 8;
                    bitIdx = 7 - (bit % 8);
                    if (!((data[byteIdx] >> bitIdx) & 1)) {
                        // Return byte-aligned position after sync
                        return (bit / 8);
                    }
                }
                return -1;
            }
        } else {
            consecutive = 0;
        }
    }
    return -1;
}

bool G64Image::decodeTrack(const uint8_t* trackData, int trackSize, int trackNum) {
    if (trackSize < 10) return false;

    // Scan for sync marks and decode sectors
    int pos = 0;
    int sectorsFound = 0;
    int maxSectors = sectorsOnTrack(trackNum);

    while (pos < trackSize - 10 && sectorsFound < maxSectors) {
        // Find sync mark
        int syncEnd = findSync(trackData, trackSize, pos * 8);
        if (syncEnd < 0 || syncEnd >= trackSize - 10) break;
        pos = syncEnd;

        // After sync: header or data block (5 GCR bytes = 4 data bytes)
        if (pos + 10 > trackSize) break;

        // Decode header: 10 GCR bytes → 8 data bytes
        uint8_t hdr[8];
        if (!decodeGcrGroup(trackData + pos, hdr) ||
            !decodeGcrGroup(trackData + pos + 5, hdr + 4)) {
            pos++;
            continue;
        }

        // Check for sector header (block type $08)
        if (hdr[0] != 0x08) {
            pos++;
            continue;
        }

        // Header format: $08, checksum, sector, track, id2, id1, $0F, $0F
        uint8_t sectorNum = hdr[2];
        uint8_t hdrTrack = hdr[3];

        if (hdrTrack != trackNum || sectorNum >= maxSectors) {
            pos++;
            continue;
        }

        // Skip header gap (~9 bytes) and find data sync
        pos += 10;
        syncEnd = findSync(trackData, trackSize, pos * 8);
        if (syncEnd < 0 || syncEnd >= trackSize - 325) {
            pos++;
            continue;
        }
        pos = syncEnd;

        // Decode data block: 325 GCR bytes → 260 data bytes
        // (1 block type + 256 data + 1 checksum + 2 padding = 260 bytes)
        // 260 bytes × 10/8 = 325 GCR bytes
        if (pos + 325 > trackSize) break;

        uint8_t decoded[260];
        bool ok = true;
        for (int i = 0; i < 65 && ok; i++) {
            ok = decodeGcrGroup(trackData + pos + i * 5, decoded + i * 4);
        }
        if (!ok) { pos++; continue; }

        // Check data block type ($07)
        if (decoded[0] != 0x07) { pos++; continue; }

        // Copy 256 data bytes to the D64 sector
        uint8_t* dest = sectorData(trackNum, sectorNum);
        if (dest) {
            std::memcpy(dest, decoded + 1, 256);
            sectorsFound++;
        }

        pos += 325;
    }

    return sectorsFound > 0;
}

bool G64Image::loadFromFile(const std::string& path) {
    // Read the raw G64/G71 file
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto fileSize = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> raw((size_t)fileSize);
    f.read(reinterpret_cast<char*>(raw.data()), fileSize);
    if (!f.good()) return false;

    // Decompress if gzipped
    if (gzip::isGzipped(raw)) {
        raw = gzip::decompress(raw);
        if (raw.empty()) return false;
    }

    if (!isG64File(raw)) return false;

    doubleSided_ = (std::memcmp(raw.data(), "GCR-1571", 8) == 0);
    int numTracks = raw[9];
    // uint16_t maxTrackSize = raw[0x0A] | (raw[0x0B] << 8);

    // Allocate a blank D64 (or D71 for G71) image
    int d64Tracks = doubleSided_ ? 70 : 35;
    format("", "");

    // Parse track offset table (4 bytes per track, starting at $0C)
    for (int t = 0; t < numTracks && t < d64Tracks * 2; t++) {
        int tableOff = 0x0C + t * 4;
        if (tableOff + 4 > (int)raw.size()) break;

        uint32_t trackOff = raw[tableOff] | (raw[tableOff + 1] << 8) |
                            (raw[tableOff + 2] << 16) | (raw[tableOff + 3] << 24);
        if (trackOff == 0) continue; // no data for this track

        // Half-tracks: only process full tracks (even indices = 0,2,4,... → tracks 1,2,3,...)
        if (t % 2 != 0) continue;
        int realTrack = (t / 2) + 1;
        if (realTrack > d64Tracks) continue;

        if (trackOff + 2 > (int)raw.size()) continue;
        uint16_t trackSize = raw[trackOff] | (raw[trackOff + 1] << 8);
        if (trackOff + 2 + trackSize > (int)raw.size()) continue;

        const uint8_t* trackData = raw.data() + trackOff + 2;
        decodeTrack(trackData, trackSize, realTrack);
    }

    return true;
}

bool G64Image::saveToFile(const std::string& path) const {
    // Save as D64 (decoded), not as G64
    // The GCR-encoded format is read-only
    return DiskImage::saveToFile(path);
}
