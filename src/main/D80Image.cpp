#include "D80Image.hpp"
#include <cstring>
#include <algorithm>

// Sectors per track for 8050/8250 format
// Tracks 1-39: 29 sectors, 40-53: 27, 54-64: 25, 65-77: 23
// Side 2 (D82 tracks 78-154) mirrors side 1
int D80Image::sectorsForTrack8050(int track) {
    // Normalize for double-sided (tracks 78-154 mirror 1-77)
    int t = track;
    if (t > 77) t -= 77;
    if (t >= 1 && t <= 39) return 29;
    if (t >= 40 && t <= 53) return 27;
    if (t >= 54 && t <= 64) return 25;
    if (t >= 65 && t <= 77) return 23;
    return 0;
}

int D80Image::sectorsOnTrack(int track) const {
    return sectorsForTrack8050(track);
}

int D80Image::totalSectors() const {
    int total = 0;
    for (int t = 1; t <= tracks_; t++)
        total += sectorsOnTrack(t);
    return total;
}

int D80Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > tracks_) return -1;
    if (sector < 0 || sector >= sectorsOnTrack(track)) return -1;

    int offset = 0;
    for (int t = 1; t < track; t++)
        offset += sectorsOnTrack(t) * 256;
    offset += sector * 256;
    return offset;
}

// ============================================================================
// BAM
// ============================================================================

// BAM is on track 38, sectors 0 and 3
// Sector 0: tracks 1-50, Sector 3: tracks 51-77 (and 78-154 for D82)
int D80Image::bamSectorForTrack(int track) const {
    int t = track;
    if (t > 77) t -= 77; // D82 side 2
    if (t <= 50) return 0;
    return 3;
}

int D80Image::bamOffsetForTrack(int track) const {
    int t = track;
    bool side2 = (t > 77);
    if (side2) t -= 77;

    int bamSec = bamSectorForTrack(track);
    int baseTrack = (bamSec == 0) ? 1 : 51;
    int entryIdx = t - baseTrack;

    // Each BAM entry: 5 bytes (1 free count + 4 allocation bitmap)
    // Entries start at offset $06 in the BAM sector
    int bamOff = sectorOffset(BAM_TRACK + (side2 ? 77 : 0), bamSec);
    if (bamOff < 0) return -1;
    return bamOff + 0x06 + entryIdx * 5;
}

bool D80Image::isSectorFree(int track, int sector) const {
    int off = bamOffsetForTrack(track);
    if (off < 0 || off + 5 > (int)image_.size()) return false;
    int byteIdx = sector / 8;
    int bitIdx = sector % 8;
    return (image_[off + 1 + byteIdx] >> bitIdx) & 1;
}

bool D80Image::allocateSector(int track, int sector) {
    int off = bamOffsetForTrack(track);
    if (off < 0 || off + 5 > (int)image_.size()) return false;
    int byteIdx = sector / 8;
    int bitIdx = sector % 8;
    if (!((image_[off + 1 + byteIdx] >> bitIdx) & 1)) return false; // already used
    image_[off + 1 + byteIdx] &= ~(1 << bitIdx);
    image_[off]--; // decrement free count
    return true;
}

bool D80Image::freeSector(int track, int sector) {
    int off = bamOffsetForTrack(track);
    if (off < 0 || off + 5 > (int)image_.size()) return false;
    int byteIdx = sector / 8;
    int bitIdx = sector % 8;
    image_[off + 1 + byteIdx] |= (1 << bitIdx);
    image_[off]++;
    return true;
}

TrackSector D80Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1) nearTrack = DIR_TRACK;
    // Search outward from nearTrack
    for (int delta = 0; delta <= tracks_; delta++) {
        for (int dir = -1; dir <= 1; dir += 2) {
            int t = nearTrack + delta * dir;
            if (t < 1 || t > tracks_) continue;
            if (t == BAM_TRACK) continue; // skip BAM track
            int ns = sectorsOnTrack(t);
            for (int s = 0; s < ns; s++) {
                if (isSectorFree(t, s)) {
                    allocateSector(t, s);
                    return {(uint8_t)t, (uint8_t)s};
                }
            }
        }
    }
    return {0, 0};
}

int D80Image::freeSectors() const {
    int free = 0;
    for (int t = 1; t <= tracks_; t++) {
        if (t == BAM_TRACK) continue;
        int ns = sectorsOnTrack(t);
        for (int s = 0; s < ns; s++) {
            if (isSectorFree(t, s)) free++;
        }
    }
    return free;
}

// ============================================================================
// Format
// ============================================================================

void D80Image::format(const std::string& diskName, const std::string& diskId) {
    int total = totalSectors();
    image_.assign(total * 256, 0);

    // Initialize BAM sectors (track 38, sectors 0 and 3)
    auto initBam = [&](int bamSector, int firstTrack, int lastTrack) {
        uint8_t* bam = sectorData(BAM_TRACK, bamSector);
        if (!bam) return;
        std::memset(bam, 0, 256);
        // Link to next BAM sector (or 00/FF for last)
        if (bamSector == 0) {
            bam[0] = BAM_TRACK; bam[1] = 3; // next BAM at 38/3
        } else {
            bam[0] = 0; bam[1] = 0xFF; // last BAM
        }
        bam[2] = 'C'; // DOS version
        bam[4] = firstTrack;
        bam[5] = lastTrack + 1;

        // Initialize free sectors for each track
        for (int t = firstTrack; t <= lastTrack; t++) {
            int ns = sectorsOnTrack(t);
            int entryOff = 0x06 + (t - firstTrack) * 5;
            bam[entryOff] = ns; // free count
            // Set all sector bits to 1 (free)
            for (int s = 0; s < ns; s++) {
                bam[entryOff + 1 + s / 8] |= (1 << (s % 8));
            }
        }
    };

    initBam(0, 1, 50);
    initBam(3, 51, (tracks_ > 77) ? 77 : tracks_);

    // For D82, initialize side 2 BAM
    if (tracks_ > 77) {
        // Side 2 BAM would be at track 115 (38+77), but standard D82 uses
        // additional BAM sectors on track 38: sectors 6 and 9
        // For simplicity, we follow the standard layout
    }

    // Mark BAM track sectors as used
    allocateSector(BAM_TRACK, 0);
    allocateSector(BAM_TRACK, 3);

    // Initialize directory header (track 39, sector 0)
    uint8_t* hdr = sectorData(DIR_TRACK, 0);
    if (hdr) {
        std::memset(hdr, 0, 256);
        hdr[0] = DIR_TRACK; // first dir sector track
        hdr[1] = 1;         // first dir sector
        hdr[2] = 'C';       // DOS version
        // Disk name at offset $06, padded with $A0
        std::memset(hdr + 0x06, 0xA0, 16);
        std::string name = diskName.empty() ? "CBM DISK" : diskName;
        std::string petName = asciiToPetscii(name);
        int nameLen = std::min((int)petName.size(), 16);
        std::memcpy(hdr + 0x06, petName.data(), nameLen);
        // Disk ID at offset $18
        hdr[0x16] = 0xA0;
        hdr[0x17] = 0xA0;
        std::string id = diskId.empty() ? "CC" : diskId;
        std::string petId = asciiToPetscii(id);
        hdr[0x18] = petId.size() > 0 ? petId[0] : 'C';
        hdr[0x19] = petId.size() > 1 ? petId[1] : 'C';
        hdr[0x1A] = 0xA0;
        hdr[0x1B] = '2'; // DOS type
        hdr[0x1C] = 'C';
    }
    allocateSector(DIR_TRACK, 0);

    // Initialize first directory sector
    uint8_t* dir = sectorData(DIR_TRACK, 1);
    if (dir) {
        std::memset(dir, 0, 256);
        dir[0] = 0; // no next dir sector
        dir[1] = 0xFF;
    }
    allocateSector(DIR_TRACK, 1);
}

// ============================================================================
// Disk name/ID
// ============================================================================

std::string D80Image::diskName() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, 0);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x06), 16));
}

std::string D80Image::diskId() const {
    const uint8_t* hdr = sectorData(DIR_TRACK, 0);
    if (!hdr) return "";
    char id[3] = {(char)hdr[0x18], (char)hdr[0x19], 0};
    return petsciiToAscii(std::string(id, 2));
}

bool D80Image::setDiskName(const std::string& name) {
    uint8_t* hdr = sectorData(DIR_TRACK, 0);
    if (!hdr) return false;
    padPetsciiName(reinterpret_cast<char*>(hdr + 0x06), name);
    return true;
}

bool D80Image::setDiskId(const std::string& id) {
    uint8_t* hdr = sectorData(DIR_TRACK, 0);
    if (!hdr) return false;
    std::string petId = asciiToPetscii(id);
    hdr[0x18] = petId.size() > 0 ? petId[0] : 0xA0;
    hdr[0x19] = petId.size() > 1 ? petId[1] : 0xA0;
    return true;
}

// ============================================================================
// File operations (use common DiskImage dir traversal)
// ============================================================================

std::vector<FileInfo> D80Image::listFiles() const {
    std::vector<FileInfo> files;
    TrackSector ts = {DIR_TRACK, 1};
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

bool D80Image::fileExists(const std::string& name) const {
    TrackSector ts; int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D80Image::readFile(const std::string& name) const {
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

bool D80Image::addFile(const std::string& name, CbmFileType type,
                        const std::vector<uint8_t>& data) {
    // Allocate directory entry
    DirEntry newEntry;
    std::memset(&newEntry, 0, sizeof(newEntry));
    newEntry.fileType = 0x80 | (uint8_t)type; // closed + type
    padPetsciiName(newEntry.filename, name);

    TrackSector dirTS = allocDirectoryEntry(newEntry);
    if (dirTS.isNull()) return false;

    // Write file data
    TrackSector prev = {0, 0};
    int bytesLeft = (int)data.size();
    int pos = 0;

    while (bytesLeft > 0) {
        TrackSector ts = allocateNextFree(DIR_TRACK);
        if (ts.isNull()) return false;

        uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) return false;
        std::memset(sec, 0, 256);

        if (prev.track != 0) {
            // Link previous sector to this one
            uint8_t* prevSec = sectorData(prev.track, prev.sector);
            prevSec[0] = ts.track;
            prevSec[1] = ts.sector;
        } else {
            // First data sector
            newEntry.firstDataTS = ts;
        }

        int chunk = std::min(bytesLeft, 254);
        std::memcpy(sec + 2, data.data() + pos, chunk);

        if (bytesLeft <= 254) {
            // Last sector
            sec[0] = 0;
            sec[1] = chunk + 1;
        }

        pos += chunk;
        bytesLeft -= chunk;
        newEntry.sizeInSectors++;
        prev = ts;
    }

    // Handle empty files
    if (data.empty()) {
        TrackSector ts = allocateNextFree(DIR_TRACK);
        if (ts.isNull()) return false;
        uint8_t* sec = sectorData(ts.track, ts.sector);
        std::memset(sec, 0, 256);
        sec[0] = 0; sec[1] = 1;
        newEntry.firstDataTS = ts;
        newEntry.sizeInSectors = 1;
    }

    // Write directory entry
    newEntry.toSector(sectorData(dirTS.track, dirTS.sector),
                      -1); // need to find the slot again
    // Actually, use findFileEntry to update
    TrackSector fts; int fidx;
    // Re-write using allocDirectoryEntry's slot
    // The entry was written by allocDirectoryEntry, update it with firstDataTS
    {
        TrackSector dts = {DIR_TRACK, 1};
        while (dts.track != 0) {
            uint8_t* sec = sectorData(dts.track, dts.sector);
            if (!sec) break;
            for (int i = 0; i < 8; i++) {
                DirEntry e = DirEntry::fromSector(sec, i);
                if (e.isValid() && std::memcmp(e.filename, newEntry.filename, 16) == 0) {
                    e.firstDataTS = newEntry.firstDataTS;
                    e.sizeInSectors = newEntry.sizeInSectors;
                    e.toSector(sec, i);
                    return true;
                }
            }
            uint8_t nextT = sec[0], nextS = sec[1];
            if (nextT == 0) break;
            dts = {nextT, nextS};
        }
    }
    return true;
}

bool D80Image::removeFile(const std::string& name) {
    TrackSector dirTS;
    int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;

    DirEntry entry = DirEntry::fromSector(sectorData(dirTS.track, dirTS.sector), entryIdx);

    // Free all file sectors
    auto chain = getFileChain(entry.firstDataTS);
    for (auto& ts : chain) {
        freeSector(ts.track, ts.sector);
    }

    // Clear directory entry
    uint8_t* sec = sectorData(dirTS.track, dirTS.sector);
    uint8_t* p = sec + 2 + entryIdx * 32;
    p[2] = 0; // clear file type (marks as deleted)

    return true;
}
