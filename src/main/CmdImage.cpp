#include "CmdImage.hpp"
#include <cstring>
#include <algorithm>
#include <set>

int CmdImage::totalTracks() const {
    switch (fmt_) {
        case FD_1M: return 81;
        case FD_2M: return 81;
        case FD_4M: return 81;
        case NATIVE: {
            int t = (int)(image_.size() / (256 * sectorsOnTrack(1)));
            return t > 0 ? t : 255;
        }
    }
    return 81;
}

int CmdImage::sectorsOnTrack(int) const {
    switch (fmt_) {
        case FD_1M: return 10;
        case FD_2M: return 20;
        case FD_4M: return 40;
        case NATIVE: return 256;
    }
    return 20;
}

int CmdImage::totalSectors() const {
    return totalTracks() * sectorsOnTrack(1);
}

int CmdImage::sectorOffset(int track, int sector) const {
    if (track < 1 || track > totalTracks()) return -1;
    int spt = sectorsOnTrack(track);
    if (sector < 0 || sector >= spt) return -1;
    return ((track - 1) * spt + sector) * 256;
}

// Build used-sector set by walking all file chains + directory
static std::set<uint64_t> buildUsedSet(const DiskImage* img, int dirTrack, int dirSector) {
    std::set<uint64_t> used;
    auto key = [](int t, int s) -> uint64_t { return ((uint64_t)t << 16) | s; };

    used.insert(key(dirTrack, 0)); // header
    TrackSector ts = {(uint8_t)dirTrack, (uint8_t)dirSector};
    while (ts.track != 0) {
        used.insert(key(ts.track, ts.sector));
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
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

bool CmdImage::isSectorFree(int track, int sector) const {
    auto used = buildUsedSet(this, 1, 3);
    return used.find(((uint64_t)track << 16) | sector) == used.end();
}

bool CmdImage::allocateSector(int, int) { return true; }
bool CmdImage::freeSector(int, int) { return true; }

TrackSector CmdImage::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = 1;
    auto used = buildUsedSet(this, 1, 3);
    auto key = [](int t, int s) -> uint64_t { return ((uint64_t)t << 16) | s; };
    int maxT = totalTracks();
    int spt = sectorsOnTrack(1);
    for (int delta = 1; delta <= maxT; delta++) {
        for (int dir = 1; dir >= -1; dir -= 2) {
            int t = nearTrack + delta * dir;
            if (t < 1 || t > maxT) continue;
            for (int s = 0; s < spt; s++) {
                if (used.find(key(t, s)) == used.end())
                    return {(uint8_t)t, (uint8_t)s};
            }
        }
    }
    return {0, 0};
}

int CmdImage::freeSectors() const {
    auto used = buildUsedSet(this, 1, 3);
    return totalSectors() - (int)used.size();
}

void CmdImage::format(const std::string& diskName, const std::string& diskId) {
    int spt = sectorsOnTrack(1);
    int tracks = (fmt_ == NATIVE) ? 255 : 81;
    image_.assign(tracks * spt * 256, 0);

    uint8_t* hdr = sectorData(1, 0);
    if (hdr) {
        hdr[0] = 1; hdr[1] = 3;
        hdr[2] = 'H';
        std::memset(hdr + 0x04, 0xA0, 16);
        std::string name = diskName.empty() ? "CMD DISK" : diskName;
        std::string pet = asciiToPetscii(name);
        std::memcpy(hdr + 0x04, pet.data(), std::min((int)pet.size(), 16));
        hdr[0x16] = diskId.size() > 0 ? asciiToPetscii(diskId)[0] : 'C';
        hdr[0x17] = diskId.size() > 1 ? asciiToPetscii(diskId)[1] : 'M';
    }
    uint8_t* dir = sectorData(1, 3);
    if (dir) { dir[0] = 0; dir[1] = 0xFF; }
}

std::string CmdImage::diskName() const {
    const uint8_t* hdr = sectorData(1, 0);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x04), 16));
}

std::string CmdImage::diskId() const {
    const uint8_t* hdr = sectorData(1, 0);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x16), 2));
}

std::vector<FileInfo> CmdImage::listFiles() const {
    std::vector<FileInfo> files;
    const uint8_t* hdr = sectorData(1, 0);
    if (!hdr) return files;
    TrackSector ts = {hdr[0], hdr[1]};
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

bool CmdImage::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> CmdImage::readFile(const std::string& name) const {
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

bool CmdImage::addFile(const std::string& name, CbmFileType type,
                        const std::vector<uint8_t>& data) {
    int dataLen = (int)data.size();
    int sectorsNeeded = (dataLen + 253) / 254;
    if (sectorsNeeded == 0) sectorsNeeded = 1;

    std::vector<TrackSector> chain;
    for (int i = 0; i < sectorsNeeded; i++) {
        TrackSector ts = allocateNextFree(1);
        if (ts.isNull()) return false;
        chain.push_back(ts);
    }

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

    DirEntry entry;
    std::memset(&entry, 0, sizeof(entry));
    entry.fileType = 0x80 | (uint8_t)type;
    entry.firstDataTS = chain[0];
    padPetsciiName(entry.filename, name);
    entry.sizeInSectors = (uint16_t)sectorsNeeded;
    return !allocDirectoryEntry(entry).isNull();
}

bool CmdImage::removeFile(const std::string& name) {
    TrackSector dirTS; int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;
    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;
    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);
    auto chain = getFileChain(e.firstDataTS);
    for (auto& ts : chain) {
        uint8_t* sec = sectorData(ts.track, ts.sector);
        if (sec) std::memset(sec, 0, 256);
    }
    uint8_t* p = dirSec + entryIdx * 32;
    p[2] = 0;
    return true;
}
