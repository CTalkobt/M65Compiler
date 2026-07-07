#include "TapImage.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

// ============================================================================
// TAP header access
// ============================================================================

bool TapImage::isTapFile(const std::vector<uint8_t>& data) {
    if (data.size() < TAP_HEADER_SIZE) return false;
    return std::memcmp(data.data(), "C64-TAPE-RAW", 12) == 0 ||
           std::memcmp(data.data(), "C16-TAPE-RAW", 12) == 0;
}

int TapImage::tapVersion() const {
    if (image_.size() < TAP_HEADER_SIZE) return -1;
    return image_[0x0C];
}

int TapImage::tapPlatform() const {
    if (image_.size() < TAP_HEADER_SIZE) return -1;
    return image_[0x0D];
}

std::string TapImage::platformName() const {
    switch (tapPlatform()) {
        case 0: return "C64";
        case 1: return "VIC-20";
        case 2: return "C16/Plus4";
        case 3: return "PET";
        case 4: return "C5x0";
        case 5: return "C6x0/C7x0";
        default: return "Unknown";
    }
}

// ============================================================================
// DiskImage interface (mostly stubs — TAP is read-only)
// ============================================================================

void TapImage::format(const std::string&, const std::string&) {
    // Create empty TAP file with header
    image_.resize(TAP_HEADER_SIZE, 0);
    std::memcpy(image_.data(), "C64-TAPE-RAW", 12);
    image_[0x0C] = 1;  // version 1
    image_[0x0D] = 0;  // C64
    image_[0x0E] = 0;  // PAL
}

bool TapImage::addFile(const std::string&, CbmFileType, const std::vector<uint8_t>&) {
    std::cerr << "Error: TAP tape images are read-only (pulse encoding not supported)" << std::endl;
    return false;
}

bool TapImage::removeFile(const std::string&) {
    std::cerr << "Error: TAP tape images are read-only" << std::endl;
    return false;
}

bool TapImage::fileExists(const std::string& name) const {
    auto files = decodeFiles();
    for (const auto& f : files) {
        if (f.name == name) return true;
    }
    return false;
}

std::vector<FileInfo> TapImage::listFiles() const {
    auto files = decodeFiles();
    std::vector<FileInfo> result;
    for (const auto& f : files) {
        FileInfo fi;
        fi.name = f.name;
        fi.type = f.type;
        fi.sizeInBytes = (uint32_t)f.data.size();
        fi.sizeInSectors = (fi.sizeInBytes + 253) / 254;
        fi.closed = true;
        result.push_back(fi);
    }
    return result;
}

std::vector<uint8_t> TapImage::readFile(const std::string& name) const {
    auto files = decodeFiles();
    for (const auto& f : files) {
        if (f.name == name) {
            // Prepend 2-byte load address (like PRG format)
            std::vector<uint8_t> result;
            result.push_back(f.startAddr & 0xFF);
            result.push_back((f.startAddr >> 8) & 0xFF);
            result.insert(result.end(), f.data.begin(), f.data.end());
            return result;
        }
    }
    return {};
}

// ============================================================================
// TAP pulse decoding
// ============================================================================

std::vector<uint8_t> TapImage::decodePulses() const {
    if (image_.size() < TAP_HEADER_SIZE) return {};

    int version = tapVersion();
    size_t pos = TAP_HEADER_SIZE;
    size_t dataEnd = image_.size();

    // Decode pulse lengths from TAP data
    // CBM ROM loader uses three pulse lengths:
    //   Short  (~$30): represents bit 0 in a pair
    //   Medium (~$42): represents bit 1 in a pair
    //   Long   (~$56): leader/sync signal
    //
    // Bits are encoded in pairs of pulses. Each bit uses two pulses:
    //   0 = short + medium
    //   1 = medium + short
    //
    // A byte = marker bit (new-data flag) + 8 data bits (LSB first) + parity + inter-byte gap

    // First, collect all pulse lengths
    std::vector<uint32_t> pulses;
    while (pos < dataEnd) {
        uint8_t b = image_[pos++];
        if (b != 0) {
            pulses.push_back((uint32_t)b * 8);
        } else {
            // Overflow: version 0 = ~256*8 cycles, version 1 = 3-byte value follows
            if (version >= 1 && pos + 2 < dataEnd) {
                uint32_t lo = image_[pos++];
                uint32_t mi = image_[pos++];
                uint32_t hi = image_[pos++];
                pulses.push_back(lo | (mi << 8) | (hi << 16));
            } else {
                pulses.push_back(256 * 8); // overflow estimate
            }
        }
    }

    // Classify pulses and decode into bytes
    // Threshold: short < 0x1A0, medium < 0x260, long >= 0x260
    // (These are cycle counts; typical C64 PAL values)
    const uint32_t SHORT_MAX = 0x1A0;
    const uint32_t MEDIUM_MAX = 0x260;

    enum PulseType { P_SHORT, P_MEDIUM, P_LONG, P_OVERFLOW };
    auto classify = [&](uint32_t p) -> PulseType {
        if (p < SHORT_MAX) return P_SHORT;
        if (p < MEDIUM_MAX) return P_MEDIUM;
        if (p < 0x400) return P_LONG;
        return P_OVERFLOW;
    };

    // Decode pulse pairs into bits, then bits into bytes
    // Look for leader (sequence of long pulses), then sync pattern,
    // then data bytes.
    std::vector<uint8_t> bytes;
    size_t pi = 0;

    while (pi < pulses.size()) {
        // Skip until we find a leader sequence (8+ long pulses)
        int leaderCount = 0;
        while (pi < pulses.size()) {
            if (classify(pulses[pi]) == P_LONG) {
                leaderCount++;
                pi++;
            } else if (leaderCount >= 8) {
                break; // leader found, now at data
            } else {
                leaderCount = 0;
                pi++;
            }
        }

        if (leaderCount < 8) break; // no more leaders

        // Skip any remaining medium/short sync pulses
        // Then decode bytes from pulse pairs
        // Each byte: 9 pulse pairs (1 marker + 8 data bits) + parity + checksum
        // Actually, CBM encoding: each bit is two pulses (short+medium or medium+short)

        while (pi + 1 < pulses.size()) {
            PulseType p1 = classify(pulses[pi]);
            PulseType p2 = classify(pulses[pi + 1]);

            // End of block: long pulse or overflow
            if (p1 == P_LONG || p1 == P_OVERFLOW) break;

            // Decode a byte: 8 bits + parity bit = 9 bit-pairs = 18 pulses
            if (pi + 17 >= pulses.size()) break;

            // Skip marker bit pair
            pi += 2;

            uint8_t byte = 0;
            bool valid = true;
            for (int bit = 0; bit < 8; bit++) {
                if (pi + 1 >= pulses.size()) { valid = false; break; }
                p1 = classify(pulses[pi]);
                p2 = classify(pulses[pi + 1]);
                pi += 2;

                if (p1 == P_SHORT && p2 == P_MEDIUM) {
                    // bit = 0
                } else if (p1 == P_MEDIUM && p2 == P_SHORT) {
                    byte |= (1 << bit); // bit = 1
                } else {
                    valid = false;
                    break;
                }
            }

            if (!valid) break;

            // Skip parity bit pair
            if (pi + 1 < pulses.size()) pi += 2;

            bytes.push_back(byte);
        }
    }

    return bytes;
}

// ============================================================================
// Extract files from decoded byte stream
// ============================================================================

std::vector<TapImage::TapeFile> TapImage::extractFromBytes(const std::vector<uint8_t>& bytes) const {
    std::vector<TapeFile> files;

    // Scan for CBM tape headers
    // Header block format (192 bytes):
    //   $00: Block type (1=basic, 3=ml, 4=seq header)
    //   $01-$02: Start address (lo/hi)
    //   $03-$04: End address (lo/hi)
    //   $05-$14: Filename (16 chars, padded with $20)
    //   $15-$BF: Padding (unused, filled with $20)
    //
    // After header comes a data block with the file contents.

    size_t pos = 0;
    while (pos + 192 <= bytes.size()) {
        // Look for a valid header block
        uint8_t blockType = bytes[pos];

        // Valid CBM tape block types for headers
        if (blockType != 1 && blockType != 3 && blockType != 4) {
            pos++;
            continue;
        }

        uint16_t startAddr = bytes[pos + 1] | (bytes[pos + 2] << 8);
        uint16_t endAddr = bytes[pos + 3] | (bytes[pos + 4] << 8);

        // Sanity check: end must be >= start, addresses in reasonable range
        if (endAddr < startAddr || startAddr == 0) {
            pos++;
            continue;
        }

        // Extract filename (16 bytes at offset 5)
        std::string name;
        for (int i = 0; i < 16; i++) {
            uint8_t ch = bytes[pos + 5 + i];
            if (ch == 0x20 || ch == 0xA0 || ch == 0) {
                // Check if rest is padding
                bool allPad = true;
                for (int j = i; j < 16; j++) {
                    uint8_t c2 = bytes[pos + 5 + j];
                    if (c2 != 0x20 && c2 != 0xA0 && c2 != 0) { allPad = false; break; }
                }
                if (allPad) break;
            }
            if (ch >= 0x41 && ch <= 0x5A) ch += 0x20; // uppercase PETSCII → lowercase ASCII
            else if (ch >= 0xC1 && ch <= 0xDA) ch -= 0x60; // shifted PETSCII → lowercase
            if (ch >= 32 && ch < 127) name += (char)ch;
        }

        if (name.empty()) {
            pos++;
            continue;
        }

        // Skip past header (192 bytes)
        size_t dataStart = pos + 192;
        uint32_t dataLen = endAddr - startAddr;

        TapeFile tf;
        tf.name = name;
        tf.startAddr = startAddr;
        tf.endAddr = endAddr;
        tf.type = (blockType == 1) ? CbmFileType::PRG : CbmFileType::PRG;

        // Extract data if available
        if (dataStart + dataLen <= bytes.size()) {
            tf.data.assign(bytes.begin() + dataStart, bytes.begin() + dataStart + dataLen);
        } else if (dataStart < bytes.size()) {
            // Partial data — take what's available
            tf.data.assign(bytes.begin() + dataStart, bytes.end());
        }

        files.push_back(tf);

        // Advance past data block
        pos = dataStart + dataLen;
    }

    return files;
}

// ============================================================================
// Main decode entry point
// ============================================================================

std::vector<TapImage::TapeFile> TapImage::decodeFiles() const {
    if (!isTapFile(image_)) return {};
    auto bytes = decodePulses();
    if (bytes.empty()) return {};
    return extractFromBytes(bytes);
}
