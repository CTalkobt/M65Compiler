#pragma once
#include "D64Image.hpp"
#include <string>

// Zipcode: 4-pack compressed D64 format
//
// A D64 split into 4 files with RLE compression:
//   1!name: tracks 1-8, starts with load addr + 2-byte disk ID
//   2!name: tracks 9-16
//   3!name: tracks 17-25
//   4!name: tracks 26-35
//
// Each file: 2-byte load address, then compressed sector data.
// Compression modes per sector (2-bit header):
//   00: uncompressed (256 raw bytes)
//   01: fill (1 byte repeated 256 times)
//   10: RLE (marker byte + data, marker+count+value for runs)
//
// Read-only: decodes Zipcode to D64 on load.

class ZipcodeImage : public D64Image {
public:
    // Load from the "1!" file — automatically finds 2!, 3!, 4!
    bool loadFromFile(const std::string& path) override;

    static bool isZipcodeFile(const std::string& path);

private:
    bool decodePart(const std::vector<uint8_t>& data, int startTrack, int endTrack, int offset);
    static std::string partPath(const std::string& basePath, int partNum);
};
