#include "D65Image.hpp"
#include <cstring>
#include <algorithm>

// D65: uniform 40 sectors/track, linear layout like D81 but 162 tracks

int D65Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > TRACKS || sector < 0 || sector >= SECTORS_PER_TRACK)
        return -1;
    return ((track - 1) * SECTORS_PER_TRACK + sector) * 256;
}

// ============================================================================
// BAM layout: 4 BAM sectors covering 4 groups of ~41 tracks each
// Same 6-bytes-per-track format as D81
// ============================================================================

void D65Image::bamLocation(int track, int& bamSector, int& entryOffset) const {
    if (track >= 1 && track <= 40) {
        bamSector = BAM_S0_SEC1;
        entryOffset = 0x10 + (track - 1) * 6;
    } else if (track >= 41 && track <= 81) {
        bamSector = BAM_S0_SEC2;
        entryOffset = 0x10 + (track - 41) * 6;
    } else if (track >= 82 && track <= 122) {
        bamSector = BAM_S1_SEC1;
        entryOffset = 0x10 + (track - 82) * 6;
    } else {
        bamSector = BAM_S1_SEC2;
        entryOffset = 0x10 + (track - 123) * 6;
    }
}

bool D65Image::isSectorFree(int track, int sector) const {
    int bs, off;
    bamLocation(track, bs, off);
    const uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    int byteIdx = off + 1 + (sector / 8);
    int bitIdx = sector % 8;
    return (bam[byteIdx] >> bitIdx) & 1;
}

bool D65Image::allocateSector(int track, int sector) {
    int bs, off;
    bamLocation(track, bs, off);
    uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    int byteIdx = off + 1 + (sector / 8);
    int bitIdx = sector % 8;
    if (!((bam[byteIdx] >> bitIdx) & 1)) return false;
    bam[byteIdx] &= ~(1 << bitIdx);
    bam[off]--;
    return true;
}

bool D65Image::freeSector(int track, int sector) {
    int bs, off;
    bamLocation(track, bs, off);
    uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    int byteIdx = off + 1 + (sector / 8);
    int bitIdx = sector % 8;
    if ((bam[byteIdx] >> bitIdx) & 1) return false;
    bam[byteIdx] |= (1 << bitIdx);
    bam[off]++;
    return true;
}

TrackSector D65Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = DIR_TRACK;
    for (int dist = 0; dist < TRACKS; dist++) {
        for (int dir = 0; dir < 2; dir++) {
            int t = nearTrack + (dir == 0 ? dist : -dist);
            if (t < 1 || t > TRACKS) continue;
            if (t == DIR_TRACK) continue;
            for (int s = 0; s < SECTORS_PER_TRACK; s++) {
                if (isSectorFree(t, s)) {
                    allocateSector(t, s);
                    return {(uint8_t)t, (uint8_t)s};
                }
            }
        }
    }
    return {0, 0};
}

int D65Image::freeSectors() const {
    int count = 0;
    int bamSectors[] = {BAM_S0_SEC1, BAM_S0_SEC2, BAM_S1_SEC1, BAM_S1_SEC2};
    int trackCounts[] = {40, 41, 41, 40}; // tracks per BAM sector
    for (int b = 0; b < 4; b++) {
        const uint8_t* bam = sectorData(DIR_TRACK, bamSectors[b]);
        if (!bam) continue;
        for (int t = 0; t < trackCounts[b]; t++) {
            count += bam[0x10 + t * 6];
        }
    }
    return count;
}

// ============================================================================
// Format
// ============================================================================

void D65Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(IMAGE_SIZE, 0);

    // Header (track 40, sector 0)
    uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    hdr[0] = DIR_TRACK;
    hdr[1] = DIR_FIRST_SECTOR;
    hdr[2] = 0x44; // 'D'
    hdr[3] = 0x00;
    padPetsciiName(reinterpret_cast<char*>(hdr + 4),
                   diskName.empty() ? "CC45 DISK" : diskName);
    hdr[0x14] = 0xA0; hdr[0x15] = 0xA0;
    std::string id = diskId.empty() ? "CC" : diskId;
    hdr[0x16] = (uint8_t)id[0];
    hdr[0x17] = (uint8_t)(id.size() > 1 ? id[1] : ' ');
    hdr[0x18] = 0xA0;
    hdr[0x19] = 0x33; hdr[0x1A] = 0x44; // "3D"
    hdr[0x1B] = 0xA0; hdr[0x1C] = 0xA0;

    // BAM sectors
    struct BamInit { int sector; int startTrack; int numTracks; int linkTrack; int linkSector; };
    BamInit bams[] = {
        {BAM_S0_SEC1, 1,   40, DIR_TRACK, BAM_S0_SEC2},
        {BAM_S0_SEC2, 41,  41, DIR_TRACK, BAM_S1_SEC1},
        {BAM_S1_SEC1, 82,  41, DIR_TRACK, BAM_S1_SEC2},
        {BAM_S1_SEC2, 123, 40, 0,         0xFF},
    };

    for (auto& bi : bams) {
        uint8_t* bam = sectorData(DIR_TRACK, bi.sector);
        bam[0] = bi.linkTrack;
        bam[1] = bi.linkSector;
        bam[2] = 0x44; bam[3] = 0xBB;
        bam[4] = hdr[0x16]; bam[5] = hdr[0x17];
        bam[6] = 0xC0; bam[7] = 0x00;

        for (int t = 0; t < bi.numTracks; t++) {
            int absTrack = bi.startTrack + t;
            int off = 0x10 + t * 6;
            if (absTrack == DIR_TRACK) {
                // Header + 4 BAM sectors + first dir sector = 6 used
                bam[off] = SECTORS_PER_TRACK - 6;
                std::memset(bam + off + 1, 0xFF, 5);
                bam[off + 1] &= ~0x3F; // clear bits 0-5
            } else {
                bam[off] = SECTORS_PER_TRACK;
                std::memset(bam + off + 1, 0xFF, 5);
            }
        }
    }

    // First directory sector (track 40, sector 5)
    uint8_t* dir = sectorData(DIR_TRACK, DIR_FIRST_SECTOR);
    dir[0] = 0x00; dir[1] = 0xFF;
}

// ============================================================================
// File operations
// ============================================================================

std::string D65Image::diskName() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 4), 16));
}

std::string D65Image::diskId() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    if (!hdr) return "";
    return std::string(1, (char)hdr[0x16]) + (char)hdr[0x17];
}

std::vector<FileInfo> D65Image::listFiles() const {
    std::vector<FileInfo> files;
    const uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
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
                fi.sizeInSectors = e.sizeInSectors; fi.sizeInBytes = e.sizeInSectors * 254;
                fi.locked = (e.fileType & 0x40) != 0; fi.closed = e.isClosed();
                files.push_back(fi);
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return files;
}

bool D65Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D65Image::readFile(const std::string& name) const {
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

bool D65Image::addFile(const std::string& name, CbmFileType type,
                       const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false;
    int dataLen = (int)data.size();
    int sectorsNeeded = (dataLen + 253) / 254;
    if (sectorsNeeded == 0) sectorsNeeded = 1;
    if (sectorsNeeded > freeSectors()) return false;
    std::vector<TrackSector> chain;
    for (int i = 0; i < sectorsNeeded; i++) {
        TrackSector ts = allocateNextFree(DIR_TRACK);
        if (ts.isNull()) return false;
        chain.push_back(ts);
    }
    int offset = 0;
    for (int i = 0; i < sectorsNeeded; i++) {
        uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        std::memset(sec, 0, 256);
        if (i < sectorsNeeded - 1) {
            sec[0] = chain[i + 1].track; sec[1] = chain[i + 1].sector;
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
    entry.fileType = 0x80 | (uint8_t)type;
    entry.firstDataTS = chain[0];
    padPetsciiName(entry.filename, name);
    entry.sizeInSectors = (uint16_t)sectorsNeeded;
    return !allocDirectoryEntry(entry).isNull();
}

bool D65Image::removeFile(const std::string& name) {
    TrackSector dirTS; int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;
    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;
    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);
    for (auto& ts : getFileChain(e.firstDataTS)) freeSector(ts.track, ts.sector);
    e.fileType = 0;
    e.toSector(dirSec, entryIdx);
    return true;
}
