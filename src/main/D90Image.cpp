#include "D90Image.hpp"
#include <cstring>
#include <algorithm>

int D90Image::sectorOffset(int track, int sector) const {
    if (track < 1 || track > totalTracks()) return -1;
    if (sector < 0 || sector >= 32) return -1;
    return ((track - 1) * 32 + sector) * 256;
}

void D90Image::format(const std::string& diskName, const std::string& diskId) {
    image_.assign(totalSectors() * 256, 0);
    // Simplified: just set up directory header at track 39, sector 0
    uint8_t* hdr = sectorData(39, 0);
    if (hdr) {
        hdr[0] = 39; hdr[1] = 1; // first dir sector
        hdr[2] = 'H'; // DOS version (hard drive)
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

int D90Image::freeSectors() const { return 0; } // BAM not implemented

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

bool D90Image::addFile(const std::string&, CbmFileType, const std::vector<uint8_t>&) {
    return false; // read-only (no BAM management)
}

bool D90Image::removeFile(const std::string&) {
    return false;
}
