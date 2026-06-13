#pragma once
#include "DiskImage.hpp"

// ARK: Arkive archive format — uncompressed collection of CBM files
// Simple format: 1-byte file count, then 29-byte directory entries,
// followed by file data in 254-byte block alignment.
//
// Directory entry (29 bytes):
//   $00: file type/attribute (bit 0-2: type, bit 6: locked, bit 7: closed)
//   $01: last sector used bytes (LSU)
//   $02-$11: filename (16 chars, PETSCII, $A0 padded)
//   $12: REL record size
//   $13-$18: unused
//   $19: side sector count (REL)
//   $1A: last sector bytes (REL)
//   $1B-$1C: file length in 254-byte blocks (lo/hi)

class ArkImage : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; } // closest match
    std::string diskName() const override { return "ARK ARCHIVE"; }
    std::string diskId() const override { return "AK"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return (int)image_.size() / 254; }
    int freeSectors() const override { return 0; }

    // ARK-specific: check if data looks like an ARK file
    static bool isArkFile(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    struct ArkEntry {
        uint8_t fileType;
        uint8_t lsu;
        char filename[16];
        uint8_t relRecordSize;
        uint16_t blockCount;
    };

    int fileCount() const;
    ArkEntry readEntry(int index) const;
    int dirSizeBytes() const;
    int dataOffset() const;
    int fileDataOffset(int fileIndex) const;
};
