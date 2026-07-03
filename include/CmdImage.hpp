#pragma once
#include "DiskImage.hpp"

// CMD device disk images:
//   D1M: CMD FD-2000 DD — 81 tracks × 10 sectors × 256 bytes = 207,360 bytes
//   D2M: CMD FD-2000 HD — 81 tracks × 20 sectors × 256 bytes = 414,720 bytes
//   D4M: CMD FD-4000 ED — 81 tracks × 40 sectors × 256 bytes = 829,440 bytes
//   DNP: CMD Native Partition — up to 255 tracks × 256 sectors × 256 bytes
//
// All use a D81-compatible directory/BAM structure at track 40.
// DNP partitions are variable-size but follow the same conventions.

class CmdImage : public DiskImage {
public:
    enum CmdFormat { FD_1M, FD_2M, FD_4M, NATIVE };

    CmdImage(CmdFormat fmt = FD_2M) : fmt_(fmt) {}

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
    int totalTracks() const override;
    int sectorsOnTrack(int track) const override;
    int totalSectors() const override;
    int freeSectors() const override;

protected:
    CmdFormat fmt_;
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 1; } // DNP uses track 1
};
