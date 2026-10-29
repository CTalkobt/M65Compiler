#include "TapImage.hpp"
#include <cstring>
#include <cmath>
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

uint32_t TapImage::dataSize() const {
    if (image_.size() < TAP_HEADER_SIZE) return 0;
    return image_[0x10] | (image_[0x11] << 8) | (image_[0x12] << 16) | ((uint32_t)image_[0x13] << 24);
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
        fi.closed = f.state != FileState::Damaged;   // a damaged file lists as *PRG
        result.push_back(fi);
    }
    return result;
}

std::vector<uint8_t> TapImage::readFile(const std::string& name) const {
    auto files = decodeFiles();
    for (const auto& f : files) {
        if (f.name == name) {
            if (f.type == CbmFileType::SEQ) return f.data;
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
// TAP pulse decoding (#142)
// ============================================================================
//
// The KERNAL's tape format (C64, VIC-20, C16 / Plus 4 alike; c64-wiki
// "Datassette Encoding", c2n's decode.c):
//   - a pilot tone of short pulses (about 10 s before a file's first block,
//     2 s before the others, a short gap before a block's second copy);
//   - a byte: a byte marker (long, medium), 8 bits LSB first and an odd
//     parity bit (1 EOR the 8 bits), each bit two pulses: short+medium 0,
//     medium+short 1;
//   - a block: a countdown $89..$81 (first copy) or $09..$01 (second copy),
//     the payload, the checksum (the EOR of the payload), and an optional
//     end-of-data marker (long, short). Each block is written twice.
//   - a file: a 192-byte header block (type 1 relocatable / 3 absolute
//     program: start and end address, 16-character name; 4 a SEQ file's
//     header; 5 end of tape), then a program's data block (end - start
//     bytes), or a SEQ file's 192-byte blocks of type 2 (191 bytes each).
// The pulses' lengths depend on the machine and on the tape's speed: the
// classes are calibrated from each pilot tone, in the platform's ratios.

int TapImage::timingOverride_ = -1;

int TapImage::timingPlatform() const {
    if (timingOverride_ >= 0) return timingOverride_;
    if (image_.size() >= 12 && std::memcmp(image_.data(), "C16-TAPE-RAW", 12) == 0) return 2;
    int p = tapPlatform();
    return p == 1 || p == 2 ? p : 0;
}

std::vector<uint32_t> TapImage::readPulses() const {
    std::vector<uint32_t> raw;   // as stored: full waves (v0, v1) or half-waves (v2)
    if (image_.size() < TAP_HEADER_SIZE) return raw;
    int version = tapVersion();
    size_t pos = TAP_HEADER_SIZE;
    while (pos < image_.size()) {
        uint8_t b = image_[pos++];
        if (b) { raw.push_back((uint32_t)b * 8); continue; }
        // $00: an overflow; v1/v2 give the length (in cycles) in 3 bytes
        if (version >= 1 && pos + 3 <= image_.size()) {
            raw.push_back(image_[pos] | (image_[pos + 1] << 8) | ((uint32_t)image_[pos + 2] << 16));
            pos += 3;
        } else {
            raw.push_back(20000);
        }
    }
    if (version != 2) return raw;
    // half-waves: two make a wave; a pause stands alone (and realigns)
    std::vector<uint32_t> full;
    const uint32_t PAUSE = 8000;
    for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] >= PAUSE || i + 1 >= raw.size()) { full.push_back(raw[i] * (raw[i] >= PAUSE ? 1 : 2)); continue; }
        if (raw[i + 1] >= PAUSE) { full.push_back(raw[i] * 2); continue; }
        full.push_back(raw[i] + raw[i + 1]);
        i++;
    }
    return full;
}

std::vector<TapImage::Block> TapImage::decodeBlocks() const {
    std::vector<Block> blocks;
    auto pulses = readPulses();
    if (pulses.empty()) return blocks;

    // the boundaries between the classes, as multiples of the short pulse:
    // C64 / VIC-20 pulses are about 1 : 1.375 : 1.79 (TAP $30 $42 $56),
    // C16 / Plus 4 about 1 : 2 : 4 (TAP $35 $6A $D4)
    bool c16 = timingPlatform() == 2;
    const double SM = c16 ? 1.5 : 1.19, ML = c16 ? 3.0 : 1.58, MAX = c16 ? 6.0 : 2.4, MIN = c16 ? 0.5 : 0.6;
    enum Cls { S, M, L, X };   // X: noise or a pause
    double shortLen = 0;       // from the latest pilot tone
    std::vector<Cls> cls(pulses.size(), X);

    // a pilot tone: 64 or more pulses within 20% of their mean
    const size_t PILOT = 64;
    size_t runStart = 0;
    double runSum = 0;
    for (size_t i = 0; i <= pulses.size(); i++) {
        bool fits = false;
        if (i < pulses.size()) {
            size_t n = i - runStart;
            double mean = n ? runSum / n : pulses[i];
            fits = n == 0 || std::abs((double)pulses[i] - mean) <= 0.2 * mean;
        }
        if (fits) { runSum += pulses[i]; continue; }
        size_t n = i - runStart;
        if (n >= PILOT) {
            // calibrate from it, and classify from its start on
            shortLen = runSum / n;
            for (size_t k = runStart; k < pulses.size(); k++) {
                double r = pulses[k] / shortLen;
                cls[k] = r < MIN || r >= MAX ? X : r < SM ? S : r < ML ? M : L;
            }
        }
        if (i < pulses.size()) { runStart = i; runSum = pulses[i]; }
    }
    if (shortLen == 0) return blocks;   // no pilot tone: nothing recorded

    // bytes: a long+medium marker each; a block ends at its end marker
    // (long+short) or at anything that is no marker (the next pilot tone)
    size_t i = 0;
    while (i + 1 < cls.size()) {
        if (!(cls[i] == L && cls[i + 1] == M)) { i++; continue; }
        std::vector<uint8_t> bytes;
        int parityErrors = 0;
        bool framing = false;
        while (i + 1 < cls.size() && cls[i] == L && cls[i + 1] == M) {
            i += 2;
            unsigned value = 0, parity = 1;
            int bit = 0;
            for (; bit < 9 && i + 1 < cls.size(); bit++, i += 2) {
                unsigned b;
                if (cls[i] == S && cls[i + 1] == M) b = 0;
                else if (cls[i] == M && cls[i + 1] == S) b = 1;
                else break;
                if (bit < 8) { value |= b << bit; parity ^= b; }
                else if (b != parity) parityErrors++;
            }
            if (bit < 9) { framing = true; break; }
            bytes.push_back((uint8_t)value);
        }
        // a block: the 9-byte countdown, the payload and the checksum
        if (bytes.size() >= 11 && (bytes[0] & 0x7F) == 9) {
            bool countdown = true;
            for (int k = 1; k < 9; k++) if (bytes[k] != bytes[0] - k) countdown = false;
            if (countdown) {
                Block b;
                b.countdown = bytes[0];
                b.payload.assign(bytes.begin() + 9, bytes.end() - 1);
                uint8_t sum = 0;
                for (uint8_t v : b.payload) sum ^= v;
                b.checksumOk = sum == bytes.back();
                b.parityErrors = parityErrors;
                b.framingError = framing;
                blocks.push_back(std::move(b));
            }
        }
        if (framing) i++;   // past the pulse that was no bit
    }
    return blocks;
}

std::vector<TapImage::Record> TapImage::pairCopies(const std::vector<Block>& blocks) {
    std::vector<Record> records;
    auto why = [](const Block& b) {
        std::string w;
        if (b.framingError) w = "pulses that are no bits";
        else if (b.parityErrors) w = std::to_string(b.parityErrors) + (b.parityErrors == 1 ? " parity error" : " parity errors");
        else if (!b.checksumOk) w = "a checksum error";
        return w;
    };
    for (size_t i = 0; i < blocks.size(); i++) {
        const Block& a = blocks[i];
        const Block* first = a.countdown == 0x89 ? &a : nullptr;
        const Block* second = nullptr;
        if (first && i + 1 < blocks.size() && blocks[i + 1].countdown == 0x09) second = &blocks[++i];
        else if (!first) second = &a;   // a second copy whose first copy was not found
        Record r;
        if (first && first->good()) {
            r = {first->payload, FileState::Verified, ""};
        } else if (second && second->good()) {
            r = {second->payload, FileState::Repaired,
                 first ? "a block's first copy has " + why(*first) + ": its second copy is used"
                       : "a block's first copy is missing: its second copy is used"};
        } else {
            const Block& b = first ? *first : *second;
            r = {b.payload, FileState::Damaged,
                 "a block has " + why(b) + (first && second ? " in both copies" : first ? " and no second copy" : "")};
        }
        records.push_back(std::move(r));
    }
    return records;
}

namespace {
std::string tapeName(const std::vector<uint8_t>& p) {
    std::string name;
    int end = 16;
    while (end > 0 && (p[5 + end - 1] == 0x20 || p[5 + end - 1] == 0xA0 || p[5 + end - 1] == 0)) end--;
    for (int i = 0; i < end; i++) {
        uint8_t ch = p[5 + i];
        if (ch >= 0x41 && ch <= 0x5A) ch += 0x20;        // PETSCII upper case -> ASCII lower
        else if (ch >= 0xC1 && ch <= 0xDA) ch -= 0x60;   // shifted letters -> ASCII upper
        if (ch >= 32 && ch < 127) name += (char)ch;
    }
    return name;
}
}  // namespace

std::vector<TapImage::TapeFile> TapImage::filesFromRecords(const std::vector<Record>& records) {
    std::vector<TapeFile> files;
    auto worse = [](TapeFile& f, const Record& r) {
        if ((int)r.state > (int)f.state) { f.state = r.state; f.detail = r.detail; }
    };
    for (size_t i = 0; i < records.size(); i++) {
        const auto& h = records[i].payload;
        if (h.size() != 192) continue;   // not a header (a data block without one)
        uint8_t type = h[0];
        if (type != 1 && type != 3 && type != 4) continue;   // 5: end of tape; 2: SEQ data
        TapeFile f;
        f.name = tapeName(h);
        f.state = records[i].state;
        f.detail = records[i].detail;
        if (type == 4) {
            // a SEQ file: its type-2 blocks, 191 bytes each; the last one's
            // padding ($00 bytes after the data) is dropped
            f.type = CbmFileType::SEQ;
            while (i + 1 < records.size() && records[i + 1].payload.size() == 192 && records[i + 1].payload[0] == 2) {
                const auto& d = records[++i];
                f.data.insert(f.data.end(), d.payload.begin() + 1, d.payload.end());
                worse(f, d);
            }
            while (!f.data.empty() && f.data.back() == 0) f.data.pop_back();
        } else {
            f.type = CbmFileType::PRG;
            f.startAddr = h[1] | (h[2] << 8);
            f.endAddr = h[3] | (h[4] << 8);
            uint32_t length = f.endAddr >= f.startAddr ? f.endAddr - f.startAddr : 0;
            if (i + 1 < records.size() && records[i + 1].payload.size() != 192 || (i + 1 < records.size() && length == 192)) {
                const auto& d = records[++i];
                f.data = d.payload;
                worse(f, d);
                if (f.data.size() != length) {
                    f.state = FileState::Damaged;
                    f.detail = "its data block has " + std::to_string(f.data.size()) + " bytes, the header says " +
                               std::to_string(length);
                }
            } else {
                f.state = FileState::Damaged;
                f.detail = "its data block is missing";
            }
        }
        files.push_back(std::move(f));
    }
    return files;
}

std::vector<TapImage::TapeFile> TapImage::decodeFiles() const {
    if (!isTapFile(image_)) return {};
    return filesFromRecords(pairCopies(decodeBlocks()));
}

std::vector<TapImage::FileReport> TapImage::report() const {
    std::vector<FileReport> out;
    for (const auto& f : decodeFiles()) out.push_back({f.name, f.type, f.state, f.detail});
    return out;
}

bool TapImage::isDamaged(const std::string& name) const {
    for (const auto& f : decodeFiles())
        if (f.name == name) return f.state == FileState::Damaged;
    return false;
}
