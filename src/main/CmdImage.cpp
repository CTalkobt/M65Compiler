#include "CmdImage.hpp"
#include <cstring>
#include <algorithm>

int CmdImage::totalTracks() const {
    switch (fmt_) {
        case FD_1M: return 81;
        case FD_2M: return 81;
        case FD_4M: return 81;
        case NATIVE: return (int)(image_.size() / (256 * 256)); // derive from file size
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

int CmdImage::freeSectors() const { return 0; } // BAM not implemented for read-only

void CmdImage::format(const std::string& diskName, const std::string& diskId) {
    int spt = sectorsOnTrack(1);
    int tracks = (fmt_ == NATIVE) ? 255 : 81;
    image_.assign(tracks * spt * 256, 0);

    // Directory header at track 1, sector 0 (CMD convention)
    uint8_t* hdr = sectorData(1, 0);
    if (hdr) {
        hdr[0] = 1; hdr[1] = 3; // first dir sector
        hdr[2] = 'H'; // DOS version
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

bool CmdImage::addFile(const std::string&, CbmFileType, const std::vector<uint8_t>&) {
    return false; // read-only
}

bool CmdImage::removeFile(const std::string&) {
    return false;
}
