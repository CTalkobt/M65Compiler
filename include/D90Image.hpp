#pragma once
#include "DiskImage.hpp"

// D9090: CBM hard drive — 153 tracks × 6 heads × 32 sectors = 29376 sectors (7.52 MB)
// D9060: CBM hard drive — 153 tracks × 4 heads × 32 sectors = 19584 sectors (5.01 MB)
//
// Uses DOS 3.0 with linked-list BAM (dynamically located).
// Directory at track 39, similar to D80/D82.
// BAM sectors are self-locating (linked from the directory header).
//
// For image purposes, tracks are linearized: track = cylinder * heads + head + 1
// D9090: 918 logical tracks (153 × 6), 32 sectors each
// D9060: 612 logical tracks (153 × 4), 32 sectors each

class D90Image : public DiskImage {
public:
    D90Image(bool is9090 = true) : is9090_(is9090) {}

    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;
    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; }
    std::string diskName() const override;
    std::string diskId() const override;
    int totalTracks() const override { return is9090_ ? 918 : 612; }
    int sectorsOnTrack(int) const override { return 32; }
    int totalSectors() const override { return totalTracks() * 32; }
    int freeSectors() const override;

protected:
    bool is9090_;
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return 39; }
};
