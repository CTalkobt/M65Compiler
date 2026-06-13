#pragma once
#include "DiskImage.hpp"

// LNX (Lynx): Archive format by Will Corley for C64/C128 BBS systems.
// Structure: BASIC stub + ASCII header + directory entries + 254-byte block-aligned data.
// All numbers and metadata stored as ASCII text (not binary), CR-terminated.
//
// Header:
//   - BASIC stub (~94 bytes, displays "Use LYNX to dissolve this file")
//   - Signature line containing "LYNX" (CR-terminated)
//   - File count as ASCII number (CR-terminated)
// Directory (per file):
//   - Filename (16 chars PETSCII, $A0 padded)
//   - Block count (ASCII, CR-terminated) — 254 bytes per block, includes 2-byte load addr
//   - File type: P, S, U, or R (ASCII, CR-terminated)
//   - LSU: last sector used bytes (ASCII, CR-terminated)
//   - If REL: record size (ASCII, CR-terminated), then LSU (ASCII, CR-terminated)
// Data:
//   - Files stored sequentially, each occupying blockCount × 254 bytes

class LnxImage : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; }
    std::string diskName() const override { return "LNX ARCHIVE"; }
    std::string diskId() const override { return "LX"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return (int)image_.size() / 254; }
    int freeSectors() const override { return 0; }

    static bool isLnxFile(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    struct LnxEntry {
        char filename[17];  // PETSCII, null-terminated
        int blockCount;
        char fileType;      // P, S, U, R
        int lsu;            // last sector used bytes
        int recSize;        // REL record size (0 if not REL)
        size_t dataOffset;  // byte offset of file data in image_
    };

    // Parse the archive: find header end, directory entries, data offsets
    bool parseHeader(size_t& dirStart, int& fileCount) const;
    std::vector<LnxEntry> parseEntries() const;

    // Read an ASCII number from the image at pos, advance pos past CR
    int readAsciiNumber(size_t& pos) const;
    // Read a CR-terminated string
    std::string readCRString(size_t& pos) const;

    // Generate the BASIC stub + header for writing
    static std::vector<uint8_t> buildHeader(int fileCount);
};
