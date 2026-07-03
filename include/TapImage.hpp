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
// Files are encoded as CBM ROM loader protocol within the pulses:
// leader → sync → header block (192 bytes) → leader → sync → data block
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
    std::string platformName() const;

    static bool isTapFile(const std::vector<uint8_t>& data);

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
        CbmFileType type;
        uint16_t startAddr;
        uint16_t endAddr;
        std::vector<uint8_t> data;
    };

    // Decode pulse data into bytes, find tape blocks, extract files
    std::vector<TapeFile> decodeFiles() const;

    // Decode raw pulse data into a bit stream
    std::vector<uint8_t> decodePulses() const;

    // Find and decode tape blocks from the byte stream
    std::vector<TapeFile> extractFromBytes(const std::vector<uint8_t>& bytes) const;
};
