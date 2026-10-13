#pragma once
#include "DiskImage.hpp"

// GEOS CVT (ConVerT) file: one GEOS file as a host file (Schepers' CVT.TXT;
// the layout cbmconvert reads and writes), 254-byte blocks:
//   Block 0: the directory entry without its first two bytes ($00-$1D:
//            CBM type, T/S, name padded with $A0, info T/S, structure
//            ($15: 0 sequential, 1 VLIR), GEOS type ($16), date, size),
//            then "PRG formatted GEOS file V1.0" at $1E
//   Block 1: the info block without its link (icon, types, load address,
//            class at $4B, author at $5F, description at $9E)
//   Block 2 (VLIR only): the record block: per record a block count and
//            the index of the last byte in its last block (00/00 no more
//            records, 00/FF an absent record)
//   Then the data: a sequential file's bytes, or the records, each padded
//   to whole blocks (the last one may be short)
// listFiles shows the file (sequential) or its records (RECORD_n); disk45
// add/convert turn a CVT into a GEOS file on a disk image.

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
    std::string geosAuthor() const;
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
    std::string infoString(int off, int len) const;
    // A VLIR record's offset and length in the file (false: absent)
    bool recordSpan(int rec, size_t& off, size_t& len) const;
};
