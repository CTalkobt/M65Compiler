#pragma once
#include "DiskImage.hpp"

// D81: 1581 disk image — 80 tracks × 40 sectors × 256 bytes = 819,200 bytes
// BAM at track 40 sectors 1-2; directory starts at track 40 sector 3
// Header at track 40 sector 0

class D81Image : public DiskImage {
public:
    static constexpr int TRACKS = 80;
    static constexpr int SECTORS_PER_TRACK = 40;
    static constexpr int IMAGE_SIZE = TRACKS * SECTORS_PER_TRACK * 256; // 819200
    static constexpr int DIR_TRACK = 40;
    static constexpr int HEADER_SECTOR = 0;
    static constexpr int BAM_SECTOR_1 = 1;
    static constexpr int BAM_SECTOR_2 = 2;
    static constexpr int DIR_FIRST_SECTOR = 3;

    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D81; }
    std::string diskName() const override;
    std::string diskId() const override;
    int totalTracks() const override { return TRACKS; }
    int sectorsOnTrack(int) const override { return SECTORS_PER_TRACK; }
    int totalSectors() const override { return TRACKS * SECTORS_PER_TRACK; }
    int freeSectors() const override;

protected:
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return DIR_TRACK; }

private:
    void bamBitPos(int track, int sector, int& bamSector, int& byteOff, int& bitOff) const;
};
