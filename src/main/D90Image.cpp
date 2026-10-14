#include "D90Image.hpp"
#include <cstring>
#include <algorithm>

namespace {
const uint32_t D9090_BYTES = 153u * 6 * 32 * 256;
const uint32_t D9060_BYTES = 153u * 4 * 32 * 256;
const int BAM_MAP = 0x10;   // the first track entry of a BAM sector
}

int D90Image::sectorOffset(int track, int sector) const {
    if (track < 0 || track > 152 || sector < 0 || sector >= heads() * 32) return -1;
    return (track * heads() * 32 + sector) * 256;
}

bool D90Image::loadFromFile(const std::string& path) {
    if (!DiskImage::loadFromFile(path)) return false;
    if (image_.size() == D9090_BYTES) is9090_ = true;
    else if (image_.size() == D9060_BYTES) is9090_ = false;
    else return false;
    return true;
}

// ----------------------------------------------------------------------------
// The configuration block (0/0) locates the directory, header and BAM; an
// image whose block is empty uses the places format() gives them
// ----------------------------------------------------------------------------

TrackSector D90Image::firstDirectorySector() const {
    const uint8_t* cfg = sectorData(0, 0);
    if (cfg && cfg[4]) return {cfg[4], cfg[5]};
    return {76, 10};
}

TrackSector D90Image::headerSector() const {
    const uint8_t* cfg = sectorData(0, 0);
    if (cfg && cfg[6]) return {cfg[6], cfg[7]};
    return {76, 20};
}

std::vector<TrackSector> D90Image::bamSectors() const {
    std::vector<TrackSector> v;
    const uint8_t* cfg = sectorData(0, 0);
    TrackSector ts = cfg && cfg[8] ? TrackSector{cfg[8], cfg[9]} : TrackSector{1, 0};
    for (int guard = 0; ts.track != 255 && guard < 64; guard++) {
        const uint8_t* b = sectorData(ts.track, ts.sector);
        if (!b) break;
        v.push_back(ts);
        ts = {b[0], b[1]};
    }
    return v;
}

std::vector<TrackSector> D90Image::systemSectors() const {
    std::vector<TrackSector> v = {{0, 0}, {0, 1}};
    for (auto& ts : bamSectors()) v.push_back(ts);
    return v;
}

// ----------------------------------------------------------------------------
// BAM
// ----------------------------------------------------------------------------

const uint8_t* D90Image::bamEntry(int track, int sector) const {
    if (track < 0 || track > 152 || sector < 0 || sector >= heads() * 32) return nullptr;
    for (auto& ts : bamSectors()) {
        const uint8_t* b = sectorData(ts.track, ts.sector);
        if (track >= b[4] && track < b[5]) {
            int at = BAM_MAP + 5 * ((track - b[4]) * heads() + sector / 32);
            return at + 5 <= 256 ? b + at : nullptr;
        }
    }
    return nullptr;
}

uint8_t* D90Image::bamEntry(int track, int sector) {
    return const_cast<uint8_t*>(static_cast<const D90Image*>(this)->bamEntry(track, sector));
}

bool D90Image::isSectorFree(int track, int sector) const {
    const uint8_t* e = bamEntry(track, sector);
    int s = sector & 31;
    return e && (e[1 + s / 8] & (1 << (s % 8)));
}

bool D90Image::allocateSector(int track, int sector) {
    uint8_t* e = bamEntry(track, sector);
    int s = sector & 31;
    if (!e || !(e[1 + s / 8] & (1 << (s % 8)))) return false;
    e[1 + s / 8] &= (uint8_t)~(1 << (s % 8));
    e[0]--;
    return true;
}

bool D90Image::freeSector(int track, int sector) {
    uint8_t* e = bamEntry(track, sector);
    int s = sector & 31;
    if (!e || (e[1 + s / 8] & (1 << (s % 8)))) return false;
    e[1 + s / 8] |= (uint8_t)(1 << (s % 8));
    e[0]++;
    return true;
}

// Outwards from the directory track, as the drive does; never track 0
// (a link to track 0 ends a chain)
TrackSector D90Image::allocateNextFree(int nearTrack) {
    if (nearTrack < 1 || nearTrack > 152) nearTrack = directoryTrack();
    for (int delta = 0; delta <= 152; delta++) {
        for (int dir : {1, -1}) {
            if (delta == 0 && dir < 0) continue;
            int t = nearTrack + delta * dir;
            if (t < 1 || t > 152) continue;
            for (int s = 0; s < heads() * 32; s++)
                if (isSectorFree(t, s) && allocateSector(t, s)) return {(uint8_t)t, (uint8_t)s};
        }
    }
    return {0, 0};
}

int D90Image::freeSectors() const {
    int n = 0;
    for (int t = 1; t <= 152; t++)
        for (int s = 0; s < heads() * 32; s++)
            if (isSectorFree(t, s)) n++;
    return n;
}

// ----------------------------------------------------------------------------
// Format, as VICE's (and the drive's) N command lays it out
// ----------------------------------------------------------------------------

void D90Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign((size_t)totalSectors() * 256, 0);
    std::string id = asciiToPetscii(diskId.empty() ? "CC" : diskId);
    id.resize(2, ' ');

    uint8_t* cfg = sectorData(0, 0);
    cfg[1] = 1;        // the bad-block list: 0/1
    cfg[3] = 0xFF;     // DOS 3.0
    cfg[4] = 76;       // the directory
    cfg[5] = 10;
    cfg[6] = 76;       // the header
    cfg[7] = 20;
    cfg[8] = 1;        // the first BAM sector
    cfg[9] = 0;
    cfg[10] = (uint8_t)id[0];
    cfg[11] = (uint8_t)id[1];
    std::memset(sectorData(0, 1), 0xFF, 256);

    uint8_t* hdr = sectorData(76, 20);
    hdr[0] = 76;
    hdr[1] = 10;
    std::memset(hdr + 0x06, 0xA0, 27);
    std::string name = asciiToPetscii(diskName.empty() ? "CBM HD" : diskName);
    std::memcpy(hdr + 0x06, name.data(), std::min<size_t>(16, name.size()));
    hdr[0x18] = (uint8_t)id[0];
    hdr[0x19] = (uint8_t)id[1];
    hdr[0x1B] = '3';
    hdr[0x1C] = 'A';
    uint8_t* dir = sectorData(76, 10);
    dir[1] = 0xFF;

    // BAM sectors on tracks 1, 1+n, 1+2n... (at most 152) sector 0, each
    // covering n tracks (240 bytes of 5-byte head entries); the last up to
    // track 152
    int perSector = 240 / heads() / 5;
    int count = (153 + perSector - 1) / perSector;
    std::vector<int> at;
    for (int i = 0; i < count; i++) at.push_back(std::min(1 + i * perSector, 152));
    for (int i = 0; i < count; i++) {
        uint8_t* b = sectorData(at[i], 0);
        bool last = i + 1 == count;
        b[0] = last ? 255 : (uint8_t)at[i + 1];
        b[1] = last ? 255 : 0;
        b[2] = i == 0 ? 255 : (uint8_t)at[i - 1];
        b[3] = i == 0 ? 255 : 0;
        b[4] = (uint8_t)(i * perSector);
        b[5] = last ? 153 : (uint8_t)((i + 1) * perSector);
    }
    for (int t = 0; t <= 152; t++)
        for (int s = 0; s < heads() * 32; s++) freeSector(t, s);
    for (auto& ts : systemSectors()) allocateSector(ts.track, ts.sector);
    allocateSector(76, 20);
    allocateSector(76, 10);
}

// ----------------------------------------------------------------------------
// Header, files
// ----------------------------------------------------------------------------

std::string D90Image::diskName() const {
    TrackSector h = headerSector();
    const uint8_t* hdr = sectorData(h.track, h.sector);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x06), 16));
}

std::string D90Image::diskId() const {
    TrackSector h = headerSector();
    const uint8_t* hdr = sectorData(h.track, h.sector);
    if (!hdr) return "";
    return petsciiToAscii(std::string(reinterpret_cast<const char*>(hdr + 0x18), 2));
}

bool D90Image::setDiskName(const std::string& name) {
    TrackSector h = headerSector();
    uint8_t* hdr = sectorData(h.track, h.sector);
    if (!hdr) return false;
    padPetsciiName(reinterpret_cast<char*>(hdr + 0x06), name);
    return true;
}

bool D90Image::setDiskId(const std::string& id) {
    TrackSector h = headerSector();
    uint8_t* hdr = sectorData(h.track, h.sector);
    uint8_t* cfg = sectorData(0, 0);
    if (!hdr || !cfg) return false;
    std::string p = asciiToPetscii(id);
    p.resize(2, ' ');
    hdr[0x18] = cfg[10] = (uint8_t)p[0];
    hdr[0x19] = cfg[11] = (uint8_t)p[1];
    return true;
}

std::vector<FileInfo> D90Image::listFiles() const {
    return listDirectory(directoryStart());
}

bool D90Image::fileExists(const std::string& name) const {
    TrackSector ts;
    int idx;
    return findFileEntry(name, ts, idx);
}

std::vector<uint8_t> D90Image::readFile(const std::string& name) const {
    DirEntry e = findFile(name);
    if (!e.isValid()) return {};
    return chainBytes(e.firstDataTS);
}

bool D90Image::addFile(const std::string& name, CbmFileType type, const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false;
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

bool D90Image::removeFile(const std::string& name) {
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
