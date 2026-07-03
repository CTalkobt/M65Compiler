#pragma once
#include "DiskImage.hpp"

// GEOS CVT (ConVerT) file format
//
// CVT is a sequential container for GEOS files, allowing them to be
// stored outside the GEOS filesystem (as PRG or SEQ on CBM disks).
//
// Structure (254-byte blocks, no T/S link bytes):
//   Block 0: Signature block
//     $00-$01: PRG load address (if PRG format) or $00/$00 (if SEQ)
//     $02-$1D: Signature "PRG formatted GEOS file V1.0" or
//              "SEQ formatted GEOS file V1.0"
//     $1E-$FD: Zero padding
//   Block 1: GEOS Info block (254 bytes)
//     $00-$02: Icon width (bytes), height, format
//     $03-$3F: Icon bitmap data (up to 63 bytes)
//     $40: C64/C128 flag ($00=C64, $80=C128)
//     $41: GEOS file type ($00=non-GEOS, $01=BASIC, $06=application, etc.)
//     $42: GEOS file structure ($00=sequential, $01=VLIR)
//     $43-$44: Load address (lo/hi)
//     $45-$46: End address (lo/hi)
//     $47-$4C: Date/time (year-1900, month, day, hour, minute, second)
//     $4D-$5D: Parent application name (zero-padded)
//     $5E-$BD: Description/info box (up to 96 chars)
//   Block 2 (VLIR only): Record table (254 bytes)
//     127 × 2-byte entries: T/S pairs or status markers
//     $00/$00 = empty record, $00/$FF = record not present
//     Non-zero T/S = record data follows in data blocks
//   Block 3+: Data blocks (254 bytes each)
//     Sequential: all data concatenated
//     VLIR: records concatenated in order, sizes from record table
//
// Read-only: GEOS encoding is complex; write support would need icon/info editing.

class GeosCvtImage : public DiskImage {
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
    std::string diskId() const override { return "GE"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return 0; }
    int freeSectors() const override { return 0; }

    // GEOS-specific info
    std::string geosFileName() const;
    uint8_t geosFileType() const;
    uint8_t geosFileStructure() const; // 0=sequential, 1=VLIR
    std::string geosFileTypeName() const;
    std::string geosClassName() const;
    std::string geosDescription() const;
    bool isVlir() const;
    int vlirRecordCount() const;

    static bool isCvtFile(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    static constexpr int BLOCK_SIZE = 254;

    // Access a 254-byte block (no T/S link)
    const uint8_t* block(int n) const;
    int blockCount() const;
};
