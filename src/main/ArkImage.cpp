#include "ArkImage.hpp"
#include <cstring>
#include <algorithm>

// ============================================================================
// ARK format: byte 0 = file count, then 29-byte entries, then block-aligned data
// ============================================================================

int ArkImage::fileCount() const {
    if (image_.empty()) return 0;
    return image_[0];
}

ArkImage::ArkEntry ArkImage::readEntry(int index) const {
    ArkEntry e = {};
    int off = 1 + index * 29;
    if (off + 29 > (int)image_.size()) return e;
    e.fileType = image_[off];
    e.lsu = image_[off + 1];
    std::memcpy(e.filename, image_.data() + off + 2, 16);
    e.relRecordSize = image_[off + 0x12];
    e.blockCount = image_[off + 0x1B] | (image_[off + 0x1C] << 8);
    return e;
}

int ArkImage::dirSizeBytes() const {
    int count = fileCount();
    int rawSize = 1 + count * 29;
    // Round up to 254-byte block boundary
    return ((rawSize + 253) / 254) * 254;
}

int ArkImage::dataOffset() const {
    return dirSizeBytes();
}

int ArkImage::fileDataOffset(int fileIndex) const {
    int offset = dataOffset();
    for (int i = 0; i < fileIndex; i++) {
        ArkEntry e = readEntry(i);
        offset += e.blockCount * 254;
    }
    return offset;
}

bool ArkImage::isArkFile(const std::vector<uint8_t>& data) {
    if (data.size() < 30) return false;
    int count = data[0];
    if (count == 0 || count > 200) return false;
    // Validate first entry's file type
    uint8_t ft = data[1] & 0x07;
    return ft >= 1 && ft <= 4; // SEQ, PRG, USR, or REL
}

// ============================================================================
// File operations
// ============================================================================

void ArkImage::format(const std::string&, const std::string&) {
    image_.assign(254, 0); // one block: count=0 + padding
}

std::vector<FileInfo> ArkImage::listFiles() const {
    std::vector<FileInfo> files;
    int count = fileCount();
    for (int i = 0; i < count; i++) {
        ArkEntry e = readEntry(i);
        if ((e.fileType & 0x07) == 0) continue;
        FileInfo fi;
        fi.name = petsciiToAscii(std::string(e.filename, 16));
        fi.type = (CbmFileType)(e.fileType & 0x07);
        fi.sizeInSectors = e.blockCount;
        // Compute actual byte size: (blocks-1)*254 + lsu
        fi.sizeInBytes = (e.blockCount > 0) ?
            (e.blockCount - 1) * 254 + e.lsu : 0;
        fi.locked = (e.fileType & 0x40) != 0;
        fi.closed = (e.fileType & 0x80) != 0;
        files.push_back(fi);
    }
    return files;
}

bool ArkImage::fileExists(const std::string& name) const {
    char petName[16];
    padPetsciiName(petName, name);
    int count = fileCount();
    for (int i = 0; i < count; i++) {
        ArkEntry e = readEntry(i);
        if ((e.fileType & 0x07) != 0 && std::memcmp(e.filename, petName, 16) == 0)
            return true;
    }
    return false;
}

std::vector<uint8_t> ArkImage::readFile(const std::string& name) const {
    char petName[16];
    padPetsciiName(petName, name);
    int count = fileCount();
    for (int i = 0; i < count; i++) {
        ArkEntry e = readEntry(i);
        if ((e.fileType & 0x07) == 0) continue;
        if (std::memcmp(e.filename, petName, 16) != 0) continue;

        int off = fileDataOffset(i);
        int byteSize = (e.blockCount > 0) ?
            (e.blockCount - 1) * 254 + e.lsu : 0;
        if (off + byteSize > (int)image_.size()) return {};
        return std::vector<uint8_t>(image_.begin() + off,
                                    image_.begin() + off + byteSize);
    }
    return {};
}

bool ArkImage::addFile(const std::string& name, CbmFileType type,
                       const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false;

    int count = fileCount();
    int blocks = ((int)data.size() + 253) / 254;
    if (blocks == 0) blocks = 1;
    int lsu = (int)data.size() % 254;
    if (lsu == 0 && !data.empty()) lsu = 254;
    if (data.empty()) lsu = 0;

    // Rebuild the archive: new dir + all existing data + new file data
    int newCount = count + 1;
    int newDirRaw = 1 + newCount * 29;
    int newDirBlocks = (newDirRaw + 253) / 254;
    int newDirSize = newDirBlocks * 254;

    std::vector<uint8_t> newImage;
    newImage.resize(newDirSize, 0);
    newImage[0] = (uint8_t)newCount;

    // Copy existing entries
    for (int i = 0; i < count; i++) {
        int srcOff = 1 + i * 29;
        int dstOff = 1 + i * 29;
        if (srcOff + 29 <= (int)image_.size())
            std::memcpy(newImage.data() + dstOff, image_.data() + srcOff, 29);
    }

    // Write new entry
    int entryOff = 1 + count * 29;
    newImage[entryOff] = 0x80 | (uint8_t)type;
    newImage[entryOff + 1] = (uint8_t)lsu;
    padPetsciiName(reinterpret_cast<char*>(newImage.data() + entryOff + 2), name);
    newImage[entryOff + 0x1B] = blocks & 0xFF;
    newImage[entryOff + 0x1C] = (blocks >> 8) & 0xFF;

    // Copy existing file data
    int oldDataOff = dataOffset();
    if (oldDataOff < (int)image_.size()) {
        newImage.insert(newImage.end(),
                       image_.begin() + oldDataOff, image_.end());
    }

    // Append new file data (block-aligned)
    int padded = blocks * 254;
    size_t fileStart = newImage.size();
    newImage.resize(newImage.size() + padded, 0);
    std::memcpy(newImage.data() + fileStart, data.data(),
                std::min((int)data.size(), padded));

    image_ = std::move(newImage);
    return true;
}

bool ArkImage::removeFile(const std::string& name) {
    char petName[16];
    padPetsciiName(petName, name);
    int count = fileCount();
    int targetIdx = -1;
    for (int i = 0; i < count; i++) {
        ArkEntry e = readEntry(i);
        if ((e.fileType & 0x07) != 0 && std::memcmp(e.filename, petName, 16) == 0) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx < 0) return false;

    ArkEntry target = readEntry(targetIdx);
    int targetDataOff = fileDataOffset(targetIdx);
    int targetDataSize = target.blockCount * 254;

    // Rebuild without this file
    int newCount = count - 1;
    int newDirRaw = 1 + newCount * 29;
    int newDirBlocks = (newDirRaw + 253) / 254;
    int newDirSize = newDirBlocks * 254;

    std::vector<uint8_t> newImage;
    newImage.resize(newDirSize, 0);
    newImage[0] = (uint8_t)newCount;

    int di = 0;
    for (int i = 0; i < count; i++) {
        if (i == targetIdx) continue;
        int srcOff = 1 + i * 29;
        int dstOff = 1 + di * 29;
        if (srcOff + 29 <= (int)image_.size())
            std::memcpy(newImage.data() + dstOff, image_.data() + srcOff, 29);
        di++;
    }

    // Copy file data, skipping removed file
    int oldDataOff = dataOffset();
    int pos = oldDataOff;
    for (int i = 0; i < count; i++) {
        ArkEntry e = readEntry(i);
        int fsize = e.blockCount * 254;
        if (i != targetIdx && pos + fsize <= (int)image_.size()) {
            newImage.insert(newImage.end(),
                           image_.begin() + pos, image_.begin() + pos + fsize);
        }
        pos += fsize;
    }

    image_ = std::move(newImage);
    return true;
}
