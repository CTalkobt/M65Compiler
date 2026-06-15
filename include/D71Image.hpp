#pragma once
#include "DiskImage.hpp"

// D71: 1571 disk image — double-sided D64
// Side 0: tracks 1-35 (same zone layout as D64)
// Side 1: tracks 36-70 (same zone layout, mirrored)
// BAM for side 0: track 18, sector 0 (same as D64)
// BAM for side 1: track 53, sector 0
// Directory at track 18 (side 0 only)
// Total: 1366 sectors × 256 bytes = 349,696 bytes

class D71Image : public DiskImage {
public:
    static constexpr int TRACKS = 70;
    static constexpr int DIR_TRACK = 18;
    static constexpr int BAM_TRACK_SIDE0 = 18;
    static constexpr int BAM_TRACK_SIDE1 = 53;
    static constexpr int BAM_SECTOR = 0;
    static constexpr int DIR_FIRST_SECTOR = 1;
    static constexpr int TOTAL_SECTORS = 1366;
    static constexpr int IMAGE_SIZE = TOTAL_SECTORS * 256; // 349696

    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D71; }
    std::string diskName() const override;
    std::string diskId() const override;
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override { return TRACKS; }
    int sectorsOnTrack(int track) const override;
    int totalSectors() const override { return TOTAL_SECTORS; }
    int freeSectors() const override;

    static int sectorsForTrack(int track);

protected:
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return DIR_TRACK; }
};
