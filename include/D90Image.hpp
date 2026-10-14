#pragma once
#include "DiskImage.hpp"

// D9090 / D9060: CBM hard drives (DOS 3.0), as VICE reads and writes them.
//   153 cylinders (tracks 0-152) of 6 / 4 heads x 32 sectors: a track has
//   192 / 128 sectors, sector = head * 32 + sector on the head; the image is
//   the tracks in order (7,520,256 / 5,013,504 bytes). The size tells them
//   apart, so .d90 and .d60 load either.
//   0/0 the configuration block: $00-$01 the bad-block list (0/1), $03 $FF,
//       $04-$05 the directory (76/10), $06-$07 the header (76/20),
//       $08-$09 the first BAM sector (1/0), $0A-$0B the ID
//   0/1 the bad-block list ($FF)
//   header: $00-$01 the directory, $06-$15 the name, $18-$19 the ID,
//       $1B-$1C the DOS version ("3A"), $A0 padding between
//   BAM: a chain of sectors (1/0, 9/0, 17/0... on a D9090), each $00-$01
//       the next (255/255 ends it), $02-$03 the previous (255/255 first),
//       $04 the first track it covers, $05 the track after the last; from
//       $10, per track and head, 5 bytes: the free count and 32 bits (bit
//       n of byte k = sector 8k+n of the head, 1 = free)
//   The directory: CBM entries on the header's track, linked from 76/10.
//   Track 0 holds only the system blocks: files are never put there.

class D90Image : public DiskImage {
public:
    D90Image(bool is9090 = true) : is9090_(is9090) {}

    bool loadFromFile(const std::string& path) override;
    void format(const std::string& diskName = "", const std::string& diskId = "CC") override;
    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;
    TrackSector firstDirectorySector() const override;
    TrackSector headerSector() const override;
    std::vector<TrackSector> systemSectors() const override;
    bool superSideSectors() const override { return true; }   // REL files, as VICE

    DiskFormat diskFormat() const override { return DiskFormat::D64; }
    std::string diskName() const override;
    std::string diskId() const override;
    bool setDiskName(const std::string& name) override;
    bool setDiskId(const std::string& id) override;
    int totalTracks() const override { return 152; }   // and track 0
    int sectorsOnTrack(int track) const override { return track >= 0 && track <= 152 ? heads() * 32 : 0; }
    int totalSectors() const override { return 153 * heads() * 32; }
    int freeSectors() const override;
    bool is9090() const { return is9090_; }

    bool isSectorFree(int track, int sector) const override;
    TrackSector allocateNextFree(int nearTrack = -1) override;
    bool freeSector(int track, int sector) override;

protected:
    bool is9090_;
    int heads() const { return is9090_ ? 6 : 4; }
    int sectorOffset(int track, int sector) const override;
    bool allocateSector(int track, int sector) override;
    int directoryTrack() const override { return 76; }

private:
    // The 5-byte BAM entry of a track's head (the free count, 32 bits)
    const uint8_t* bamEntry(int track, int sector) const;
    uint8_t* bamEntry(int track, int sector);
    std::vector<TrackSector> bamSectors() const;
};
