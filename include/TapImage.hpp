#pragma once
#include "DiskImage.hpp"

// TAP: Commodore tape image format (raw pulse data)
//
// Header (20 bytes):
//   $00-$0B: Signature "C64-TAPE-RAW"
//   $0C: Version (0=original, 1=cycle-exact, 2=halfwave)
//   $0D: Platform (0=C64, 1=VIC-20, 2=C16, 3=PET)
//   $0E: Video standard (0=PAL, 1=NTSC)
//   $0F: Reserved
//   $10-$13: Data size (32-bit LE, excluding header)
//   $14+: Pulse data
//
// Files are the KERNAL's tape format (TapImage.cpp): a pilot tone of short
// pulses, then each block twice (countdown $89..$81, then $09..$01), its
// bytes framed by a long+medium marker with odd parity, and a checksum.
// The short/medium/long boundaries are calibrated from each pilot tone, in
// the platform's ratios (C64 / VIC-20 1 : 1.4 : 1.8, C16 / Plus 4 1 : 2 : 4);
// version 2 (C16) stores half-waves, joined into full waves here.
//
// Read-only: list and extract operations. Create/add not supported
// (encoding pulses requires timing precision beyond this scope).

class TapImage : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; } // closest match
    std::string diskName() const override { return "TAPE IMAGE"; }
    std::string diskId() const override { return "TP"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return 0; }
    int freeSectors() const override { return 0; }

    // TAP-specific info
    int tapVersion() const;
    int tapPlatform() const;
    uint32_t dataSize() const;   // the pulse data's size (header $10-$13)
    std::string platformName() const;

    static bool isTapFile(const std::vector<uint8_t>& data);

    // The timing family to decode with, instead of the header's platform
    // (disk45 --tap-timing=c64|vic20|c16): 0 C64, 1 VIC-20, 2 C16/Plus 4; -1 from the header
    static void setTimingOverride(int platform) { timingOverride_ = platform; }
    int timingPlatform() const;   // the one used

    // Each file's state: verified (both or the first copy good), repaired (a
    // block taken from its second copy), damaged (no good copy of a block)
    enum class FileState { Verified, Repaired, Damaged };
    struct FileReport { std::string name; CbmFileType type; FileState state; std::string detail; };
    std::vector<FileReport> report() const;
    bool isDamaged(const std::string& name) const;

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    static constexpr int TAP_HEADER_SIZE = 20;

    // CBM tape block types
    enum TapeBlockType : uint8_t {
        BLOCK_HEADER = 1,       // File header (type, name, addresses)
        BLOCK_DATA = 2,         // File data
        BLOCK_NONDOC_HEADER = 3, // Non-relocatable header
        BLOCK_NONDOC_DATA = 4,  // Non-relocatable data
        BLOCK_EOT = 5,          // End of tape marker
    };

    struct TapeFile {
        std::string name;
        CbmFileType type = CbmFileType::PRG;
        uint16_t startAddr = 0;
        uint16_t endAddr = 0;
        std::vector<uint8_t> data;
        FileState state = FileState::Verified;
        std::string detail;   // why it is repaired or damaged
    };

    // One block as read: its countdown ($89 first copy, $09 second), its
    // payload (without countdown and checksum) and whether it verified
    struct Block {
        uint8_t countdown = 0;
        std::vector<uint8_t> payload;
        bool checksumOk = false;
        int parityErrors = 0;
        bool framingError = false;   // a pulse pair that is no bit, before the end marker
        bool good() const { return checksumOk && parityErrors == 0 && !framingError; }
    };
    // A record: a block's two copies, the one to use
    struct Record { std::vector<uint8_t> payload; FileState state; std::string detail; };

    static int timingOverride_;

    std::vector<uint32_t> readPulses() const;               // full waves, in cycles
    std::vector<Block> decodeBlocks() const;
    static std::vector<Record> pairCopies(const std::vector<Block>& blocks);
    static std::vector<TapeFile> filesFromRecords(const std::vector<Record>& records);
    std::vector<TapeFile> decodeFiles() const;
};
