#pragma once
#include "DiskImage.hpp"

// T64: Commodore tape container format
//
// Header (64 bytes):
//   $00-$1F: Signature "C64S tape image file" or "C64 tape image file"
//   $20-$21: Version (LE)
//   $22-$23: Max directory entries (LE)
//   $24-$25: Used entries (LE)
//   $26-$27: Reserved
//   $28-$3F: Tape name (24 chars, space-padded)
//
// Directory entries (32 bytes each, starting at $40):
//   $00: Entry type (0=free, 1=normal, 2=with header)
//   $01: CBM file type ($82=PRG, $81=SEQ)
//   $02-$03: Start address (LE)
//   $04-$05: End address (LE)
//   $06-$07: Reserved
//   $08-$0B: Data offset in file (LE, 32-bit)
//   $0C-$0F: Reserved
//   $10-$1F: Filename (16 chars, $20 padded)
//
// Supports list, extract, info, create, add.

class T64Image : public DiskImage {
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
    std::string diskId() const override { return "T6"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return 0; }
    int freeSectors() const override { return 0; }

    static bool isT64File(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    static constexpr int T64_HEADER_SIZE = 64;
    static constexpr int T64_ENTRY_SIZE = 32;

    struct T64Entry {
        uint8_t entryType;
        uint8_t fileType;
        uint16_t startAddr;
        uint16_t endAddr;
        uint32_t dataOffset;
        char filename[16];
    };

    int maxEntries() const;
    int usedEntries() const;
    T64Entry readEntry(int index) const;
    std::string entryName(const T64Entry& e) const;
};
