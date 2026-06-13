#include "DiskImage.hpp"
#include "GzipHelper.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
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
    // Strip .gz to get the actual disk format extension
    std::string p = gzip::hasGzExtension(path) ? gzip::stripGzExtension(path) : path;
    std::string ext;
    auto dot = p.rfind('.');
    if (dot != std::string::npos) {
        ext = p.substr(dot);
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
    std::vector<uint8_t> raw((size_t)size);
    f.read(reinterpret_cast<char*>(raw.data()), size);
    if (!f.good()) return false;

    // Transparent gzip decompression
    if (gzip::isGzipped(raw)) {
        image_ = gzip::decompress(raw);
        return !image_.empty();
    }
    image_ = std::move(raw);
    return true;
}

bool DiskImage::saveToFile(const std::string& path) const {
    // Transparent gzip compression if path ends in .gz
    if (gzip::hasGzExtension(path)) {
        auto compressed = gzip::compress(image_);
        if (compressed.empty()) return false;
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        return f.good();
    }
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
// Rename a file in the directory
// ============================================================================

bool DiskImage::renameFile(const std::string& oldName, const std::string& newName) {
    TrackSector dirTS;
    int entryIdx;
    if (!findFileEntry(oldName, dirTS, entryIdx)) return false;
    if (fileExists(newName)) return false; // target name already exists

    uint8_t* sec = sectorData(dirTS.track, dirTS.sector);
    if (!sec) return false;

    DirEntry e = DirEntry::fromSector(sec, entryIdx);
    padPetsciiName(e.filename, newName);
    e.toSector(sec, entryIdx);
    return true;
}

// ============================================================================
// Set disk name / ID (default: modifies header sector)
// ============================================================================

bool DiskImage::setDiskName(const std::string&) { return false; }
bool DiskImage::setDiskId(const std::string&) { return false; }

// ============================================================================
// Validate disk image integrity
// ============================================================================

DiskImage::ValidateResult DiskImage::validate() const {
    ValidateResult r;
    if (totalTracks() == 0) {
        r.errors.push_back("Validation not supported for archive formats");
        return r;
    }

    // Build a usage map: track×sector → count of files claiming each sector
    int maxTrack = totalTracks();
    std::map<uint64_t, int> usageCount;
    auto key = [](int t, int s) -> uint64_t { return ((uint64_t)t << 16) | s; };

    // Count directory sectors as used
    int dirTrack_ = directoryTrack();
    const uint8_t* hdr = sectorData(dirTrack_, 0);
    if (!hdr) {
        r.errors.push_back("Cannot read header sector");
        return r;
    }

    // Walk directory and trace all file chains
    TrackSector ts = {hdr[0], hdr[1]};
    while (ts.track != 0) {
        usageCount[key(ts.track, ts.sector)]++; // directory sector itself
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) {
            r.errors.push_back("Broken directory chain at T" +
                std::to_string(ts.track) + " S" + std::to_string(ts.sector));
            r.brokenChains++;
            break;
        }
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (!e.isValid()) continue;
            r.filesFound++;

            auto chain = getFileChain(e.firstDataTS);
            for (auto& cs : chain) {
                r.sectorsUsedByFiles++;
                auto k = key(cs.track, cs.sector);
                usageCount[k]++;
                if (usageCount[k] > 1) {
                    r.crossLinked++;
                    r.errors.push_back("Cross-linked sector T" +
                        std::to_string(cs.track) + " S" + std::to_string(cs.sector));
                }
            }
            // Verify chain endpoint
            if (!chain.empty()) {
                auto& last = chain.back();
                const uint8_t* lastSec = sectorData(last.track, last.sector);
                if (!lastSec) {
                    r.brokenChains++;
                    std::string fn = petsciiToAscii(std::string(e.filename, 16));
                    r.errors.push_back("Broken chain for \"" + fn + "\"");
                }
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    // Also count BAM/header sector itself
    usageCount[key(dirTrack_, 0)]++;

    // Compare BAM vs actual usage
    r.freeSectorsInBAM = freeSectors();
    int totalUsedInBAM = 0;
    for (int t = 1; t <= maxTrack; t++) {
        int spt = sectorsOnTrack(t);
        for (int s = 0; s < spt; s++) {
            bool bamFree = isSectorFree(t, s);
            bool fileUsed = usageCount.count(key(t, s)) > 0;
            if (!bamFree) totalUsedInBAM++;

            if (bamFree && fileUsed) {
                r.errors.push_back("Sector T" + std::to_string(t) +
                    " S" + std::to_string(s) + " used by file but marked free in BAM");
            }
            if (!bamFree && !fileUsed) {
                r.orphanedSectors++;
            }
        }
    }
    r.sectorsMarkedUsed = totalUsedInBAM;

    return r;
}

// ============================================================================
// Factory methods (forward to subclass headers)
// ============================================================================

// These are defined after the subclass headers are included.
// See DiskImageFactory.cpp or the individual format .cpp files.
