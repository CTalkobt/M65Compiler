#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ============================================================================
// CBM Disk Image Library — Common Types and Abstract Base
// Supports D64, D71, D81, D65 image formats.
// ============================================================================

// Track/sector address pair
struct TrackSector {
    uint8_t track = 0;
    uint8_t sector = 0;
    bool operator==(const TrackSector& o) const { return track == o.track && sector == o.sector; }
    bool operator!=(const TrackSector& o) const { return !(*this == o); }
    bool isNull() const { return track == 0 && sector == 0; }
};

// CBM file types (stored in directory entry byte $02, bits 0-3)
enum class CbmFileType : uint8_t {
    DEL = 0,    // Deleted/scratched
    SEQ = 1,    // Sequential
    PRG = 2,    // Program
    USR = 3,    // User
    REL = 4,    // Relative (record-based)
    CBM = 5,    // CBM subdirectory (D71/D81)
};

// Directory entry (32 bytes, 8 per directory sector)
struct DirEntry {
    TrackSector next;           // link to next dir sector (first entry only)
    uint8_t fileType = 0;       // bits 0-3: type, bit 6: locked, bit 7: closed
    TrackSector firstDataTS;    // first track/sector of file data
    char filename[16];          // PETSCII, padded with $A0
    TrackSector sideTS;         // side-sector (REL files)
    uint8_t recordLen = 0;      // REL record length
    uint8_t unused[4] = {};     // unused (or GEOS)
    uint8_t replaceTrk = 0;    // @SAVE replacement track
    uint8_t replaceSec = 0;    // @SAVE replacement sector
    uint16_t sizeInSectors = 0; // file size in sectors (lo/hi)

    bool isValid() const { return (fileType & 0x07) != 0; }
    bool isClosed() const { return (fileType & 0x80) != 0; }
    CbmFileType type() const { return (CbmFileType)(fileType & 0x07); }
    std::string name() const;       // returns trimmed ASCII name
    std::string petsciiName() const; // returns raw PETSCII name

    static DirEntry fromSector(const uint8_t* data, int index);
    void toSector(uint8_t* data, int index) const;
};

// File listing entry (high-level)
struct FileInfo {
    std::string name;           // ASCII filename
    CbmFileType type = CbmFileType::PRG;
    uint16_t sizeInSectors = 0;
    uint32_t sizeInBytes = 0;   // estimated from sector count
    bool locked = false;
    bool closed = true;
};

// Disk image format identifier
enum class DiskFormat {
    D64,    // C64 1541 — 35 tracks, 170KB
    D71,    // C128 1571 — 70 tracks, 340KB
    D81,    // C65/MEGA65 1581 — 80 tracks, 800KB
    D65,    // MEGA65 native — 81 tracks, 2×D81
};

// ============================================================================
// Abstract disk image base class
// ============================================================================

class DiskImage {
public:
    virtual ~DiskImage() = default;

    // --- Factory ---
    static std::unique_ptr<DiskImage> create(DiskFormat fmt);
    static std::unique_ptr<DiskImage> createFromExtension(const std::string& path);
    static std::unique_ptr<DiskImage> load(const std::string& path);

    // --- Image lifecycle ---
    virtual void format(const std::string& diskName = "", const std::string& diskId = "CC") = 0;
    virtual bool loadFromFile(const std::string& path);
    virtual bool saveToFile(const std::string& path) const;

    // --- File operations ---
    virtual std::vector<FileInfo> listFiles() const = 0;
    virtual bool addFile(const std::string& name, CbmFileType type,
                         const std::vector<uint8_t>& data) = 0;
    virtual bool removeFile(const std::string& name) = 0;
    virtual bool renameFile(const std::string& oldName, const std::string& newName);
    virtual bool lockFile(const std::string& name, bool locked);
    virtual std::vector<uint8_t> readFile(const std::string& name) const = 0;
    virtual bool fileExists(const std::string& name) const = 0;

    // --- Image info ---
    virtual DiskFormat diskFormat() const = 0;
    virtual std::string diskName() const = 0;
    virtual std::string diskId() const = 0;
    virtual bool setDiskName(const std::string& name);
    virtual bool setDiskId(const std::string& id);
    virtual int totalTracks() const = 0;
    virtual int sectorsOnTrack(int track) const = 0;
    virtual int totalSectors() const = 0;
    virtual int freeSectors() const = 0;
    virtual int totalBytes() const { return totalSectors() * 256; }

    // --- Validation ---
    struct ValidateResult {
        int filesFound = 0;
        int sectorsUsedByFiles = 0;
        int sectorsMarkedUsed = 0;
        int freeSectorsInBAM = 0;
        int crossLinked = 0;        // sectors claimed by multiple files
        int orphanedSectors = 0;    // marked used but not in any file chain
        int brokenChains = 0;       // chains pointing to invalid track/sector
        std::vector<std::string> errors;
        bool ok() const { return errors.empty(); }
    };
    virtual ValidateResult validate() const;

    // --- Raw sector access ---
    uint8_t* sectorData(int track, int sector);
    const uint8_t* sectorData(int track, int sector) const;

    // --- Utility ---
    static std::string asciiToPetscii(const std::string& ascii);
    static std::string petsciiToAscii(const std::string& petscii);
    static void padPetsciiName(char* dest, const std::string& name, int len = 16);
    static DiskFormat formatFromExtension(const std::string& path);

protected:
    std::vector<uint8_t> image_;  // raw disk image bytes

    // Subclasses implement: convert (track,sector) to byte offset in image_
    virtual int sectorOffset(int track, int sector) const = 0;

    // --- BAM operations (implemented by subclass) ---
public:
    virtual bool isSectorFree(int track, int sector) const = 0;
    virtual TrackSector allocateNextFree(int nearTrack = -1) = 0;
protected:
    virtual bool allocateSector(int track, int sector) = 0;
    virtual bool freeSector(int track, int sector) = 0;

    // --- Directory helpers (common logic, uses virtual BAM/sector methods) ---
    DirEntry findFile(const std::string& name) const;
    bool findFileEntry(const std::string& name, TrackSector& dirTS, int& entryIdx) const;
    std::vector<TrackSector> getFileChain(TrackSector start) const;
    TrackSector allocDirectoryEntry(DirEntry& entry);

    // Directory track (subclass provides)
    virtual int directoryTrack() const = 0;
};
