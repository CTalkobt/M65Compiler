#pragma once
#include "DiskImage.hpp"
#include "D64Image.hpp"

// G64: GCR-encoded 1541 disk image (raw track-level data)
// G71: GCR-encoded 1571 disk image (double-sided)
//
// Header (12 bytes):
//   $00-$07: Signature "GCR-1541" or "GCR-1571"
//   $08: Version (0)
//   $09: Number of tracks (84 for G64, 168 for G71)
//   $0A-$0B: Max track size (LE, typically 7928)
//
// Track offset table: 4 bytes per track (absolute file offset, 0=no data)
// Speed zone table: 4 bytes per track (0-3 = constant speed zone)
// Track data: 2-byte size prefix + GCR-encoded data
//
// Read-only: GCR decoding extracts standard D64/D71 sectors.
// Write not supported (encoding GCR is complex and format-dependent).

class G64Image : public D64Image {
public:
    // Override loadFromFile to decode GCR → D64 sectors
    bool loadFromFile(const std::string& path);

    // Override to prevent saving as G64 (we decode to D64 internally)
    bool saveToFile(const std::string& path) const;

    static bool isG64File(const std::vector<uint8_t>& data);
    bool isDoubleSided() const { return doubleSided_; }

private:
    bool doubleSided_ = false;

    // GCR nybble-to-binary decode table
    static const uint8_t gcrDecode[32];

    // Decode 5 GCR bytes → 4 data bytes
    static bool decodeGcrGroup(const uint8_t* gcr5, uint8_t* out4);

    // Decode a full GCR track, extracting sector data into the D64 image
    bool decodeTrack(const uint8_t* trackData, int trackSize, int trackNum);

    // Find sync mark (10+ consecutive 1-bits) in track data
    static int findSync(const uint8_t* data, int size, int startBit);
};
