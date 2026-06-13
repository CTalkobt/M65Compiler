#include "DiskImage.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

// Forward declarations for format-specific subclasses
class D64Image;
class D71Image;
class D81Image;
class D65Image;

// ============================================================================
// PETSCII / ASCII conversion
// ============================================================================

std::string DiskImage::asciiToPetscii(const std::string& ascii) {
    std::string result;
    result.reserve(ascii.size());
    for (char c : ascii) {
        if (c >= 'a' && c <= 'z') result += (char)(c - 32);
        else if (c >= 'A' && c <= 'Z') result += (char)(c + 32);
        else result += c;
    }
    return result;
}

std::string DiskImage::petsciiToAscii(const std::string& petscii) {
    std::string result;
    result.reserve(petscii.size());
    for (unsigned char c : petscii) {
        if (c >= 0xC1 && c <= 0xDA) result += (char)(c - 0x60); // shifted uppercase → A-Z
        else if (c >= 0x41 && c <= 0x5A) result += (char)(c + 0x20); // unshifted → a-z
        else if (c == 0xA0 || c == 0x00) break; // padding / null
        else result += (char)c;
    }
    return result;
}

void DiskImage::padPetsciiName(char* dest, const std::string& name, int len) {
    std::string pet = asciiToPetscii(name);
    std::memset(dest, 0xA0, len); // pad with shifted space
    int copyLen = std::min((int)pet.size(), len);
    std::memcpy(dest, pet.data(), copyLen);
}

DiskFormat DiskImage::formatFromExtension(const std::string& path) {
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
    if (ext == ".d64") return DiskFormat::D64;
    if (ext == ".d71") return DiskFormat::D71;
    if (ext == ".d81") return DiskFormat::D81;
    if (ext == ".d65") return DiskFormat::D65;
    return DiskFormat::D81; // default
}

// ============================================================================
// DirEntry serialization
// ============================================================================

DirEntry DirEntry::fromSector(const uint8_t* data, int index) {
    const uint8_t* p = data + (index * 32);
    DirEntry e;
    e.next.track = p[0];
    e.next.sector = p[1];
    e.fileType = p[2];
    e.firstDataTS.track = p[3];
    e.firstDataTS.sector = p[4];
    std::memcpy(e.filename, p + 5, 16);
    e.sideTS.track = p[21];
    e.sideTS.sector = p[22];
    e.recordLen = p[23];
    std::memcpy(e.unused, p + 24, 4);
    e.replaceTrk = p[28];
    e.replaceSec = p[29];
    e.sizeInSectors = p[30] | (p[31] << 8);
    return e;
}

void DirEntry::toSector(uint8_t* data, int index) const {
    uint8_t* p = data + (index * 32);
    p[0] = next.track;
    p[1] = next.sector;
    p[2] = fileType;
    p[3] = firstDataTS.track;
    p[4] = firstDataTS.sector;
    std::memcpy(p + 5, filename, 16);
    p[21] = sideTS.track;
    p[22] = sideTS.sector;
    p[23] = recordLen;
    std::memcpy(p + 24, unused, 4);
    p[28] = replaceTrk;
    p[29] = replaceSec;
    p[30] = sizeInSectors & 0xFF;
    p[31] = (sizeInSectors >> 8) & 0xFF;
}

std::string DirEntry::name() const {
    return DiskImage::petsciiToAscii(std::string(filename, 16));
}

std::string DirEntry::petsciiName() const {
    std::string result(filename, 16);
    // Trim trailing $A0 padding
    while (!result.empty() && (unsigned char)result.back() == 0xA0)
        result.pop_back();
    return result;
}

// ============================================================================
// Raw sector access
// ============================================================================

uint8_t* DiskImage::sectorData(int track, int sector) {
    int off = sectorOffset(track, sector);
    if (off < 0 || off + 256 > (int)image_.size()) return nullptr;
    return image_.data() + off;
}

const uint8_t* DiskImage::sectorData(int track, int sector) const {
    int off = sectorOffset(track, sector);
    if (off < 0 || off + 256 > (int)image_.size()) return nullptr;
    return image_.data() + off;
}

// ============================================================================
// File I/O
// ============================================================================

bool DiskImage::loadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto size = f.tellg();
    f.seekg(0);
    image_.resize((size_t)size);
    f.read(reinterpret_cast<char*>(image_.data()), size);
    return f.good();
}

bool DiskImage::saveToFile(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(image_.data()), image_.size());
    return f.good();
}

// ============================================================================
// Common directory operations
// ============================================================================

DirEntry DiskImage::findFile(const std::string& name) const {
    TrackSector ts;
    int idx;
    if (findFileEntry(name, ts, idx)) {
        return DirEntry::fromSector(sectorData(ts.track, ts.sector), idx);
    }
    return DirEntry{}; // empty/invalid
}

bool DiskImage::findFileEntry(const std::string& name, TrackSector& dirTS, int& entryIdx) const {
    char petName[16];
    padPetsciiName(petName, name);

    int dirTrack = directoryTrack();
    // Start from directory first sector (sector 1 on D64/D71, sector 3 on D81)
    const uint8_t* bam = sectorData(dirTrack, 0);
    if (!bam) return false;

    // Follow directory chain from BAM's link
    TrackSector ts;
    ts.track = bam[0];
    ts.sector = bam[1];

    while (ts.track != 0) {
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) return false;

        for (int i = 0; i < 8; i++) {
            DirEntry entry = DirEntry::fromSector(sec, i);
            if (entry.isValid() && entry.isClosed()) {
                if (std::memcmp(entry.filename, petName, 16) == 0) {
                    dirTS = ts;
                    entryIdx = i;
                    return true;
                }
            }
        }

        // Follow chain
        uint8_t nextT = sec[0];
        uint8_t nextS = sec[1];
        if (nextT == 0) break;
        ts.track = nextT;
        ts.sector = nextS;
    }
    return false;
}

std::vector<TrackSector> DiskImage::getFileChain(TrackSector start) const {
    std::vector<TrackSector> chain;
    TrackSector ts = start;
    while (ts.track != 0) {
        chain.push_back(ts);
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) break;
        uint8_t nextT = sec[0];
        uint8_t nextS = sec[1];
        if (nextT == 0) break;
        ts.track = nextT;
        ts.sector = nextS;
        if (chain.size() > 100000) break; // safety: prevent infinite loops
    }
    return chain;
}

TrackSector DiskImage::allocDirectoryEntry(DirEntry& entry) {
    int dirTrack = directoryTrack();
    const uint8_t* bam = sectorData(dirTrack, 0);
    if (!bam) return {0, 0};

    TrackSector ts;
    ts.track = bam[0];
    ts.sector = bam[1];
    TrackSector prevTS = {0, 0};

    while (ts.track != 0) {
        uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) return {0, 0};

        for (int i = 0; i < 8; i++) {
            DirEntry existing = DirEntry::fromSector(sec, i);
            if (!existing.isValid()) {
                // Found empty slot
                entry.toSector(sec, i);
                return ts;
            }
        }

        prevTS = ts;
        uint8_t nextT = sec[0];
        uint8_t nextS = sec[1];
        if (nextT == 0) break;
        ts.track = nextT;
        ts.sector = nextS;
    }

    // Need to allocate a new directory sector
    TrackSector newDir = allocateNextFree(directoryTrack());
    if (newDir.isNull()) return {0, 0};

    // Link previous directory sector to new one
    if (prevTS.track != 0) {
        uint8_t* prev = sectorData(prevTS.track, prevTS.sector);
        if (prev) {
            prev[0] = newDir.track;
            prev[1] = newDir.sector;
        }
    }

    // Initialize new directory sector
    uint8_t* newSec = sectorData(newDir.track, newDir.sector);
    if (!newSec) return {0, 0};
    std::memset(newSec, 0, 256);
    newSec[0] = 0;    // no next sector
    newSec[1] = 0xFF; // convention: $FF = last byte used

    // Write entry in first slot
    entry.toSector(newSec, 0);
    return newDir;
}

// ============================================================================
// Factory methods (forward to subclass headers)
// ============================================================================

// These are defined after the subclass headers are included.
// See DiskImageFactory.cpp or the individual format .cpp files.
