// Gcr — Commodore GCR tracks (1541/1571): decoding a raw track (G64, G71,
// NIB) into sectors and encoding sectors as a standard track.
//
// Layout of a standard sector (Schepers' G64.TXT, Inside Commodore DOS):
// sync (5 x $FF), header (10 GCR bytes: $08, checksum, sector, track, ID2,
// ID1, $0F, $0F), header gap (9 x $55), sync, data block (325 GCR bytes: $07,
// 256 data bytes, checksum, $00, $00), tail gap ($55). Each nybble is 5 GCR
// bits.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gcr {

struct Sector {
    int track = 0;
    int sector = 0;
    uint8_t id1 = 0, id2 = 0;   // the format ID (ID1 = the disk ID's first character)
    uint8_t data[256] = {};
};

// The sectors of a raw track whose header and data checksums are right
// (the first good copy of each). The data is read as a bit stream: a sync is
// 10 or more 1 bits, and the bytes after it need not be byte-aligned. A G64
// track is circular (a sector may wrap from its end to its start); a NIB
// track is a linear read of more than one revolution.
std::vector<Sector> decodeTrack(const uint8_t* bytes, size_t len, bool circular);

// The 1541 speed zone of a track (1-42): 3 for 1-17, 2 for 18-24, 1 for
// 25-30, 0 from 31
int speedZone(int track);
// Sectors per track (1-42) on a 1541
int sectorsOnTrack(int track);
// The length MNIB (and G64.TXT) gives a standard track of a speed zone:
// 6250, 6666, 7142, 7692 bytes
size_t trackLength(int zone);

// A standard track: the sectors 0..n-1 in order (headers carry `track`:
// 36-70 on a 1571's second side), the gaps spread evenly so that the track
// is trackLength(zone) bytes
std::vector<uint8_t> encodeTrack(int track, const std::vector<const uint8_t*>& sectors, uint8_t id1, uint8_t id2,
                                 int zone);

}  // namespace gcr
