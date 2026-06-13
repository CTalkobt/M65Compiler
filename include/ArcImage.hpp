#pragma once
#include "DiskImage.hpp"

// ARC: Commodore ARC archive format — compressed collection of CBM files
// Supports compression modes: stored, packed (RLE), squeezed (Huffman),
// crunched (LZW 12-bit), squeezed+packed, crunched one-pass.
// Based on cbmconvert's unarc.c by Chris Smeets and Marko Mäkelä.

class ArcImage : public DiskImage {
public:
    void format(const std::string& diskName = "", const std::string& diskId = "") override;

    std::vector<FileInfo> listFiles() const override;
    bool addFile(const std::string& name, CbmFileType type,
                 const std::vector<uint8_t>& data) override;
    bool removeFile(const std::string& name) override;
    std::vector<uint8_t> readFile(const std::string& name) const override;
    bool fileExists(const std::string& name) const override;

    DiskFormat diskFormat() const override { return DiskFormat::D64; }
    std::string diskName() const override { return "ARC ARCHIVE"; }
    std::string diskId() const override { return "AR"; }
    int totalTracks() const override { return 0; }
    int sectorsOnTrack(int) const override { return 0; }
    int totalSectors() const override { return (int)image_.size() / 254; }
    int freeSectors() const override { return 0; }

    static bool isArcFile(const std::vector<uint8_t>& data);

protected:
    int sectorOffset(int, int) const override { return -1; }
    bool allocateSector(int, int) override { return false; }
    bool freeSector(int, int) override { return false; }
    bool isSectorFree(int, int) const override { return false; }
    TrackSector allocateNextFree(int) override { return {0, 0}; }
    int directoryTrack() const override { return 0; }

private:
    // Internal entry parsed from archive
    struct ArcEntry {
        uint8_t version = 0;   // 1 or 2
        uint8_t mode = 0;      // compression: 0-5
        uint16_t checksum = 0;
        uint32_t origSize = 0; // 3 bytes in file
        uint16_t blocks = 0;   // compressed blocks (254 bytes each)
        uint8_t fileType = 0;  // P/S/U/R
        uint8_t fnLen = 0;
        char filename[17] = {};
        uint8_t recLen = 0;    // REL record length (v2)
        uint16_t date = 0;     // MS-DOS date (v2)
        size_t dataOffset = 0; // byte offset of compressed data in image_
    };

    // Parse all entries from the archive
    std::vector<ArcEntry> parseEntries() const;

    // Find start of data (skip SDA header if present)
    size_t getStartPos() const;

    // Decompress a single file entry
    std::vector<uint8_t> decompressEntry(const ArcEntry& entry) const;

    // --- Decompression algorithms ---

    // Bit-stream reader state
    struct BitReader {
        const uint8_t* data;
        size_t size;
        size_t pos;
        unsigned buf;

        BitReader(const uint8_t* d, size_t s, size_t startPos)
            : data(d), size(s), pos(startPos), buf(2) {}

        uint8_t getByte();
        uint16_t getWord();
        unsigned getBit();
        bool eof() const { return pos >= size; }
    };

    // RLE unpacking (applied after Huffman or raw data for modes 1, 4)
    static std::vector<uint8_t> unpackRLE(const std::vector<uint8_t>& input,
                                          unsigned ctrl, uint8_t version);

    // Huffman decoding (modes 2, 4)
    struct HuffmanTree {
        unsigned long codes[257];
        uint8_t lengths[257];
        uint8_t values[257];
        int count;
        bool build(BitReader& reader);
        int decode(BitReader& reader) const;
    };

    static std::vector<uint8_t> decompressHuffman(BitReader& reader, size_t origSize);

    // LZW decoding (modes 3, 5)
    static std::vector<uint8_t> decompressLZW(BitReader& reader, size_t origSize, uint8_t mode);
};
