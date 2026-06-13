#include "D81Image.hpp"
#include <cstring>
#include <algorithm>

// ============================================================================
// D81 sector layout: linear — (track-1) * 40 + sector
// Tracks are numbered 1-80, sectors 0-39
// ============================================================================

int D81Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > TRACKS || sector < 0 || sector >= SECTORS_PER_TRACK)
        return -1;
    return ((track - 1) * SECTORS_PER_TRACK + sector) * 256;
}

// ============================================================================
// BAM layout for D81
// Track 40, sector 1: BAM for tracks 1-40
// Track 40, sector 2: BAM for tracks 41-80
// Each BAM sector:
//   $00-$01: link to next BAM sector (or 0,FF for last)
//   $02: DOS version ('D')
//   $03: complement of version ($BB)
//   $04-$05: disk ID
//   $06: I/O byte
//   $07: auto-boot flag
//   $10-$FF: 40 tracks × 6 bytes each = 240 bytes
//     Byte 0: free sector count for this track
//     Bytes 1-5: 40 bits (5 bytes), 1 = free, 0 = allocated
// ============================================================================

void D81Image::bamBitPos(int track, int sector, int& bamSector, int& byteOff, int& bitOff) const {
    if (track >= 1 && track <= 40) {
        bamSector = BAM_SECTOR_1;
        int t = track - 1; // 0-based within this BAM sector
        byteOff = 0x10 + t * 6 + 1 + (sector / 8);
        bitOff = sector % 8;
    } else {
        bamSector = BAM_SECTOR_2;
        int t = track - 41; // 0-based within this BAM sector
        byteOff = 0x10 + t * 6 + 1 + (sector / 8);
        bitOff = sector % 8;
    }
}

bool D81Image::isSectorFree(int track, int sector) const {
    int bs, bo, bi;
    bamBitPos(track, sector, bs, bo, bi);
    const uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    return (bam[bo] >> bi) & 1;
}

bool D81Image::allocateSector(int track, int sector) {
    int bs, bo, bi;
    bamBitPos(track, sector, bs, bo, bi);
    uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    if (!((bam[bo] >> bi) & 1)) return false; // already allocated
    bam[bo] &= ~(1 << bi); // clear bit = allocated
    // Decrement free count
    int t = (bs == BAM_SECTOR_1) ? (track - 1) : (track - 41);
    bam[0x10 + t * 6]--;
    return true;
}

bool D81Image::freeSector(int track, int sector) {
    int bs, bo, bi;
    bamBitPos(track, sector, bs, bo, bi);
    uint8_t* bam = sectorData(DIR_TRACK, bs);
    if (!bam) return false;
    if ((bam[bo] >> bi) & 1) return false; // already free
    bam[bo] |= (1 << bi); // set bit = free
    int t = (bs == BAM_SECTOR_1) ? (track - 1) : (track - 41);
    bam[0x10 + t * 6]++;
    return true;
}

TrackSector D81Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = DIR_TRACK;
    // Search outward from nearTrack, alternating above/below
    for (int dist = 0; dist < TRACKS; dist++) {
        for (int dir = 0; dir < 2; dir++) {
            int t = nearTrack + (dir == 0 ? dist : -dist);
            if (t < 1 || t > TRACKS) continue;
            if (t == DIR_TRACK) continue; // skip directory track
            for (int s = 0; s < SECTORS_PER_TRACK; s++) {
                if (isSectorFree(t, s)) {
                    allocateSector(t, s);
                    return {(uint8_t)t, (uint8_t)s};
                }
            }
        }
    }
    return {0, 0}; // disk full
}

int D81Image::freeSectors() const {
    int count = 0;
    for (int bs = BAM_SECTOR_1; bs <= BAM_SECTOR_2; bs++) {
        const uint8_t* bam = sectorData(DIR_TRACK, bs);
        if (!bam) continue;
        int tracks = (bs == BAM_SECTOR_1) ? 40 : 40;
        for (int t = 0; t < tracks; t++) {
            count += bam[0x10 + t * 6]; // free count byte
        }
    }
    return count;
}

// ============================================================================
// Format a new D81 image
// ============================================================================

void D81Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(IMAGE_SIZE, 0);

    // --- Header sector (track 40, sector 0) ---
    uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    hdr[0] = DIR_TRACK; // link to directory: track 40
    hdr[1] = DIR_FIRST_SECTOR; // sector 3
    hdr[2] = 0x44; // 'D' — DOS version
    hdr[3] = 0x00; // format version
    padPetsciiName(reinterpret_cast<char*>(hdr + 4), diskName.empty() ? "CC45 DISK" : diskName);
    hdr[0x14] = 0xA0; // padding
    hdr[0x15] = 0xA0;
    std::string id = diskId.empty() ? "CC" : diskId;
    hdr[0x16] = (uint8_t)id[0];
    hdr[0x17] = (uint8_t)(id.size() > 1 ? id[1] : ' ');
    hdr[0x18] = 0xA0;
    hdr[0x19] = 0x33; // '3' — DOS type
    hdr[0x1A] = 0x44; // 'D'
    hdr[0x1B] = 0xA0;
    hdr[0x1C] = 0xA0;

    // --- BAM sectors (track 40, sectors 1-2) ---
    for (int bs = BAM_SECTOR_1; bs <= BAM_SECTOR_2; bs++) {
        uint8_t* bam = sectorData(DIR_TRACK, bs);
        if (bs == BAM_SECTOR_1) {
            bam[0] = DIR_TRACK;
            bam[1] = BAM_SECTOR_2;
        } else {
            bam[0] = 0x00;
            bam[1] = 0xFF;
        }
        bam[2] = 0x44; // 'D'
        bam[3] = 0xBB; // complement
        bam[4] = hdr[0x16]; // disk ID
        bam[5] = hdr[0x17];
        bam[6] = 0xC0; // I/O byte
        bam[7] = 0x00; // auto-boot flag

        // Mark all sectors as free
        int startTrack = (bs == BAM_SECTOR_1) ? 1 : 41;
        for (int t = 0; t < 40; t++) {
            int absTrack = startTrack + t;
            int off = 0x10 + t * 6;
            if (absTrack == DIR_TRACK) {
                // Directory track: mark header + BAM + first dir sector as used
                bam[off] = SECTORS_PER_TRACK - 4; // 36 free
                std::memset(bam + off + 1, 0xFF, 5);
                // Allocate sectors 0,1,2,3
                bam[off + 1] &= ~0x0F; // clear bits 0-3
            } else {
                bam[off] = SECTORS_PER_TRACK; // 40 free
                std::memset(bam + off + 1, 0xFF, 5);
            }
        }
    }

    // --- First directory sector (track 40, sector 3) ---
    uint8_t* dir = sectorData(DIR_TRACK, DIR_FIRST_SECTOR);
    dir[0] = 0x00; // no next directory sector
    dir[1] = 0xFF; // last sector marker
}

// ============================================================================
// File operations
// ============================================================================

std::string D81Image::diskName() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 4), 16));
}

std::string D81Image::diskId() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, HEADER_SECTOR);
    if (!hdr) return "";
    return std::string(1, (char)hdr[0x16]) + (char)hdr[0x17];
}

std::vector<FileInfo> D81Image::listFiles() const {
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
                fi.name = e.name();
                fi.type = e.type();
                fi.sizeInSectors = e.sizeInSectors;
                fi.sizeInBytes = e.sizeInSectors * 254; // approximate
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

bool D81Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D81Image::readFile(const std::string& name) const {
    DirEntry e = findFile(name);
    if (!e.isValid()) return {};

    std::vector<uint8_t> data;
    auto chain = getFileChain(e.firstDataTS);
    for (size_t i = 0; i < chain.size(); i++) {
        const uint8_t* sec = sectorData(chain[i].track, chain[i].sector);
        if (!sec) break;
        if (i == chain.size() - 1) {
            // Last sector: byte 1 = index of last data byte
            int lastByte = sec[1];
            if (lastByte >= 2)
                data.insert(data.end(), sec + 2, sec + lastByte + 1);
        } else {
            data.insert(data.end(), sec + 2, sec + 256);
        }
    }
    return data;
}

bool D81Image::addFile(const std::string& name, CbmFileType type,
                       const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false; // duplicate

    // Calculate sectors needed
    int dataLen = (int)data.size();
    int sectorsNeeded = (dataLen + 253) / 254; // 254 data bytes per sector
    if (sectorsNeeded == 0) sectorsNeeded = 1; // empty file still needs 1 sector

    if (sectorsNeeded > freeSectors()) return false; // disk full

    // Allocate all data sectors
    std::vector<TrackSector> chain;
    for (int i = 0; i < sectorsNeeded; i++) {
        TrackSector ts = allocateNextFree(DIR_TRACK);
        if (ts.isNull()) return false;
        chain.push_back(ts);
    }

    // Write data into sectors
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
            sec[0] = 0; // last sector
            int remaining = dataLen - offset;
            if (remaining <= 0) {
                sec[1] = 1; // empty last sector: last byte at position 1
            } else {
                sec[1] = (uint8_t)(remaining + 1); // index of last data byte
                std::memcpy(sec + 2, data.data() + offset, remaining);
            }
        }
    }

    // Create directory entry
    DirEntry entry;
    entry.fileType = 0x80 | (uint8_t)type; // closed flag + type
    entry.firstDataTS = chain[0];
    padPetsciiName(entry.filename, name);
    entry.sizeInSectors = (uint16_t)sectorsNeeded;

    TrackSector dirTS = allocDirectoryEntry(entry);
    return !dirTS.isNull();
}

bool D81Image::removeFile(const std::string& name) {
    TrackSector dirTS;
    int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;

    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;

    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);

    // Free all data sectors
    auto chain = getFileChain(e.firstDataTS);
    for (auto& ts : chain) {
        freeSector(ts.track, ts.sector);
    }

    // Clear directory entry (set type to DEL)
    e.fileType = 0;
    e.toSector(dirSec, entryIdx);
    return true;
}
