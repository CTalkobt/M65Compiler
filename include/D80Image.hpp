#pragma once
#include "DiskImage.hpp"

// D80: CBM 8050 disk image — 77 tracks, 2083 sectors, 533,248 bytes
// D82: CBM 8250 disk image — 154 tracks (double-sided 8050), 4166 sectors, 1,066,496 bytes
//
// Track zones: 1-39 = 29 sectors, 40-53 = 27, 54-64 = 25, 65-77 = 23
// D82: side 2 mirrors side 1 at tracks 78-154
// BAM at track 38 (sectors 0 and 3)
// Directory header at track 39, sector 0
// Directory chain starts at track 39, sector 1

class D80Image : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; } // closest
    std::string diskName() const override;
    std::string diskId() const override;
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override { return tracks_; }
    int sectorsOnTrack(int track) const override;
    int totalSectors() const override;
    int freeSectors() const override;

    static int sectorsForTrack8050(int track);

protected:
    int tracks_ = 77; // 77 for D80, 154 for D82

    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    bool freeSector(int track, int sector) override;
    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    int directoryTrack() const override { return 39; }

private:
    static constexpr int BAM_TRACK = 38;
    static constexpr int DIR_TRACK = 39;

    // BAM helpers
    int bamSectorForTrack(int track) const;
    int bamOffsetForTrack(int track) const;
};

// D82 is double-sided D80 (154 tracks)
class D82Image : public D80Image {
public:
    D82Image() { tracks_ = 154; }
};
