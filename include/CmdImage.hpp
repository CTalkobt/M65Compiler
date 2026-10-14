#pragma once
#include "DiskImage.hpp"
#include <vector>

// CMD device images (Schepers' D2M-DNP.TXT):
//   D1M / D2M / D4M: FD2000/FD4000 floppies, 3240 / 6480 / 12960 sectors
//     (829,440 / 1,658,880 / 3,317,760 bytes; error bytes may follow). The
//     reserved last physical track holds the system partition: the DevBlock
//     at its sector 5 and the partition directory in sectors 8-11 (up to 31
//     partitions: native, emulated 1541/1571/1581; offsets and sizes in
//     512-byte blocks).
//   DNP: one CMD native partition (a hard disk partition), up to 255 tracks.
//
// A native partition has 256-sector tracks counted from 1 inside the
// partition: 1/0 the C128 boot block, 1/1 the root header (name, ID, link to
// the directory, GEOS signature at $AD), 1/2-1/33 the BAM (from offset $20 of
// 1/2, 32 bytes per track, bit 7 of byte 0 = sector 0, 1 = free; byte $08 of
// 1/2 = the last track), the directory from 1/34 (CBM entries, the last
// sector linked 00/FF). disk45 works on the first native partition of a
// D1M/D2M/D4M; create makes one native partition filling the disk.

class CmdImage : public DiskImage {
public:
    enum CmdFormat { FD_1M, FD_2M, FD_4M, NATIVE };

    CmdImage(CmdFormat fmt = FD_2M) : fmt_(fmt) {}

    bool loadFromFile(const std::string& path) override;
    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;
    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;
    // A native subdirectory (type DIR): a header block, as the root's 1/1
    // (link to the directory, name, ID, "1H"; $20 itself, $22 the parent's
    // header, $24-$26 the entry in the parent), and one directory block
    bool makeDirectory(const std::string& name) override;
    TrackSector firstDirectorySector() const override;
    TrackSector headerSector() const override { return {1, 1}; }
    std::vector<TrackSector> systemSectors() const override;
    bool superSideSectors() const override { return true; }   // REL files, as VICE

    DiskFormat diskFormat() const override { return DiskFormat::D81; }
    std::string diskName() const override;
    std::string diskId() const override;
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override;
    int sectorsOnTrack(int track) const override;
    int totalSectors() const override { return (int)partSectors_; }
    int freeSectors() const override;

    struct Partition {
        int number = 0;
        uint8_t type = 0;          // 1 native, 2 1541, 3 1571, 4 1581
        std::string name;
        uint32_t start = 0;        // 256-byte sectors from the image start
        uint32_t sectors = 0;
    };
    // The partitions of a D1M/D2M/D4M (empty for a DNP)
    const std::vector<Partition>& partitions() const { return parts_; }
    bool hasNative() const { return partSectors_ > 0; }
    // A partition by number or name (case-insensitive); nullptr if none
    const Partition* findPartition(const std::string& spec) const;
    // Work on a native partition instead of the first one
    bool selectPartition(const Partition& p);
    // The size of an emulated partition's D64/D71/D81 (0: not emulated)
    static uint32_t emulatedBytes(uint8_t type);
    static const char* emulatedExtension(uint8_t type);
    uint32_t nativeStart() const { return partStart_; }
    const char* formatName() const;
    // DNP: the partition size for format() (1-255 tracks; default 32)
    void setNativeTracks(int tracks) { nativeTracks_ = tracks; }
    // D1M/D2M/D4M: the partitions format() lays out, from the start of the
    // disk (default: one native partition filling it). Type 1 native (blocks:
    // 512-byte blocks, rounded up to whole 256-sector tracks; 0 = the rest
    // of the disk), 2/3/4 an emulated 1541/1571/1581 (formatted as its
    // D64/D71/D81). False (with err) when the layout does not fit
    struct PartSpec { uint8_t type = 1; std::string name; uint32_t blocks = 0; };
    bool setLayout(const std::vector<PartSpec>& layout, std::string& err);
    static const char* partitionTypeName(uint8_t type);

    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    bool freeSector(int track, int sector) override;

protected:
    CmdFormat fmt_;
    uint32_t partStart_ = 0;     // the native partition, in 256-byte sectors
    uint32_t partSectors_ = 0;
    std::vector<Partition> parts_;
    int nativeTracks_ = 32;
    std::vector<PartSpec> layout_;   // resolved: every size known

    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    int directoryTrack() const override { return 1; }

private:
    uint32_t diskSectors() const;      // D1M/D2M/D4M: 3240 / 6480 / 12960
    uint32_t systemStart() const;      // the system partition's first sector
    uint8_t* bamByte(int track, int byteIdx);
    const uint8_t* bamByte(int track, int byteIdx) const;
    void formatNative(const std::string& diskName, const std::string& diskId);
    void readPartitions();
};
