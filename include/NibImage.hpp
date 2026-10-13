#pragma once
#include "G64Image.hpp"

// NIB: a nibtools disk read (.nib), and the same compressed with LZ77 (.nbz;
// Geelnard's lz.c stream, see Lz77.hpp).
//
// Layout: a 256-byte header ("MNIB-1541-RAW", version at $0D, then from
// $10 pairs of half-track number (2 = track 1) and density, ended by 0),
// then one $2000-byte read per listed half track, in the header's order.
// A read holds more than one revolution and starts anywhere; the bytes need
// not be aligned to the GCR stream. A headerless file of 84 $2000-byte
// reads (half tracks 1-42.5) is accepted as well.
//
// Read-only: the full tracks are decoded into D64 sectors (convert to write).

class NibImage : public G64Image {
public:
    bool writable() const override { return false; }
    bool canCreateBlank() const override { return false; }
    bool loadFromFile(const std::string& path) override;
    bool saveToFile(const std::string& path) const override { return DiskImage::saveToFile(path); }
    static bool isNibFile(const std::vector<uint8_t>& data);

private:
    static constexpr size_t NIB_TRACK_SIZE = 0x2000;
    static constexpr size_t NIB_HEADER_SIZE = 0x100;
};
