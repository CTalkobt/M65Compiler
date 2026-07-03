#pragma once
#include "D64Image.hpp"

// X64: Extended D64 with 64-byte header
//
// Header (64 bytes):
//   $00-$03: Magic {$43, $15, $41, $64}  ("C", $15, "Ad")
//   $04-$05: Version
//   $06: Disk type ($01 = 1541)
//   $07: Track count (35)
//   $08: Double-sided (0)
//   $09: Error flag
//   $0A-$1F: Reserved (zeros)
//   $20-$3E: Description (31 chars)
//   $3F: Zero
// $40+: Standard D64 image data

class X64Image : public D64Image {
public:
    bool loadFromFile(const std::string& path) override;
    bool saveToFile(const std::string& path) const override;
    static bool isX64File(const std::vector<uint8_t>& data);

private:
    static constexpr int X64_HEADER_SIZE = 64;
    static const uint8_t X64_MAGIC[4];
};
