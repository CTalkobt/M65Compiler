#pragma once
#include "G64Image.hpp"

// NIB: Raw nibble disk image (nibtools format)
//
// Simple format: 8192 bytes per half-track, 84 half-tracks = 688,128 bytes
// No header — just raw GCR/nibble data from the drive head.
// Read-only: decodes GCR to D64 sectors using G64Image's decoder.

class NibImage : public G64Image {
public:
    bool loadFromFile(const std::string& path) override;
    static bool isNibFile(const std::vector<uint8_t>& data);

private:
    static constexpr int NIB_TRACK_SIZE = 8192;
    static constexpr int NIB_HALF_TRACKS = 84;
    static constexpr int NIB_IMAGE_SIZE = NIB_TRACK_SIZE * NIB_HALF_TRACKS; // 688128
};
