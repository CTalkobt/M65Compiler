#pragma once
#include "DiskImage.hpp"

// P00/S00/U00/R00: PC64 single-file container
//
// Header (26 bytes):
//   $00-$07: Signature "C64File\0"
//   $08-$17: Original C64 filename (16 bytes, $00 padded)
//   $18-$19: REL record size (LE, 0 for non-REL)
// $1A+: File data (for PRG: 2-byte load address + data)
//
// Extension determines file type: Pxx=PRG, Sxx=SEQ, Uxx=USR, Rxx=REL

class P00Image : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "") override;
    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; }
    std::string diskName() const override;
    std::string diskId() const override { return "P0"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return 0; }
    int freeSectors() const override { return 0; }

    static bool isP00File(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    static constexpr int P00_HEADER_SIZE = 26;
    std::string originalName() const;
};
