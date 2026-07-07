#include "D90Image.hpp"
#include <cstring>
#include <algorithm>
#include <set>

int D90Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > totalTracks()) return -1;
    if (sector < 0 || sector >= 32) return -1;
    return ((track - 1) * 32 + sector) * 256;
}

// Build used-sector set by walking all file chains + directory
static std::set<uint64_t> buildUsedSet(const DiskImage* img, int dirTrack) {
    std::set<uint64_t> used;
    auto key = [](int t, int s) -> uint64_t { return ((uint64_t)t << 16) | s; };

    // Mark directory track
    used.insert(key(dirTrack, 0));
    TrackSector ts = {(uint8_t)dirTrack, 1};
    while (ts.track != 0) {
        used.insert(key(ts.track, ts.sector));
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;

        // Walk file chains for each entry
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && !e.firstDataTS.isNull()) {
                TrackSector fts = e.firstDataTS;
                while (fts.track != 0) {
                    used.insert(key(fts.track, fts.sector));
                    const uint8_t* fsec = img->sectorData(fts.track, fts.sector);
                    if (!fsec) break;
                    uint8_t nt = fsec[0], ns = fsec[1];
                    if (nt == 0) break;
                    fts = {nt, ns};
                }
            }
        }

        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return used;
}

void D90Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(totalSectors() * 256, 0);
    uint8_t* hdr = sectorData(39, 0);
    if (hdr) {
        hdr[0] = 39; hdr[1] = 1;
        hdr[2] = 'H';
        std::memset(hdr + 0x06, 0xA0, 16);
        std::string name = diskName.empty() ? "CBM HD" : diskName;
        std::string pet = asciiToPetscii(name);
        std::memcpy(hdr + 0x06, pet.data(), std::min((int)pet.size(), 16));
        hdr[0x18] = diskId.size() > 0 ? asciiToPetscii(diskId)[0] : 'H';
        hdr[0x19] = diskId.size() > 1 ? asciiToPetscii(diskId)[1] : 'D';
    }
    uint8_t* dir = sectorData(39, 1);
    if (dir) { dir[0] = 0; dir[1] = 0xFF; }
}

std::string D90Image::diskName() const {
    const uint8_t* hdr = sectorData(39, 0);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x06), 16));
}

std::string D90Image::diskId() const {
    const uint8_t* hdr = sectorData(39, 0);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x18), 2));
}

bool D90Image::isSectorFree(int track, int sector) const {
    auto used = buildUsedSet(this, 39);
    return used.find(((uint64_t)track << 16) | sector) == used.end();
}

bool D90Image::allocateSector(int, int) { return true; } // no on-disk BAM to update
bool D90Image::freeSector(int, int) { return true; }

TrackSector D90Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = 39;
    auto used = buildUsedSet(this, 39);
    auto key = [](int t, int s) -> uint64_t { return ((uint64_t)t << 16) | s; };
    int maxT = totalTracks();
    for (int delta = 1; delta <= maxT; delta++) {
        for (int dir = 1; dir >= -1; dir -= 2) {
            int t = nearTrack + delta * dir;
            if (t < 1 || t > maxT) continue;
            for (int s = 0; s < 32; s++) {
                if (used.find(key(t, s)) == used.end())
                    return {(uint8_t)t, (uint8_t)s};
            }
        }
    }
    return {0, 0};
}

int D90Image::freeSectors() const {
    auto used = buildUsedSet(this, 39);
    return totalSectors() - (int)used.size();
}

std::vector<FileInfo> D90Image::listFiles() const {
    std::vector<FileInfo> files;
    TrackSector ts = {39, 1};
    while (ts.track != 0) {
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid()) {
                FileInfo fi;
                fi.name = e.name(); fi.type = e.type();
                fi.sizeInSectors = e.sizeInSectors;
                fi.sizeInBytes = e.sizeInSectors * 254;
                fi.closed = e.isClosed();
                files.push_back(fi);
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return files;
}

bool D90Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D90Image::readFile(const std::string& name) const {
    DirEntry e = findFile(name);
    if (!e.isValid()) return {};
    std::vector<uint8_t> data;
    auto chain = getFileChain(e.firstDataTS);
    for (size_t i = 0; i < chain.size(); i++) {
        const uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        if (!sec) break;
        if (i == chain.size() - 1) {
            int lastByte = sec[1];
            if (lastByte >= 2) data.insert(data.end(), sec + 2, sec + lastByte + 1);
        } else {
            data.insert(data.end(), sec + 2, sec + 256);
        }
    }
    return data;
}

bool D90Image::addFile(const std::string& name, CbmFileType type,
                        const std::vector<uint8_t>& data) {
    // Allocate data sectors
    int dataLen = (int)data.size();
    int sectorsNeeded = (dataLen + 253) / 254;
    if (sectorsNeeded == 0) sectorsNeeded = 1;

    std::vector<TrackSector> chain;
    for (int i = 0; i < sectorsNeeded; i++) {
        TrackSector ts = allocateNextFree(39);
        if (ts.isNull()) return false;
        chain.push_back(ts);
    }

    // Write data to sectors
    int offset = 0;
    for (int i = 0; i < sectorsNeeded; i++) {
        uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        std::memset(sec, 0, 256);
        if (i < sectorsNeeded - 1) {
            sec[0] = chain[i + 1].track;
            sec[1] = chain[i + 1].sector;
            std::memcpy(sec + 2, data.data() + offset, std::min(254, dataLen - offset));
            offset += 254;
        } else {
            sec[0] = 0;
            int remaining = dataLen - offset;
            sec[1] = remaining <= 0 ? 1 : (uint8_t)(remaining + 1);
            if (remaining > 0) std::memcpy(sec + 2, data.data() + offset, remaining);
        }
    }

    // Create directory entry
    DirEntry entry;
    std::memset(&entry, 0, sizeof(entry));
    entry.fileType = 0x80 | (uint8_t)type;
    entry.firstDataTS = chain[0];
    padPetsciiName(entry.filename, name);
    entry.sizeInSectors = (uint16_t)sectorsNeeded;
    return !allocDirectoryEntry(entry).isNull();
}

bool D90Image::removeFile(const std::string& name) {
    TrackSector dirTS; int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;
    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;
    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);
    // Zero out file sectors (effectively "frees" them since BAM is chain-based)
    auto chain = getFileChain(e.firstDataTS);
    for (auto& ts : chain) {
        uint8_t* sec = sectorData(ts.track, ts.sector);
        if (sec) std::memset(sec, 0, 256);
    }
    // Clear directory entry
    uint8_t* p = dirSec + entryIdx * 32;
    p[2] = 0;
    return true;
}
