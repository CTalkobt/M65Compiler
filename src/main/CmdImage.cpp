#include "CmdImage.hpp"
#include <algorithm>
#include <cstring>

namespace {
const uint32_t NATIVE_DIR_SECTOR = 34;  // 1/34
}

const char* CmdImage::partitionTypeName(uint8_t type) {
    switch (type) {
        case 1: return "native";
        case 2: return "1541 (D64)";
        case 3: return "1571 (D71)";
        case 4: return "1581 (D81)";
        case 5: return "1581 CP/M";
        case 6: return "print buffer";
        case 7: return "foreign";
        case 255: return "system";
        default: return "unknown";
    }
}

const char* CmdImage::formatName() const {
    switch (fmt_) {
        case FD_1M: return "D1M (CMD FD-2000 DD)";
        case FD_2M: return "D2M (CMD FD-2000 HD)";
        case FD_4M: return "D4M (CMD FD-4000 ED)";
        case NATIVE: break;
    }
    return "DNP (CMD native partition)";
}

uint32_t CmdImage::diskSectors() const {
    switch (fmt_) {
        case FD_1M: return 3240;
        case FD_2M: return 6480;
        case FD_4M: return 12960;
        case NATIVE: break;
    }
    return 0;
}

// the reserved last physical track (81 tracks of 40/80/160 sectors)
uint32_t CmdImage::systemStart() const {
    return diskSectors() - diskSectors() / 81;
}

int CmdImage::totalTracks() const {
    return (int)((partSectors_ + 255) / 256);
}

int CmdImage::sectorsOnTrack(int track) const {
    int tracks = totalTracks();
    if (track < 1 || track > tracks) return 0;
    if (track < tracks) return 256;
    return (int)(partSectors_ - 256u * (tracks - 1));
}

int CmdImage::sectorOffset(int track, int sector) const {
    if (track < 1 || sector < 0 || sector > 255) return -1;
    uint32_t idx = (uint32_t)(track - 1) * 256 + sector;
    if (idx >= partSectors_) return -1;
    return (int)((partStart_ + idx) * 256);
}

std::vector<TrackSector> CmdImage::systemSectors() const {
    std::vector<TrackSector> v;
    for (int s = 0; s <= 33; s++) v.push_back({1, (uint8_t)s});  // boot block, header, BAM
    return v;
}

TrackSector CmdImage::firstDirectorySector() const {
    const uint8_t* hdr = sectorData(1, 1);
    if (!hdr) return {0, 0};
    return {hdr[0], hdr[1]};
}

// ----------------------------------------------------------------------------
// BAM: from offset $20 of 1/2, 32 bytes per track over 1/2-1/33
// ----------------------------------------------------------------------------

const uint8_t* CmdImage::bamByte(int track, int byteIdx) const {
    if (track < 1 || track > 255 || byteIdx < 0 || byteIdx > 31) return nullptr;
    uint32_t g = 0x20 + (uint32_t)(track - 1) * 32 + byteIdx;
    const uint8_t* sec = sectorData(1, 2 + g / 256);
    return sec ? sec + g % 256 : nullptr;
}

uint8_t* CmdImage::bamByte(int track, int byteIdx) {
    return const_cast<uint8_t*>(static_cast<const CmdImage*>(this)->bamByte(track, byteIdx));
}

bool CmdImage::isSectorFree(int track, int sector) const {
    if (sector < 0 || sector >= sectorsOnTrack(track)) return false;
    const uint8_t* b = bamByte(track, sector / 8);
    return b && (*b & (0x80 >> (sector % 8)));
}

bool CmdImage::allocateSector(int track, int sector) {
    uint8_t* b = bamByte(track, sector / 8);
    if (!b || !(*b & (0x80 >> (sector % 8)))) return false;
    *b &= (uint8_t)~(0x80 >> (sector % 8));
    return true;
}

bool CmdImage::freeSector(int track, int sector) {
    if (sector < 0 || sector >= sectorsOnTrack(track)) return false;
    uint8_t* b = bamByte(track, sector / 8);
    if (!b) return false;
    *b |= (uint8_t)(0x80 >> (sector % 8));
    return true;
}

TrackSector CmdImage::allocateNextFree(int) {
    for (int t = 1; t <= totalTracks(); t++)
        for (int s = 0; s < sectorsOnTrack(t); s++)
            if (isSectorFree(t, s) && allocateSector(t, s)) return {(uint8_t)t, (uint8_t)s};
    return {0, 0};
}

int CmdImage::freeSectors() const {
    int n = 0;
    for (int t = 1; t <= totalTracks(); t++)
        for (int s = 0; s < sectorsOnTrack(t); s++)
            if (isSectorFree(t, s)) n++;
    return n;
}

// ----------------------------------------------------------------------------
// Loading and formatting
// ----------------------------------------------------------------------------

void CmdImage::readPartitions() {
    parts_.clear();
    partStart_ = partSectors_ = 0;
    uint32_t sys = systemStart();
    for (int s = 0; s < 4; s++) {
        size_t off = (size_t)(sys + 8 + s) * 256;
        if (off + 256 > image_.size()) break;
        for (int i = 0; i < 8; i++) {
            const uint8_t* e = image_.data() + off + i * 32;
            int number = s * 8 + i;
            if (number == 0 || e[2] == 0 || e[2] == 0xFF) continue;
            Partition p;
            p.number = number;
            p.type = e[2];
            std::string name((const char*)e + 5, 16);
            while (!name.empty() && (uint8_t)name.back() == 0xA0) name.pop_back();
            p.name = petsciiToAscii(name);
            p.start = (uint32_t)((e[0x16] << 8) | e[0x17]) * 2;
            p.sectors = (uint32_t)((e[0x1E] << 8) | e[0x1F]) * 2;
            parts_.push_back(p);
            if (p.type == 1 && partSectors_ == 0 && p.start + p.sectors <= sys) {
                partStart_ = p.start;
                partSectors_ = p.sectors;
            }
        }
    }
}

bool CmdImage::loadFromFile(const std::string& path) {
    if (!DiskImage::loadFromFile(path)) return false;
    if (fmt_ == NATIVE) {
        if (image_.size() < 512 * 256 || image_.size() % 256) return false;
        partStart_ = 0;
        partSectors_ = (uint32_t)std::min<size_t>(image_.size() / 256, 255u * 256);
        return true;
    }
    uint32_t n = diskSectors();
    if (image_.size() != (size_t)n * 256 && image_.size() != (size_t)n * 257) return false;
    readPartitions();
    return true;
}

void CmdImage::formatNative(const std::string& diskName, const std::string& diskId) {
    std::memset(image_.data() + (size_t)partStart_ * 256, 0, (size_t)partSectors_ * 256);
    std::string name = asciiToPetscii(diskName.empty() ? "CMD DISK" : diskName);
    std::string id = asciiToPetscii(diskId.empty() ? "CC" : diskId);
    id.resize(2, ' ');
    // root header 1/1
    uint8_t* hdr = sectorData(1, 1);
    hdr[0] = 1;
    hdr[1] = NATIVE_DIR_SECTOR;
    hdr[2] = 'H';
    std::memset(hdr + 0x04, 0xA0, 18);  // the name, padded to $15
    std::memcpy(hdr + 0x04, name.data(), std::min<size_t>(16, name.size()));
    hdr[0x16] = (uint8_t)id[0];
    hdr[0x17] = (uint8_t)id[1];
    hdr[0x18] = 0xA0;
    hdr[0x19] = '1';
    hdr[0x1A] = 'H';
    hdr[0x1B] = hdr[0x1C] = 0xA0;
    hdr[0x20] = 1;   // this header
    hdr[0x21] = 1;
    // BAM 1/2: format "H", its complement, ID, I/O byte, the last track
    uint8_t* bam = sectorData(1, 2);
    bam[2] = 'H';
    bam[3] = (uint8_t)~'H';
    bam[4] = (uint8_t)id[0];
    bam[5] = (uint8_t)id[1];
    bam[6] = 0xC0;
    bam[8] = (uint8_t)totalTracks();
    for (int t = 1; t <= 255; t++)
        for (int k = 0; k < 32; k++) {
            uint8_t* b = bamByte(t, k);
            if (!b) continue;
            if (t > totalTracks()) { *b = 0xFF; continue; }  // past the partition (as CMD fills it)
            uint8_t v = 0;
            for (int bit = 0; bit < 8; bit++)
                if (k * 8 + bit < sectorsOnTrack(t)) v |= (uint8_t)(0x80 >> bit);
            *b = v;
        }
    // 1/0 boot block, 1/1 header, 1/2-1/33 BAM, 1/34 directory
    for (uint32_t s = 0; s <= NATIVE_DIR_SECTOR; s++) allocateSector(1, (int)s);
    uint8_t* dir = sectorData(1, NATIVE_DIR_SECTOR);
    dir[0] = 0x00;
    dir[1] = 0xFF;
}

void CmdImage::format(const std::string& diskName, const std::string& diskId) {
    if (fmt_ == NATIVE) {
        // 32 tracks (2 MB) unless disk45 create -t N asks for another size
        uint32_t tracks = (uint32_t)std::max(2, std::min(255, nativeTracks_));
        image_.assign(tracks * 256 * 256, 0);
        partStart_ = 0;
        partSectors_ = tracks * 256;
        formatNative(diskName, diskId);
        return;
    }
    image_.assign((size_t)diskSectors() * 256, 0);
    uint32_t sys = systemStart();
    // DevBlock (system partition sector 5), as D2M-DNP.TXT gives it
    uint8_t* dev = image_.data() + (size_t)(sys + 5) * 256;
    std::memset(dev, 0xFF, 256);
    dev[0x00] = 0x00;
    dev[0x38] = dev[0x39] = 0x00;
    dev[0x70] = 0x00; dev[0x71] = 0x0C;
    dev[0xA8] = 0x00; dev[0xA9] = 0x80;
    std::memset(dev + 0xE0, 0, 16);
    dev[0xE2] = dev[0xE3] = 0x01;
    std::memcpy(dev + 0xF0, "CMD FD SERIES   ", 16);
    // partition directory: sectors 8-11, linked 1/1, 1/2, 1/3, 00/FF
    for (int s = 0; s < 4; s++) {
        uint8_t* d = image_.data() + (size_t)(sys + 8 + s) * 256;
        d[0] = s < 3 ? 0x01 : 0x00;
        d[1] = s < 3 ? (uint8_t)(s + 1) : 0xFF;
    }
    uint8_t* root = image_.data() + (size_t)(sys + 8) * 256;
    root[2] = 0xFF;
    std::memset(root + 5, 0xA0, 16);
    std::memcpy(root + 5, "SYSTEM", 6);
    // partition 1: native, the whole disk before the system track
    uint8_t* p1 = root + 32;
    p1[2] = 0x01;
    std::string pname = asciiToPetscii(diskName.empty() ? "PARTITION 1" : diskName);
    std::memset(p1 + 5, 0xA0, 16);
    std::memcpy(p1 + 5, pname.data(), std::min<size_t>(16, pname.size()));
    uint32_t blocks = sys / 2;
    p1[0x16] = 0;
    p1[0x17] = 0;
    p1[0x1E] = (uint8_t)(blocks >> 8);
    p1[0x1F] = (uint8_t)blocks;
    readPartitions();
    formatNative(diskName, diskId);
}

// ----------------------------------------------------------------------------
// Header, files
// ----------------------------------------------------------------------------

std::string CmdImage::diskName() const {
    const uint8_t* hdr = sectorData(1, 1);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x04), 16));
}

std::string CmdImage::diskId() const {
    const uint8_t* hdr = sectorData(1, 1);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x16), 2));
}

bool CmdImage::setDiskName(const std::string& name) {
    uint8_t* hdr = sectorData(1, 1);
    if (!hdr) return false;
    padPetsciiName(reinterpret_cast<char*>(hdr + 0x04), name);
    return true;
}

bool CmdImage::setDiskId(const std::string& id) {
    uint8_t* hdr = sectorData(1, 1);
    uint8_t* bam = sectorData(1, 2);
    if (!hdr || !bam) return false;
    std::string p = asciiToPetscii(id);
    p.resize(2, ' ');
    hdr[0x16] = bam[4] = (uint8_t)p[0];
    hdr[0x17] = bam[5] = (uint8_t)p[1];
    return true;
}

std::vector<FileInfo> CmdImage::listFiles() const {
    std::vector<FileInfo> files;
    TrackSector ts = firstDirectorySector();
    for (int guard = 0; ts.track != 0 && guard < 65536; guard++) {
        const uint8_t* sec = sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (!e.isValid()) continue;
            FileInfo fi;
            fi.name = e.name();
            fi.type = e.type();
            fi.sizeInSectors = e.sizeInSectors;
            fi.sizeInBytes = e.sizeInSectors * 254;
            fi.locked = (e.fileType & 0x40) != 0;
            fi.closed = e.isClosed();
            files.push_back(fi);
        }
        if (sec[0] == 0) break;
        ts = {sec[0], sec[1]};
    }
    return files;
}

bool CmdImage::fileExists(const std::string& name) const {
    TrackSector ts;
    int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> CmdImage::readFile(const std::string& name) const {
    DirEntry e = findFile(name);
    if (!e.isValid()) return {};
    return chainBytes(e.firstDataTS);
}

bool CmdImage::addFile(const std::string& name, CbmFileType type, const std::vector<uint8_t>& data) {
    if (!hasNative() || fileExists(name)) return false;
    size_t need = data.empty() ? 1 : (data.size() + 253) / 254;
    if ((int)need > freeSectors()) return false;
    TrackSector first = writeChainBytes(data.data(), data.size());
    if (first.isNull()) return false;
    DirEntry entry;
    entry.fileType = 0x80 | (uint8_t)type;
    entry.firstDataTS = first;
    padPetsciiName(entry.filename, name);
    entry.sizeInSectors = (uint16_t)need;
    return !allocDirectoryEntry(entry).isNull();
}

bool CmdImage::removeFile(const std::string& name) {
    TrackSector dirTS;
    int entryIdx;
    if (!findFileEntry(name, dirTS, entryIdx)) return false;
    uint8_t* dirSec = sectorData(dirTS.track, dirTS.sector);
    if (!dirSec) return false;
    DirEntry e = DirEntry::fromSector(dirSec, entryIdx);
    for (auto& ts : fileSectors(e)) freeSector(ts.track, ts.sector);
    e.fileType = 0;
    e.toSector(dirSec, entryIdx);
    return true;
}
