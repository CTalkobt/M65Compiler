#include "GeosCvtImage.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

static const char* CVT_SIG_PRG = "PRG formatted GEOS file V1.0";
static const char* CVT_SIG_SEQ = "SEQ formatted GEOS file V1.0";
static constexpr int CVT_SIG_LEN = 28;

bool GeosCvtImage::isCvtFile(const std::vector<uint8_t>& data) {
    if (data.size() < 254) return false;
    // Check for signature at offset 2
    if (std::memcmp(data.data() + 2, CVT_SIG_PRG, CVT_SIG_LEN) == 0) return true;
    if (std::memcmp(data.data() + 2, CVT_SIG_SEQ, CVT_SIG_LEN) == 0) return true;
    return false;
}

const uint8_t* GeosCvtImage::block(int n) const {
    // For PRG-format CVT, the first 2 bytes are the load address,
    // so blocks start at offset 2 with block 0 signature at offset 2.
    // Actually, the entire file is 254-byte blocks INCLUDING the first
    // 2 bytes as part of block 0. For PRG files, bytes 0-1 are the
    // load address which are part of the PRG container, not the CVT data.
    // The CVT spec treats the file as a sequence of 254-byte blocks.
    //
    // However, if stored as a PRG file (with 2-byte load address prefix),
    // the actual CVT data starts at offset 2. If stored as raw/SEQ, it
    // starts at offset 0.
    //
    // Detection: if signature is at offset 2, the first 2 bytes are PRG load addr.
    // If signature is at offset 0, no load address prefix.
    int base = 0;
    if (image_.size() >= CVT_SIG_LEN + 2 &&
        std::memcmp(image_.data() + 2, CVT_SIG_PRG, CVT_SIG_LEN) == 0)
        base = 0; // PRG: signature at +2, block 0 starts at 0 (includes load addr)
    else if (image_.size() >= CVT_SIG_LEN + 2 &&
             std::memcmp(image_.data() + 2, CVT_SIG_SEQ, CVT_SIG_LEN) == 0)
        base = 0;

    // Block n starts at base + n * 254
    // But the first block (signature) includes the 2-byte prefix
    // Treat the whole file as 254-byte blocks starting from byte 0
    // (the 2-byte PRG prefix is part of block 0)
    size_t off = base + (size_t)n * BLOCK_SIZE;
    if (off + BLOCK_SIZE > image_.size()) return nullptr;
    return image_.data() + off;
}

int GeosCvtImage::blockCount() const {
    return (int)(image_.size() / BLOCK_SIZE);
}

// ============================================================================
// GEOS info accessors
// ============================================================================

std::string GeosCvtImage::geosFileName() const {
    // The filename is in the CBM directory entry, not in the CVT info block.
    // For CVT files, we derive the name from the info block's class/parent.
    // Actually, the filename should come from the directory entry that
    // referenced this file. In a standalone CVT, we use the parent app name
    // or the file description.
    const uint8_t* info = block(1);
    if (!info) return "GEOS FILE";

    // Try the parent application name at $4D-$5D (17 bytes)
    std::string name;
    for (int i = 0x4D; i < 0x5E; i++) {
        if (info[i] == 0) break;
        uint8_t c = info[i];
        if (c >= 0x41 && c <= 0x5A) name += (char)c;
        else if (c >= 0xC1 && c <= 0xDA) name += (char)(c - 0x60);
        else if (c >= 0x20 && c < 0x7F) name += (char)c;
    }
    return name.empty() ? "GEOS FILE" : name;
}

uint8_t GeosCvtImage::geosFileType() const {
    const uint8_t* info = block(1);
    return info ? info[0x41] : 0;
}

uint8_t GeosCvtImage::geosFileStructure() const {
    const uint8_t* info = block(1);
    return info ? info[0x42] : 0;
}

bool GeosCvtImage::isVlir() const {
    return geosFileStructure() == 1;
}

std::string GeosCvtImage::geosFileTypeName() const {
    switch (geosFileType()) {
        case 0x00: return "Non-GEOS";
        case 0x01: return "BASIC";
        case 0x02: return "Assembler";
        case 0x03: return "Data";
        case 0x04: return "System";
        case 0x05: return "Desk Accessory";
        case 0x06: return "Application";
        case 0x07: return "Application Data";
        case 0x08: return "Font";
        case 0x09: return "Printer Driver";
        case 0x0A: return "Input Driver";
        case 0x0B: return "Disk Driver";
        case 0x0C: return "System Boot";
        case 0x0D: return "Temporary";
        case 0x0E: return "Auto-Execute";
        case 0x0F: return "Input 128";
        default: return "Unknown ($" + std::to_string(geosFileType()) + ")";
    }
}

std::string GeosCvtImage::geosClassName() const {
    const uint8_t* info = block(1);
    if (!info) return "";
    std::string cls;
    for (int i = 0x4D; i < 0x5E; i++) {
        if (info[i] == 0) break;
        cls += (char)info[i];
    }
    return cls;
}

std::string GeosCvtImage::geosDescription() const {
    const uint8_t* info = block(1);
    if (!info) return "";
    std::string desc;
    for (int i = 0x5E; i < 0xBE; i++) {
        if (info[i] == 0) break;
        uint8_t c = info[i];
        if (c >= 0x20 && c < 0x7F) desc += (char)c;
        else if (c == 0x0D) desc += '\n';
    }
    return desc;
}

int GeosCvtImage::vlirRecordCount() const {
    if (!isVlir()) return 0;
    const uint8_t* rec = block(2);
    if (!rec) return 0;
    int count = 0;
    for (int i = 0; i < 127; i++) {
        uint8_t t = rec[i * 2];
        uint8_t s = rec[i * 2 + 1];
        if (t == 0 && s == 0) continue; // empty
        if (t == 0 && s == 0xFF) continue; // not present
        count++;
    }
    return count;
}

// ============================================================================
// DiskImage interface
// ============================================================================

void GeosCvtImage::format(const std::string&, const std::string&) {
    // Create minimal CVT with signature + empty info block
    image_.assign(BLOCK_SIZE * 2, 0);
    // Signature block
    image_[0] = 0; image_[1] = 0;
    std::memcpy(image_.data() + 2, CVT_SIG_SEQ, CVT_SIG_LEN);
}

std::string GeosCvtImage::diskName() const {
    return "GEOS CVT: " + geosFileName();
}

std::vector<FileInfo> GeosCvtImage::listFiles() const {
    std::vector<FileInfo> files;
    if (blockCount() < 2) return files;

    if (isVlir() && blockCount() >= 3) {
        // VLIR: list each record as a "file"
        const uint8_t* rec = block(2);
        if (!rec) return files;

        int dataBlock = 3; // data starts at block 3 for VLIR
        for (int i = 0; i < 127; i++) {
            uint8_t t = rec[i * 2];
            uint8_t s = rec[i * 2 + 1];
            if (t == 0 && s == 0) continue; // empty slot
            if (t == 0 && s == 0xFF) continue; // not present

            // Record exists — calculate its size from the chain
            // In CVT format, each VLIR record's data is stored as consecutive
            // 254-byte blocks. The record table encodes sizes via the T/S:
            // t=0,s=N means N-1 bytes in last block (like CBM last sector)
            // Otherwise t/s is the original on-disk T/S (ignored in CVT —
            // records are just sequential in the file)
            FileInfo fi;
            fi.name = "RECORD_" + std::to_string(i);
            fi.type = CbmFileType::USR;
            fi.closed = true;
            files.push_back(fi);
        }
    } else {
        // Sequential GEOS file — one "file" entry
        FileInfo fi;
        fi.name = geosFileName();
        fi.type = CbmFileType::USR;
        fi.sizeInBytes = (blockCount() - 2) * BLOCK_SIZE; // minus signature + info
        fi.sizeInSectors = blockCount() - 2;
        fi.closed = true;
        files.push_back(fi);
    }
    return files;
}

bool GeosCvtImage::fileExists(const std::string& name) const {
    auto files = listFiles();
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    for (auto& f : files) {
        std::string fn = f.name;
        std::transform(fn.begin(), fn.end(), fn.begin(), ::toupper);
        if (fn == upper) return true;
    }
    return false;
}

std::vector<uint8_t> GeosCvtImage::readFile(const std::string& name) const {
    if (blockCount() < 2) return {};

    if (!isVlir()) {
        // Sequential: return all data blocks (block 2 onward)
        std::vector<uint8_t> data;
        for (int b = 2; b < blockCount(); b++) {
            const uint8_t* blk = block(b);
            if (!blk) break;
            data.insert(data.end(), blk, blk + BLOCK_SIZE);
        }
        return data;
    }

    // VLIR: extract specific record
    // Try to parse record number from name (RECORD_N)
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    int targetRec = -1;
    if (upper.rfind("RECORD_", 0) == 0) {
        try { targetRec = std::stoi(upper.substr(7)); } catch (...) {}
    }

    if (targetRec < 0) {
        // Return the entire VLIR data (all records concatenated)
        std::vector<uint8_t> data;
        for (int b = 3; b < blockCount(); b++) {
            const uint8_t* blk = block(b);
            if (!blk) break;
            data.insert(data.end(), blk, blk + BLOCK_SIZE);
        }
        return data;
    }

    // Extract specific VLIR record
    const uint8_t* rec = block(2);
    if (!rec) return {};

    // Count blocks for records before targetRec to find the data offset
    int dataBlock = 3;
    for (int i = 0; i < targetRec && i < 127; i++) {
        uint8_t t = rec[i * 2];
        uint8_t s = rec[i * 2 + 1];
        if (t == 0 && s == 0) continue;
        if (t == 0 && s == 0xFF) continue;
        // Count blocks for this record: t is the number of 254-byte blocks
        // In the CVT linearization, each record is stored as t blocks
        // (t=track value from original disk, repurposed as block count)
        dataBlock += t;
    }

    uint8_t rt = rec[targetRec * 2];
    uint8_t rs = rec[targetRec * 2 + 1];
    if (rt == 0) return {}; // empty or not present

    std::vector<uint8_t> data;
    for (int b = 0; b < rt; b++) {
        const uint8_t* blk = block(dataBlock + b);
        if (!blk) break;
        if (b == rt - 1 && rs > 1) {
            // Last block: rs-1 bytes used
            data.insert(data.end(), blk, blk + rs - 1);
        } else {
            data.insert(data.end(), blk, blk + BLOCK_SIZE);
        }
    }
    return data;
}

bool GeosCvtImage::addFile(const std::string&, CbmFileType, const std::vector<uint8_t>&) {
    std::cerr << "Error: GEOS CVT files are read-only\n";
    return false;
}

bool GeosCvtImage::removeFile(const std::string&) {
    std::cerr << "Error: GEOS CVT files are read-only\n";
    return false;
}
