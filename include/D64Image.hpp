#pragma once
#include "DiskImage.hpp"

// D64: 1541 disk image — 35 tracks, variable sectors per track, 174,848 bytes
// Track zones: 1-17 = 21 sectors, 18-24 = 19, 25-30 = 18, 31-35 = 17
// BAM + directory at track 18; directory starts at track 18 sector 1
// Total: 683 sectors × 256 bytes = 174,848 bytes (standard, no error bytes)

class D64Image : public DiskImage {
public:
    static constexpr int TRACKS = 35;
    static constexpr int DIR_TRACK = 18;
    static constexpr int BAM_SECTOR = 0;
    static constexpr int DIR_FIRST_SECTOR = 1;
    static constexpr int TOTAL_SECTORS = 683;
    static constexpr int IMAGE_SIZE = TOTAL_SECTORS * 256; // 174848

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
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override;
    int sectorsOnTrack(int track) const override;
    int totalSectors() const override;
    int freeSectors() const override;

    // Static helper: sectors on a given track (D64 zone layout)
    static int sectorsForTrack(int track);

protected:
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return DIR_TRACK; }
};
