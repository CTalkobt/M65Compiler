#include "GeosCvtImage.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

bool GeosCvtImage::isCvtFile(const std::vector<uint8_t>& data) {
    return DiskImage::isCvt(data);
}

const uint8_t* GeosCvtImage::block(int n) const {
    size_t off = (size_t)n * BLOCK_SIZE;
    if (off + BLOCK_SIZE > image_.size()) return nullptr;
    return image_.data() + off;
}

int GeosCvtImage::blockCount() const {
    return (int)((image_.size() + BLOCK_SIZE - 1) / BLOCK_SIZE);
}

// ============================================================================
// GEOS info accessors
// ============================================================================

std::string GeosCvtImage::geosFileName() const {
    if (image_.size() < 19) return "GEOS FILE";
    return cvtName(image_);
}

uint8_t GeosCvtImage::geosFileType() const {
    return image_.size() > 0x16 ? image_[0x16] : 0;
}

uint8_t GeosCvtImage::geosFileStructure() const {
    return image_.size() > 0x15 ? image_[0x15] : 0;
}

bool GeosCvtImage::isVlir() const {
    return geosFileStructure() == 1;
}

std::string GeosCvtImage::geosFileTypeName() const {
    return geosTypeName(geosFileType());
}

// ASCII text from the info block (block 1), up to a $00
std::string GeosCvtImage::infoString(int off, int len) const {
    const uint8_t* info = block(1);
    if (!info) return "";
    std::string s;
    for (int i = off; i < off + len && i < BLOCK_SIZE; i++) {
        uint8_t c = info[i];
        if (c == 0) break;
        if (c == 0x0D) s += '\n';
        else if (c >= 0x20 && c < 0x7F) s += (char)c;
    }
    return s;
}

std::string GeosCvtImage::geosClassName() const { return infoString(0x4B, 20); }
std::string GeosCvtImage::geosAuthor() const { return infoString(0x5F, 20); }
std::string GeosCvtImage::geosDescription() const { return infoString(0x9E, 0x60); }

bool GeosCvtImage::recordSpan(int rec, size_t& off, size_t& len) const {
    const uint8_t* table = block(2);
    if (!isVlir() || !table || rec < 0 || rec >= 127) return false;
    size_t pos = 3 * BLOCK_SIZE;
    for (int i = 0; i < 127; i++) {
        uint8_t blocks = table[2 * i], last = table[2 * i + 1];
        if (blocks == 0) {
            if (i == rec) return false;
            continue;
        }
        if (i == rec) {
            off = pos;
            len = (size_t)BLOCK_SIZE * (blocks - 1) + (last ? last - 1 : 0);
            if (off > image_.size()) return false;
            len = std::min(len, image_.size() - off);
            return true;
        }
        pos += (size_t)BLOCK_SIZE * blocks;
    }
    return false;
}

int GeosCvtImage::vlirRecordCount() const {
    int count = 0;
    size_t off, len;
    for (int i = 0; i < 127; i++)
        if (recordSpan(i, off, len)) count++;
    return count;
}

// ============================================================================
// DiskImage interface
// ============================================================================

void GeosCvtImage::format(const std::string&, const std::string&) {
    // a signature block and an empty info block (a sequential file)
    image_.assign(BLOCK_SIZE * 2, 0);
    image_[0] = 0x83;
    std::memset(image_.data() + 3, 0xA0, 16);
    static const char sig[] = "PRG formatted GEOS file V1.0";
    std::memcpy(image_.data() + 30, sig, sizeof sig);
}

std::string GeosCvtImage::diskName() const {
    return "GEOS CVT: " + geosFileName();
}

std::vector<FileInfo> GeosCvtImage::listFiles() const {
    std::vector<FileInfo> files;
    if (!isCvt(image_)) return files;
    if (isVlir()) {
        size_t off, len;
        for (int i = 0; i < 127; i++) {
            if (!recordSpan(i, off, len)) continue;
            FileInfo fi;
            fi.name = "RECORD_" + std::to_string(i);
            fi.type = CbmFileType::USR;
            fi.sizeInBytes = (uint32_t)len;
            fi.sizeInSectors = (uint16_t)((len + 253) / 254);
            files.push_back(fi);
        }
    } else {
        FileInfo fi;
        fi.name = geosFileName();
        fi.type = (CbmFileType)(image_[0] & 0x07);
        size_t len = image_.size() > 2 * BLOCK_SIZE ? image_.size() - 2 * BLOCK_SIZE : 0;
        fi.sizeInBytes = (uint32_t)len;
        fi.sizeInSectors = (uint16_t)((len + 253) / 254);
        files.push_back(fi);
    }
    return files;
}

bool GeosCvtImage::fileExists(const std::string& name) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    for (auto& f : listFiles()) {
        std::string fn = f.name;
        std::transform(fn.begin(), fn.end(), fn.begin(), ::toupper);
        if (fn == upper) return true;
    }
    return false;
}

std::vector<uint8_t> GeosCvtImage::readFile(const std::string& name) const {
    if (!isCvt(image_)) return {};
    if (!isVlir()) {
        // a sequential file: everything after the info block
        return std::vector<uint8_t>(image_.begin() + 2 * BLOCK_SIZE, image_.end());
    }
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper.rfind("RECORD_", 0) != 0) return {};
    int rec = -1;
    try { rec = std::stoi(upper.substr(7)); } catch (...) { return {}; }
    size_t off, len;
    if (!recordSpan(rec, off, len)) return {};
    return std::vector<uint8_t>(image_.begin() + off, image_.begin() + off + len);
}

bool GeosCvtImage::addFile(const std::string&, CbmFileType, const std::vector<uint8_t>&) {
    std::cerr << "Error: a GEOS CVT file holds one GEOS file (disk45 add <disk image> <file.cvt> "
                 "adds it to a disk image)\n";
    return false;
}

bool GeosCvtImage::removeFile(const std::string&) {
    std::cerr << "Error: a GEOS CVT file holds one GEOS file\n";
    return false;
}
