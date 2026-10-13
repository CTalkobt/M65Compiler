#include "DiskImage.hpp"
#include <iostream>
#include "GzipHelper.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
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
    // Map both ASCII cases to PETSCII uppercase ($41-$5A)
    // This is the standard CBM filename encoding
    std::string result;
    result.reserve(ascii.size());
    for (unsigned char c : ascii) {
        if (c >= 'a' && c <= 'z') result += (char)(c - 32); // a-z → A-Z ($41-$5A)
        else if (c >= 'A' && c <= 'Z') result += (char)c;   // A-Z stays $41-$5A
        else result += (char)c;
    }
    return result;
}

std::string DiskImage::petsciiToAscii(const std::string& petscii) {
    // Map PETSCII to uppercase ASCII for consistent round-tripping.
    // PETSCII $41-$5A (unshifted uppercase) → ASCII A-Z
    // PETSCII $C1-$DA (shifted uppercase) → ASCII A-Z
    std::string result;
    result.reserve(petscii.size());
    for (unsigned char c : petscii) {
        if (c >= 0xC1 && c <= 0xDA) result += (char)(c - 0x60); // shifted → A-Z
        else if (c >= 0x41 && c <= 0x5A) result += (char)c;     // unshifted → A-Z (same)
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
    // In slot 0, bytes 0-1 are the directory sector's link to the next one:
    // an entry never overwrites them (a new entry has next 0/0, which cut
    // the directory after the sector)
    if (index != 0) {
        p[0] = next.track;
        p[1] = next.sector;
    }
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
    if (!writable() && !(blank_ && canCreateBlank())) {
        std::cerr << "Error: " << path << ": disk45 cannot write this format (read-only: convert it, "
                     "e.g. disk45 convert <image> copy.d64, and change the copy)\n";
        return false;
    }
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

TrackSector DiskImage::firstDirectorySector() const {
    const uint8_t* hdr = sectorData(directoryTrack(), 0);
    if (!hdr) return {0, 0};
    return {hdr[0], hdr[1]};
}

DirEntry DiskImage::findFile(const std::string& name) const {
    TrackSector ts;
    int idx;
    if (findFileEntry(name, ts, idx)) {
        return DirEntry::fromSector(sectorData(ts.track, ts.sector), idx);
    }
    return DirEntry{}; // empty/invalid
}

bool DiskImage::findFileEntry(const std::string& name, TrackSector& dirTS, int& entryIdx) const {
    // Normalize: uppercase for case-insensitive CBM filename matching
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    char petName[16];
    padPetsciiName(petName, upper);

    int dirTrack = directoryTrack();
    // Start from directory first sector (sector 1 on D64/D71, sector 3 on D81)
    (void)dirTrack;
    // Follow the directory chain from its first sector
    TrackSector ts = firstDirectorySector();

    while (ts.track != 0) {
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) return false;

        for (int i = 0; i < 8; i++) {
            DirEntry entry = DirEntry::fromSector(sec, i);
            if (entry.isValid() && entry.isClosed()) {
                // GEOS names hold lower-case ASCII ($61-$7A): compare them
                // folded, as the name is (listings show them as stored)
                char stored[16];
                for (int k = 0; k < 16; k++) {
                    unsigned char c = (unsigned char)entry.filename[k];
                    stored[k] = (char)(c >= 0x61 && c <= 0x7A ? c - 0x20 : c);
                }
                if (std::memcmp(stored, petName, 16) == 0) {
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
    TrackSector ts = firstDirectorySector();
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
// Lock / unlock a file (bit 6 of file type byte)
// ============================================================================

bool DiskImage::lockFile(const std::string& name, bool locked) {
    TrackSector dirTS;
    int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;

    uint8_t* sec = sectorData(dirTS.track, dirTS.sector);
    if (!sec) return false;

    DirEntry e = DirEntry::fromSector(sec, entryIdx);
    if (locked)
        e.fileType |= 0x40;
    else
        e.fileType &= ~0x40;
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
    TrackSector ts = firstDirectorySector();
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

            auto chain = fileSectors(e);
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
    usageCount[key(headerSector().track, headerSector().sector)]++;
    for (auto& ts : systemSectors()) usageCount[key(ts.track, ts.sector)]++;
    // and a GEOS disk's border sector
    if (!geosSignature().empty()) {
        const uint8_t* h = sectorData(headerSector().track, headerSector().sector);
        if (h && h[0xAB] != 0) usageCount[key(h[0xAB], h[0xAC])]++;
    }

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

// ============================================================================
// GEOS files (Schepers' GEOS.TXT and CVT.TXT; the CVT layout as cbmconvert
// reads and writes it)
// ============================================================================

namespace {
const char CVT_SIGNATURE[] = "PRG formatted GEOS file V1.0";
}

bool DiskImage::isGeos(const DirEntry& e) const {
    // a DEL/SEQ/PRG/USR entry with a GEOS file type and structure 0 or 1,
    // and an info block that starts 00 FF 03 15 BF... (the ID bytes may
    // differ, so only the link is checked)
    if ((e.fileType & 0x07) >= 4 || e.unused[0] == 0 || e.recordLen > 1) return false;
    const uint8_t* info = sectorData(e.sideTS.track, e.sideTS.sector);
    return info && info[0] == 0x00 && info[1] == 0xFF;
}

std::string DiskImage::geosTypeName(uint8_t type) {
    static const char* names[] = {"Non-GEOS", "BASIC", "Assembler", "Data", "System", "Desk Accessory",
                                  "Application", "Application Data", "Font", "Printer Driver",
                                  "Input Driver", "Disk Driver", "System Boot", "Temporary",
                                  "Auto-Execute", "Input 128"};
    if (type < sizeof(names) / sizeof(names[0])) return names[type];
    char b[16];
    std::snprintf(b, sizeof b, "type $%02X", type);
    return b;
}

std::string DiskImage::geosSignature() const {
    TrackSector h = headerSector();
    const uint8_t* hdr = sectorData(h.track, h.sector);
    if (!hdr || std::memcmp(hdr + 0xAD, "GEOS format", 11) != 0) return "";
    std::string s((const char*)hdr + 0xAD, 16);
    return s.substr(0, s.find('\0'));
}

std::vector<uint8_t> DiskImage::chainBytes(TrackSector start, bool* ok) const {
    std::vector<uint8_t> out;
    if (ok) *ok = true;
    auto chain = getFileChain(start);
    for (size_t i = 0; i < chain.size(); i++) {
        const uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        if (!sec) { if (ok) *ok = false; break; }
        if (i + 1 == chain.size()) {
            if (sec[0] != 0) { if (ok) *ok = false; }  // broken link
            int last = sec[1];
            if (last >= 2) out.insert(out.end(), sec + 2, sec + last + 1);
        } else {
            out.insert(out.end(), sec + 2, sec + 256);
        }
    }
    return out;
}

std::vector<TrackSector> DiskImage::fileSectors(const DirEntry& e) const {
    std::vector<TrackSector> all;
    if (isGeos(e)) {
        all.push_back(e.sideTS);  // info block
        if (e.recordLen == 1) {
            // VLIR: the record block, then each record's chain
            const uint8_t* rec = sectorData(e.firstDataTS.track, e.firstDataTS.sector);
            all.push_back(e.firstDataTS);
            if (rec)
                for (int i = 0; i < 127; i++)
                    if (rec[2 + 2 * i] != 0) {
                        auto c = getFileChain({rec[2 + 2 * i], rec[3 + 2 * i]});
                        all.insert(all.end(), c.begin(), c.end());
                    }
            return all;
        }
    }
    auto chain = getFileChain(e.firstDataTS);
    all.insert(all.end(), chain.begin(), chain.end());
    if (e.type() == CbmFileType::REL && e.sideTS.track != 0) {
        // the side sectors (and the 1581 super side sector)
        TrackSector ss = e.sideTS;
        if (superSideSectors()) {
            const uint8_t* super = sectorData(ss.track, ss.sector);
            all.push_back(ss);
            ss = super ? TrackSector{super[0], super[1]} : TrackSector{0, 0};
        }
        for (int guard = 0; ss.track != 0 && guard < 126 * 6; guard++) {
            const uint8_t* side = sectorData(ss.track, ss.sector);
            if (!side) break;
            all.push_back(ss);
            ss = {side[0], side[1]};
        }
    }
    return all;
}

bool DiskImage::readGeosCvt(const DirEntry& e, std::vector<uint8_t>& cvt, std::string& err) const {
    if (!isGeos(e)) { err = "not a GEOS file"; return false; }
    const uint8_t* info = sectorData(e.sideTS.track, e.sideTS.sector);
    bool vlir = e.recordLen == 1;
    const uint8_t* rec = vlir ? sectorData(e.firstDataTS.track, e.firstDataTS.sector) : nullptr;
    if (vlir && (!rec || rec[0] != 0 || rec[1] != 0xFF)) { err = "bad VLIR record block"; return false; }

    cvt.assign((2 + vlir) * 254, 0);
    // signature block: the directory entry without its first two bytes,
    // the T/S fields cleared, then the signature
    uint8_t ent[64] = {};
    e.toSector(ent, 1);  // slot 1: bytes 32-63
    std::memcpy(cvt.data(), ent + 32 + 2, 30);
    cvt[1] = cvt[2] = cvt[0x13] = cvt[0x14] = 0;
    std::memcpy(cvt.data() + 30, CVT_SIGNATURE, sizeof CVT_SIGNATURE);
    std::memcpy(cvt.data() + 254, info + 2, 254);
    if (!vlir) {
        bool ok;
        auto data = chainBytes(e.firstDataTS, &ok);
        if (!ok) { err = "broken data chain"; return false; }
        cvt.insert(cvt.end(), data.begin(), data.end());
        return true;
    }
    // VLIR: the record block holds each record's block count and last-byte
    // index; the records follow, each padded to whole 254-byte blocks
    std::memcpy(cvt.data() + 2 * 254, rec + 2, 254);
    size_t length = 3 * 254;
    for (int i = 0; i < 127; i++) {
        uint8_t t = rec[2 + 2 * i], s = rec[3 + 2 * i];
        if (t == 0) continue;  // 00/00 end, 00/FF absent: as stored
        bool ok;
        auto data = chainBytes({t, s}, &ok);
        if (!ok) { err = "broken chain for VLIR record " + std::to_string(i); return false; }
        length = 254 * ((length + 253) / 254);
        cvt.resize(length);
        cvt.insert(cvt.end(), data.begin(), data.end());
        length += data.size();
        // an empty record still takes its block (cbmconvert's writer skips
        // 254 bytes per block)
        if (data.empty()) { length += 254; cvt.resize(length); }
        size_t blocks = (data.size() + 253) / 254;
        if (blocks == 0) blocks = 1;  // an empty record is one sector holding 0 bytes
        cvt[2 * 254 + 2 * i] = (uint8_t)blocks;
        cvt[2 * 254 + 2 * i + 1] = data.empty() ? 1 : (data.size() % 254 ? data.size() % 254 + 1 : 0xFF);
    }
    return true;
}

bool DiskImage::isCvt(const std::vector<uint8_t>& data) {
    return data.size() > 2 * 254 && std::memcmp(data.data() + 33, " formatted GEOS file ", 21) == 0;
}

std::string DiskImage::cvtName(const std::vector<uint8_t>& cvt) {
    std::string n((const char*)cvt.data() + 3, 16);
    while (!n.empty() && (uint8_t)n.back() == 0xA0) n.pop_back();
    return n;
}

TrackSector DiskImage::writeChainBytes(const uint8_t* data, size_t len) {
    // as the formats' addFile: sectors from allocateNextFree, the last one
    // linked 00 / index of its last byte
    size_t n = len ? (len + 253) / 254 : 1;
    std::vector<TrackSector> chain;
    for (size_t i = 0; i < n; i++) {
        TrackSector ts = allocateNextFree(directoryTrack());
        if (ts.isNull()) {
            for (auto& c : chain) freeSector(c.track, c.sector);
            return {0, 0};
        }
        chain.push_back(ts);
    }
    for (size_t i = 0; i < n; i++) {
        uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        std::memset(sec, 0, 256);
        size_t off = i * 254, cnt = std::min<size_t>(254, len - std::min(len, off));
        if (cnt) std::memcpy(sec + 2, data + off, cnt);
        if (i + 1 < n) { sec[0] = chain[i + 1].track; sec[1] = chain[i + 1].sector; }
        else { sec[0] = 0; sec[1] = (uint8_t)(cnt + 1); }
    }
    return chain[0];
}

bool DiskImage::addGeosCvt(const std::vector<uint8_t>& cvt, const std::string& name, std::string& err) {
    if (!isCvt(cvt)) { err = "not a GEOS Convert (CVT) file"; return false; }
    const uint8_t* hdr = cvt.data();
    uint8_t structure = hdr[0x15], geosType = hdr[0x16];
    if ((hdr[0] & 0x07) >= 4 || geosType == 0 || structure > 1) { err = "the CVT holds no valid GEOS file"; return false; }
    bool vlir = structure == 1;
    if (vlir && cvt.size() < 3 * 254) { err = "the CVT is too short for its record block"; return false; }

    // sectors needed: info (+ record block) + the data
    size_t need = 1 + vlir;
    if (vlir) {
        const uint8_t* src = cvt.data() + 2 * 254;
        size_t pos = 3 * 254;
        for (int i = 0; i < 127; i++) {
            uint8_t blocks = src[2 * i], last = src[2 * i + 1];
            if (!blocks) continue;
            if (last < 1) { err = "bad VLIR record size"; return false; }
            size_t len = 254 * (blocks - 1) + last - 1;
            if (pos + len > cvt.size()) { err = "the CVT is shorter than its records"; return false; }
            need += blocks;
            pos += 254 * blocks;
        }
    } else {
        size_t len = cvt.size() - 2 * 254;
        need += len ? (len + 253) / 254 : 1;
    }
    if ((int)need > freeSectors()) { err = "disk full"; return false; }

    std::string n = name.empty() ? cvtName(cvt) : name;
    if (fileExists(n)) { err = "\"" + n + "\" already exists"; return false; }

    // info block
    TrackSector infoTS = allocateNextFree(directoryTrack());
    if (infoTS.isNull()) { err = "disk full"; return false; }
    uint8_t* info = sectorData(infoTS.track, infoTS.sector);
    info[0] = 0x00;
    info[1] = 0xFF;
    std::memcpy(info + 2, cvt.data() + 254, 254);

    TrackSector first;
    if (vlir) {
        uint8_t rec[256] = {};
        rec[0] = 0x00;
        rec[1] = 0xFF;
        const uint8_t* src = cvt.data() + 2 * 254;
        std::memcpy(rec + 2, src, 254);
        size_t pos = 3 * 254;
        for (int i = 0; i < 127; i++) {
            uint8_t blocks = src[2 * i], last = src[2 * i + 1];
            if (!blocks) continue;
            size_t len = 254 * (blocks - 1) + last - 1;
            TrackSector ts = writeChainBytes(cvt.data() + pos, len);
            if (ts.isNull()) { err = "disk full"; return false; }
            rec[2 + 2 * i] = ts.track;
            rec[3 + 2 * i] = ts.sector;
            pos += 254 * blocks;
        }
        first = allocateNextFree(directoryTrack());
        if (first.isNull()) { err = "disk full"; return false; }
        std::memcpy(sectorData(first.track, first.sector), rec, 256);
    } else {
        first = writeChainBytes(cvt.data() + 2 * 254, cvt.size() - 2 * 254);
        if (first.isNull()) { err = "disk full"; return false; }
    }

    // the directory entry from the CVT, with the new T/S and size
    uint8_t ent[32] = {};
    std::memcpy(ent + 2, hdr, 30);
    DirEntry e = DirEntry::fromSector(ent, 0);
    e.next = {0, 0};
    e.fileType = hdr[0] | 0x80;
    e.firstDataTS = first;
    e.sideTS = infoTS;
    if (!name.empty()) padPetsciiName(e.filename, name);
    e.sizeInSectors = (uint16_t)need;
    if (allocDirectoryEntry(e).isNull()) { err = "directory full"; return false; }
    return true;
}

bool DiskImage::makeGeosDisk(std::string& err) {
    // GEOS.TXT: on a 1541/1571 disk the BAM sector 18/0 holds the border
    // sector's T/S at $AB and "GEOS format V1.0" at $AD; the border sector
    // (an empty directory sector) is searched upwards from 19/0, then from 1/0
    if (directoryTrack() != 18 || totalTracks() > 70) {
        err = "GEOS formatting is supported on D64 and D71 images";
        return false;
    }
    uint8_t* bam = sectorData(18, 0);
    if (!bam) { err = "no BAM sector"; return false; }
    if (!geosSignature().empty()) return true;
    TrackSector border{0, 0};
    for (int pass = 0; pass < 2 && border.isNull(); pass++)
        for (int t = pass ? 1 : 19; t <= totalTracks() && border.isNull(); t++) {
            if (t == 18 || (t == 53 && totalTracks() > 35)) continue;  // directory tracks
            for (int s = 0; s < sectorsOnTrack(t); s++)
                if (isSectorFree(t, s)) { border = {(uint8_t)t, (uint8_t)s}; break; }
        }
    if (border.isNull() || !allocateSector(border.track, border.sector)) { err = "disk full"; return false; }
    uint8_t* b = sectorData(border.track, border.sector);
    std::memset(b, 0, 256);
    b[1] = 0xFF;
    bam[0xAB] = border.track;
    bam[0xAC] = border.sector;
    std::memcpy(bam + 0xAD, "GEOS format V1.0", 16);
    return true;
}
