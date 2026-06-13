#include "D71Image.hpp"
#include <cstring>
#include <algorithm>

// D71 uses same zone layout as D64, repeated for both sides.
// Tracks 1-35 = side 0, tracks 36-70 = side 1 (same zone pattern).

int D71Image::sectorsForTrack(int track) {
    // Normalize to side-relative track (1-35)
    int t = track;
    if (t > 35) t -= 35;
    if (t >= 1 && t <= 17) return 21;
    if (t >= 18 && t <= 24) return 19;
    if (t >= 25 && t <= 30) return 18;
    if (t >= 31 && t <= 35) return 17;
    return 0;
}

int D71Image::sectorsOnTrack(int track) const {
    return sectorsForTrack(track);
}

int D71Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > TRACKS) return -1;
    int spt = sectorsForTrack(track);
    if (sector < 0 || sector >= spt) return -1;
    int offset = 0;
    for (int t = 1; t < track; t++)
        offset += sectorsForTrack(t);
    return (offset + sector) * 256;
}

// ============================================================================
// BAM: Side 0 at track 18 sector 0 (D64 format), side 1 at track 53 sector 0
// Side 0 BAM: $04-$8F = tracks 1-35 (4 bytes each, same as D64)
// Side 1 BAM: $00-$01 = link, $04-$8F = tracks 36-70 (4 bytes each)
// Side 0 BAM also has free counts for side 1 at $DD-$FF (1 byte per track)
// ============================================================================

bool D71Image::isSectorFree(int track, int sector) const {
    int bamTrack = (track <= 35) ? BAM_TRACK_SIDE0 : BAM_TRACK_SIDE1;
    int relTrack = (track <= 35) ? track : (track - 35);
    const uint8_t* bam = sectorData(bamTrack, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (relTrack - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    return (bam[off + byteIdx] >> bitIdx) & 1;
}

bool D71Image::allocateSector(int track, int sector) {
    int bamTrack = (track <= 35) ? BAM_TRACK_SIDE0 : BAM_TRACK_SIDE1;
    int relTrack = (track <= 35) ? track : (track - 35);
    uint8_t* bam = sectorData(bamTrack, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (relTrack - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    if (!((bam[off + byteIdx] >> bitIdx) & 1)) return false;
    bam[off + byteIdx] &= ~(1 << bitIdx);
    bam[off]--;
    return true;
}

bool D71Image::freeSector(int track, int sector) {
    int bamTrack = (track <= 35) ? BAM_TRACK_SIDE0 : BAM_TRACK_SIDE1;
    int relTrack = (track <= 35) ? track : (track - 35);
    uint8_t* bam = sectorData(bamTrack, BAM_SECTOR);
    if (!bam) return false;
    int off = 0x04 + (relTrack - 1) * 4;
    int byteIdx = 1 + (sector / 8);
    int bitIdx = sector % 8;
    if ((bam[off + byteIdx] >> bitIdx) & 1) return false;
    bam[off + byteIdx] |= (1 << bitIdx);
    bam[off]++;
    return true;
}

TrackSector D71Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = DIR_TRACK;
    for (int dist = 0; dist < TRACKS; dist++) {
        for (int dir = 0; dir < 2; dir++) {
            int t = nearTrack + (dir == 0 ? dist : -dist);
            if (t < 1 || t > TRACKS) continue;
            if (t == DIR_TRACK || t == BAM_TRACK_SIDE1) continue;
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

int D71Image::freeSectors() const {
    int count = 0;
    for (int side = 0; side < 2; side++) {
        int bamTrack = (side == 0) ? BAM_TRACK_SIDE0 : BAM_TRACK_SIDE1;
        const uint8_t* bam = sectorData(bamTrack, BAM_SECTOR);
        if (!bam) continue;
        for (int t = 0; t < 35; t++) {
            count += bam[0x04 + t * 4];
        }
    }
    return count;
}

// ============================================================================
// Format
// ============================================================================

void D71Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(IMAGE_SIZE, 0);

    // Side 0 BAM (track 18, sector 0) — same layout as D64
    uint8_t* bam0 = sectorData(BAM_TRACK_SIDE0, BAM_SECTOR);
    bam0[0] = DIR_TRACK;
    bam0[1] = DIR_FIRST_SECTOR;
    bam0[2] = 0x41; // 'A'
    bam0[3] = 0x80; // double-sided flag

    for (int t = 1; t <= 35; t++) {
        int off = 0x04 + (t - 1) * 4;
        int spt = sectorsForTrack(t);
        if (t == DIR_TRACK) {
            bam0[off] = spt - 2;
            bam0[off + 1] = 0xFF; bam0[off + 2] = 0xFF; bam0[off + 3] = 0xFF;
            if (spt < 24) bam0[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam0[off + 2] &= (1 << (spt - 8)) - 1;
            bam0[off + 1] &= ~0x03; // allocate sectors 0, 1
        } else {
            bam0[off] = spt;
            bam0[off + 1] = 0xFF; bam0[off + 2] = 0xFF; bam0[off + 3] = 0xFF;
            if (spt < 24) bam0[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam0[off + 2] &= (1 << (spt - 8)) - 1;
        }
    }

    padPetsciiName(reinterpret_cast<char*>(bam0 + 0x90),
                   diskName.empty() ? "CC45 DISK" : diskName);
    bam0[0xA0] = 0xA0; bam0[0xA1] = 0xA0;
    std::string id = diskId.empty() ? "CC" : diskId;
    bam0[0xA2] = (uint8_t)id[0];
    bam0[0xA3] = (uint8_t)(id.size() > 1 ? id[1] : ' ');
    bam0[0xA4] = 0xA0;
    bam0[0xA5] = 0x32; bam0[0xA6] = 0x41; // "2A"
    bam0[0xA7] = 0xA0; bam0[0xA8] = 0xA0; bam0[0xA9] = 0xA0; bam0[0xAA] = 0xA0;

    // Side 1 BAM (track 53, sector 0) — same structure
    uint8_t* bam1 = sectorData(BAM_TRACK_SIDE1, BAM_SECTOR);
    bam1[0] = 0x00; bam1[1] = 0xFF;
    bam1[2] = 0x41; bam1[3] = 0x80;

    for (int t = 1; t <= 35; t++) {
        int off = 0x04 + (t - 1) * 4;
        int spt = sectorsForTrack(t + 35); // side 1 tracks
        if (t + 35 == BAM_TRACK_SIDE1) {
            bam1[off] = spt - 1; // BAM sector itself used
            bam1[off + 1] = 0xFF; bam1[off + 2] = 0xFF; bam1[off + 3] = 0xFF;
            if (spt < 24) bam1[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam1[off + 2] &= (1 << (spt - 8)) - 1;
            bam1[off + 1] &= ~0x01; // allocate sector 0
        } else {
            bam1[off] = spt;
            bam1[off + 1] = 0xFF; bam1[off + 2] = 0xFF; bam1[off + 3] = 0xFF;
            if (spt < 24) bam1[off + 3] &= (1 << (spt - 16)) - 1;
            if (spt < 16) bam1[off + 2] &= (1 << (spt - 8)) - 1;
        }
    }

    // First directory sector
    uint8_t* dir = sectorData(DIR_TRACK, DIR_FIRST_SECTOR);
    dir[0] = 0x00; dir[1] = 0xFF;
}

// ============================================================================
// File operations — delegates to common base helpers
// ============================================================================

std::string D71Image::diskName() const {
    const uint8_t* bam = sectorData(BAM_TRACK_SIDE0, BAM_SECTOR);
    if (!bam) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(bam + 0x90), 16));
}

std::string D71Image::diskId() const {
    const uint8_t* bam = sectorData(BAM_TRACK_SIDE0, BAM_SECTOR);
    if (!bam) return "";
    return std::string(1, (char)bam[0xA2]) + (char)bam[0xA3];
}

std::vector<FileInfo> D71Image::listFiles() const {
    std::vector<FileInfo> files;
    const uint8_t* bam = sectorData(BAM_TRACK_SIDE0, BAM_SECTOR);
    if (!bam) return files;
    TrackSector ts = {bam[0], bam[1]};
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

bool D71Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D71Image::readFile(const std::string& name) const {
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

bool D71Image::addFile(const std::string& name, CbmFileType type,
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

bool D71Image::removeFile(const std::string& name) {
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
