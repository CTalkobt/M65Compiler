#include "T64Image.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

bool T64Image::isT64File(const std::vector<uint8_t>& data) {
    if (data.size() < T64_HEADER_SIZE) return false;
    return std::memcmp(data.data(), "C64S tape image file", 20) == 0 ||
           std::memcmp(data.data(), "C64 tape image file", 19) == 0;
}

std::string T64Image::diskName() const {
    if (image_.size() < T64_HEADER_SIZE) return "T64 TAPE";
    std::string name(reinterpret_cast<const char*>(image_.data() + 0x28), 24);
    while (!name.empty() && (name.back() == ' ' || name.back() == 0))
        name.pop_back();
    // Convert to uppercase ASCII
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name.empty() ? "T64 TAPE" : name;
}

int T64Image::maxEntries() const {
    if (image_.size() < T64_HEADER_SIZE) return 0;
    return image_[0x22] | (image_[0x23] << 8);
}

int T64Image::usedEntries() const {
    if (image_.size() < T64_HEADER_SIZE) return 0;
    return image_[0x24] | (image_[0x25] << 8);
}

T64Image::T64Entry T64Image::readEntry(int index) const {
    T64Entry e{};
    size_t off = T64_HEADER_SIZE + index * T64_ENTRY_SIZE;
    if (off + T64_ENTRY_SIZE > image_.size()) return e;

    const uint8_t* p = image_.data() + off;
    e.entryType = p[0];
    e.fileType = p[1];
    e.startAddr = p[2] | (p[3] << 8);
    e.endAddr = p[4] | (p[5] << 8);
    e.dataOffset = p[8] | (p[9] << 8) | (p[10] << 16) | (p[11] << 24);
    std::memcpy(e.filename, p + 16, 16);
    return e;
}

std::string T64Image::entryName(const T64Entry& e) const {
    std::string name(e.filename, 16);
    // Trim trailing spaces and $A0
    while (!name.empty() && ((unsigned char)name.back() == 0x20 ||
           (unsigned char)name.back() == 0xA0 || name.back() == 0))
        name.pop_back();
    // Convert to uppercase ASCII
    for (auto& c : name) {
        unsigned char uc = (unsigned char)c;
        if (uc >= 0xC1 && uc <= 0xDA) c = (char)(uc - 0x60);
        else if (uc >= 0x61 && uc <= 0x7A) c = (char)(uc - 0x20);
        else if (uc >= 0x41 && uc <= 0x5A) {} // already uppercase
    }
    return name;
}

// ============================================================================
// DiskImage interface
// ============================================================================

void T64Image::format(const std::string& diskName, const std::string&) {
    image_.assign(T64_HEADER_SIZE, 0);
    std::memcpy(image_.data(), "C64S tape image file\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32);
    image_[0x20] = 0x00; image_[0x21] = 0x01; // version 1.0
    image_[0x22] = 30; image_[0x23] = 0;       // max 30 entries
    image_[0x24] = 0; image_[0x25] = 0;        // 0 used

    // Tape name
    std::string name = diskName.empty() ? "TAPE" : diskName;
    std::memset(image_.data() + 0x28, 0x20, 24);
    int len = std::min((int)name.size(), 24);
    for (int i = 0; i < len; i++)
        image_[0x28 + i] = (uint8_t)toupper(name[i]);

    // Allocate space for directory entries
    image_.resize(T64_HEADER_SIZE + 30 * T64_ENTRY_SIZE, 0);
}

std::vector<FileInfo> T64Image::listFiles() const {
    std::vector<FileInfo> files;
    int max = maxEntries();
    for (int i = 0; i < max; i++) {
        T64Entry e = readEntry(i);
        if (e.entryType == 0) continue; // free slot

        FileInfo fi;
        fi.name = entryName(e);
        fi.type = (e.fileType == 0x81) ? CbmFileType::SEQ : CbmFileType::PRG;
        fi.sizeInBytes = (e.endAddr > e.startAddr) ? (e.endAddr - e.startAddr) : 0;
        fi.sizeInSectors = (fi.sizeInBytes + 253) / 254;
        fi.closed = true;
        files.push_back(fi);
    }
    return files;
}

bool T64Image::fileExists(const std::string& name) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    int max = maxEntries();
    for (int i = 0; i < max; i++) {
        T64Entry e = readEntry(i);
        if (e.entryType == 0) continue;
        if (entryName(e) == upper) return true;
    }
    return false;
}

std::vector<uint8_t> T64Image::readFile(const std::string& name) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    int max = maxEntries();
    for (int i = 0; i < max; i++) {
        T64Entry e = readEntry(i);
        if (e.entryType == 0) continue;
        if (entryName(e) != upper) continue;

        uint32_t dataLen = (e.endAddr > e.startAddr) ? (e.endAddr - e.startAddr) : 0;
        if (e.dataOffset + dataLen > image_.size()) {
            // Truncate to available data
            dataLen = (e.dataOffset < image_.size()) ? (uint32_t)(image_.size() - e.dataOffset) : 0;
        }

        // Return with 2-byte load address prefix (PRG format)
        std::vector<uint8_t> result;
        result.push_back(e.startAddr & 0xFF);
        result.push_back((e.startAddr >> 8) & 0xFF);
        if (dataLen > 0 && e.dataOffset < image_.size()) {
            result.insert(result.end(),
                          image_.begin() + e.dataOffset,
                          image_.begin() + e.dataOffset + dataLen);
        }
        return result;
    }
    return {};
}

bool T64Image::addFile(const std::string& name, CbmFileType type,
                        const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false; // need at least load address

    // Find a free directory slot
    int max = maxEntries();
    int freeSlot = -1;
    for (int i = 0; i < max; i++) {
        T64Entry e = readEntry(i);
        if (e.entryType == 0) { freeSlot = i; break; }
    }
    if (freeSlot < 0) return false; // directory full

    uint16_t startAddr = data[0] | (data[1] << 8);
    uint16_t endAddr = startAddr + (uint16_t)(data.size() - 2);
    uint32_t dataOffset = (uint32_t)image_.size();

    // Write directory entry
    size_t entryOff = T64_HEADER_SIZE + freeSlot * T64_ENTRY_SIZE;
    if (entryOff + T64_ENTRY_SIZE > image_.size())
        image_.resize(entryOff + T64_ENTRY_SIZE, 0);

    uint8_t* p = image_.data() + entryOff;
    std::memset(p, 0, T64_ENTRY_SIZE);
    p[0] = 1; // normal file
    p[1] = (type == CbmFileType::SEQ) ? 0x81 : 0x82;
    p[2] = startAddr & 0xFF;
    p[3] = (startAddr >> 8) & 0xFF;
    p[4] = endAddr & 0xFF;
    p[5] = (endAddr >> 8) & 0xFF;
    p[8] = dataOffset & 0xFF;
    p[9] = (dataOffset >> 8) & 0xFF;
    p[10] = (dataOffset >> 16) & 0xFF;
    p[11] = (dataOffset >> 24) & 0xFF;

    // Filename (uppercase, $20-padded)
    std::memset(p + 16, 0x20, 16);
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    int nameLen = std::min((int)upper.size(), 16);
    std::memcpy(p + 16, upper.data(), nameLen);

    // Append file data (skip 2-byte load address)
    image_.insert(image_.end(), data.begin() + 2, data.end());

    // Update used count
    int used = usedEntries() + 1;
    image_[0x24] = used & 0xFF;
    image_[0x25] = (used >> 8) & 0xFF;

    return true;
}

bool T64Image::removeFile(const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    int max = maxEntries();
    for (int i = 0; i < max; i++) {
        T64Entry e = readEntry(i);
        if (e.entryType == 0) continue;
        if (entryName(e) != upper) continue;

        // Clear directory entry
        size_t entryOff = T64_HEADER_SIZE + i * T64_ENTRY_SIZE;
        std::memset(image_.data() + entryOff, 0, T64_ENTRY_SIZE);

        // Update used count
        int used = std::max(0, usedEntries() - 1);
        image_[0x24] = used & 0xFF;
        image_[0x25] = (used >> 8) & 0xFF;
        return true;
    }
    return false;
}
