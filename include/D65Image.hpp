#pragma once
#include "DiskImage.hpp"

// D65: MEGA65 native disk image — based on C65 1565 drive specification
// 81 tracks × 40 sectors × 256 bytes per side, 2 sides
// Essentially a double-sided D81 with an extra track
// Side 0: tracks 1-81, side 1: tracks 82-162
// BAM: track 40 sectors 1-2 (side 0), track 40 sectors 3-4 (side 1 BAM)
// Header: track 40 sector 0
// Directory: track 40 sector 5+
// Total: 81 × 40 × 2 = 6480 sectors = 1,658,880 bytes

class D65Image : public DiskImage {
public:
    static constexpr int TRACKS_PER_SIDE = 81;
    static constexpr int SIDES = 2;
    static constexpr int TRACKS = TRACKS_PER_SIDE * SIDES; // 162
    static constexpr int SECTORS_PER_TRACK = 40;
    static constexpr int TOTAL_SECTORS = TRACKS * SECTORS_PER_TRACK; // 6480
    static constexpr int IMAGE_SIZE = TOTAL_SECTORS * 256; // 1658880
    static constexpr int DIR_TRACK = 40;
    static constexpr int HEADER_SECTOR = 0;
    static constexpr int BAM_S0_SEC1 = 1;  // Side 0 BAM tracks 1-40
    static constexpr int BAM_S0_SEC2 = 2;  // Side 0 BAM tracks 41-81
    static constexpr int BAM_S1_SEC1 = 3;  // Side 1 BAM tracks 82-122
    static constexpr int BAM_S1_SEC2 = 4;  // Side 1 BAM tracks 123-162
    static constexpr int DIR_FIRST_SECTOR = 5;

    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D65; }
    std::string diskName() const override;
    std::string diskId() const override;
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override { return TRACKS; }
    int sectorsOnTrack(int) const override { return SECTORS_PER_TRACK; }
    int totalSectors() const override { return TOTAL_SECTORS; }
    int freeSectors() const override;

protected:
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return DIR_TRACK; }

private:
    void bamLocation(int track, int& bamSector, int& entryOffset) const;
};
