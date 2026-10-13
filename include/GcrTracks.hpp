// GcrTracks — the track table of a G64/G71 file (Schepers' G64.TXT), shared
// by G64Image (1541) and G71Image (1571).
//
// load() decodes every full track into the disk image's sectors and keeps
// the raw tracks; save() writes a G64/G71 in which a track whose sectors are
// unchanged since load keeps its original GCR (copy protection, extra
// syncs, half tracks), and a changed or missing track is encoded as a
// standard track (Gcr::encodeTrack). A new image's tracks are all standard.
//
// File: "GCR-1541"/"GCR-1571", version 0, entry count (84 or 168: tracks
// and half tracks; G71 entries 84-167 are the second side), maximum track
// size (7928); 4-byte track offsets, 4-byte speed zones (0-3, or an offset
// to a speed block, kept as it is); each track block is a 2-byte length and
// the GCR bytes, padded to the maximum size with $FF.
#pragma once

#include "DiskImage.hpp"
#include <string>
#include <vector>

class GcrTracks {
public:
    static bool isGcrFile(const std::vector<uint8_t>& data, bool* doubleSided = nullptr);

    // Decode a G64/G71 file into img (formatted blank first by the caller);
    // sides: 1 or 2
    bool load(const std::vector<uint8_t>& file, DiskImage& img, int sides);
    // A new image: no raw tracks (all standard on save)
    void reset(int sides);
    std::vector<uint8_t> save(const DiskImage& img) const;

    int sectorsDecoded() const { return decoded_; }

private:
    int sides_ = 1;
    int entries_ = 84;                       // per file
    uint16_t maxTrack_ = 7928;
    std::vector<std::vector<uint8_t>> raw_;  // per entry (empty: none)
    std::vector<uint32_t> speed_;            // per entry, as loaded
    std::vector<uint8_t> speedBlocks_;       // speed-offset blocks are copied
    std::vector<std::vector<uint8_t>> loadedSectors_;  // per logical track: its sectors at load
    uint8_t id1_ = 0, id2_ = 0;              // the format ID found in the headers
    bool haveId_ = false;
    int decoded_ = 0;

    // The logical track (1-35 / 36-70) of a table entry, 0 for a half track
    int logicalTrack(int entry) const;
};
