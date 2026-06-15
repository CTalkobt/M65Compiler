#include "D64Image.hpp"
#include <cstring>
#include <algorithm>

// ============================================================================
// D64 track zone layout
// ============================================================================

int D64Image::sectorsForTrack(int track) {
    if (track >= 1 && track <= 17) return 21;
    if (track >= 18 && track <= 24) return 19;
    if (track >= 25 && track <= 30) return 18;
    if (track >= 31 && track <= 35) return 17;
    return 0;
}

int D64Image::sectorsOnTrack(int track) const {
    return sectorsForTrack(track);
}

// ============================================================================
// Sector offset: cumulative sectors before this track
// ============================================================================

int D64Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > TRACKS) return -1;
    int spt = sectorsForTrack(track);
    if (sector < 0 || sector >= spt) return -1;
    int offset = 0;
    for (int t = 1; t < track; t++)
        offset += sectorsForTrack(t);
    return (offset + sector) * 256;
}

// ============================================================================
// BAM layout for D64
// Track 18, sector 0:
//   $00-$01: track/sector of first directory sector (18/1)
//   $02: DOS version ('A')
//   $03: unused ($00)
//   $04-$8F: BAM entries for tracks 1-35 (4 bytes each)
//     Byte 0: free sector count
//     Bytes 1-3: bitmap (up to 21 bits, LSB first, 1=free)
//   $90-$9F: disk name (16 chars, PETSCII, $A0 padded)
//   $A0-$A1: $A0 padding
//   $A2-$A3: disk ID
//   $A4: $A0 padding
//   $A5-$A6: DOS type ("2A")
//   $A7-$AA: $A0 padding
// ============================================================================

bool D64Image::isSectorFree(int track, int sector) const {
    if (track < 1 || track > TRACKS) return false;
    const uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (track - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    return (bam[off + byteIdx] >> bitIdx) & 1;
}

bool D64Image::allocateSector(int track, int sector) {
    if (track < 1 || track > TRACKS) return false;
    uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (track - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    if (!((bam[off + byteIdx] >> bitIdx) & 1)) return false;
    bam[off + byteIdx] &= ~(1 << bitIdx);
    bam[off]--; // decrement free count
    return true;
}

bool D64Image::freeSector(int track, int sector) {
    if (track < 1 || track > TRACKS) return false;
    uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (track - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    if ((bam[off + byteIdx] >> bitIdx) & 1) return false;
    bam[off + byteIdx] |= (1 << bitIdx);
    bam[off]++;
    return true;
}

TrackSector D64Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = DIR_TRACK;
    for (int dist = 0; dist < TRACKS; dist++) {
        for (int dir = 0; dir < 2; dir++) {
            int t = nearTrack + (dir == 0 ? dist : -dist);
            if (t < 1 || t > TRACKS) continue;
            if (t == DIR_TRACK) continue;
            int spt = sectorsForTrack(t);
            for (int s = 0; s < spt; s++) {
                if (isSectorFree(t, s)) {
                    allocateSector(t, s);
                    return {(uint8_t)t, (uint8_t)s};
                }
            }
        }
    }
    return {0, 0};
}

int D64Image::freeSectors() const {
    const uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return 0;
    int count = 0;
    for (int t = 0; t < TRACKS; t++) {
        count += bam[0x04 + t * 4];
    }
    return count;
}

// ============================================================================
// Format
// ============================================================================

void D64Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(IMAGE_SIZE, 0);

    uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    bam[0] = DIR_TRACK;
    bam[1] = DIR_FIRST_SECTOR;
    bam[2] = 0x41; // 'A' — DOS version
    bam[3] = 0x00;

    // Initialize BAM entries — all sectors free
    for (int t = 1; t <= TRACKS; t++) {
        int off = 0x04 + (t - 1) * 4;
        int spt = sectorsForTrack(t);
        if (t == DIR_TRACK) {
            bam[off] = spt - 2; // track 18: BAM + first dir sector used
            // Set all bits, then clear used ones
            bam[off + 1] = 0xFF; bam[off + 2] = 0xFF; bam[off + 3] = 0xFF;
            // Mask unused bits above spt
            if (spt < 24) bam[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam[off + 2] &= (1 << (spt - 8)) - 1;
            if (spt < 8) bam[off + 1] &= (1 << spt) - 1;
            // Allocate sector 0 (BAM) and sector 1 (first dir)
            bam[off + 1] &= ~0x03;
        } else {
            bam[off] = spt;
            bam[off + 1] = 0xFF; bam[off + 2] = 0xFF; bam[off + 3] = 0xFF;
            // Mask unused bits
            if (spt < 24) bam[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam[off + 2] &= (1 << (spt - 8)) - 1;
            if (spt < 8)  bam[off + 1] &= (1 << spt) - 1;
        }
    }

    // Disk name at $90
    padPetsciiName(reinterpret_cast<char*>(bam + 0x90),
                   diskName.empty() ? "CC45 DISK" : diskName);
    bam[0xA0] = 0xA0;
    bam[0xA1] = 0xA0;
    std::string id = diskId.empty() ? "CC" : diskId;
    bam[0xA2] = (uint8_t)id[0];
    bam[0xA3] = (uint8_t)(id.size() > 1 ? id[1] : ' ');
    bam[0xA4] = 0xA0;
    bam[0xA5] = 0x32; // '2'
    bam[0xA6] = 0x41; // 'A'
    bam[0xA7] = 0xA0; bam[0xA8] = 0xA0; bam[0xA9] = 0xA0; bam[0xAA] = 0xA0;

    // First directory sector
    uint8_t* dir = sectorData(DIR_TRACK, DIR_FIRST_SECTOR);
    dir[0] = 0x00;
    dir[1] = 0xFF;
}

// ============================================================================
// File operations (same logic as D81, using common base helpers)
// ============================================================================

std::string D64Image::diskName() const {
    const uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(bam + 0x90), 16));
}

std::string D64Image::diskId() const {
    const uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return "";
    return std::string(1, (char)bam[0xA2]) + (char)bam[0xA3];
}

std::vector<FileInfo> D64Image::listFiles() const {
    std::vector<FileInfo> files;
    const uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return files;

    TrackSector ts = {bam[0], bam[1]};
    while (ts.track != 0) {
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid()) {
                FileInfo fi;
                fi.name = e.name();
                fi.type = e.type();
                fi.sizeInSectors = e.sizeInSectors;
                fi.sizeInBytes = e.sizeInSectors * 254;
                fi.locked = (e.fileType & 0x40) != 0;
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

bool D64Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D64Image::readFile(const std::string& name) const {
    DirEntry e = findFile(name);
    if (!e.isValid()) return {};
    std::vector<uint8_t> data;
    auto chain = getFileChain(e.firstDataTS);
    for (size_t i = 0; i < chain.size(); i++) {
        const uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        if (!sec) break;
        if (i == chain.size() - 1) {
            int lastByte = sec[1];
            if (lastByte >= 2)
                data.insert(data.end(), sec + 2, sec + lastByte + 1);
        } else {
            data.insert(data.end(), sec + 2, sec + 256);
        }
    }
    return data;
}

bool D64Image::addFile(const std::string& name, CbmFileType type,
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
            sec[0] = chain[i + 1].track;
            sec[1] = chain[i + 1].sector;
            int copyLen = std::min(254, dataLen - offset);
            std::memcpy(sec + 2, data.data() + offset, copyLen);
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

bool D64Image::removeFile(const std::string& name) {
    TrackSector dirTS; int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;
    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;
    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);
    auto chain = getFileChain(e.firstDataTS);
    for (auto& ts : chain) freeSector(ts.track, ts.sector);
    e.fileType = 0;
    e.toSector(dirSec, entryIdx);
    return true;
}

bool D64Image::setDiskName(const std::string& name) {
    uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return false;
    padPetsciiName(reinterpret_cast<char*>(bam + 0x90), name);
    return true;
}

bool D64Image::setDiskId(const std::string& id) {
    uint8_t* bam = sectorData(DIR_TRACK, BAM_SECTOR);
    if (!bam) return false;
    bam[0xA2] = id.size() > 0 ? (uint8_t)id[0] : ' ';
    bam[0xA3] = id.size() > 1 ? (uint8_t)id[1] : ' ';
    return true;
}
