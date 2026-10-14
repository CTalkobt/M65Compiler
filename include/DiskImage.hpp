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
    DIR = 6,    // CMD native partition subdirectory (a header block, then its directory)
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

    // --- Images inside a .zip ---
    // A path "x.d81.zip", "x.zip" (its one disk image, or the member named
    // like the zip) or "archive.zip:member.d64" names a zip member: load()
    // reads it through a temporary file, saveImage() writes it back into the
    // zip (the other members are kept as they are) or creates the zip.
    // saveImage() saves any other path with saveToFile().
    bool saveImage(const std::string& path) const;
    // Split a zip path; false when it is not one
    static bool zipPath(const std::string& path, std::string& zip, std::string& member);
    // "card.img:/DIR/X.D81": a file on a MEGA65 SD card image (load/saveImage
    // read and write it there)
    static bool cardPath(const std::string& path, std::string& card, std::string& member);
    // "disk.d2m:N": partition N (a number, or its name) of a CMD D1M/D2M/D4M.
    // A native partition is opened in place; an emulated 1541/1571/1581 one
    // as a D64/D71/D81 whose bytes saveImage writes back into the image
    static bool partitionPath(const std::string& path, std::string& image, std::string& part);
    // A MEGA65 SD card image (MBR with FAT32 / $41 partitions, 32 MB or more)
    static bool isSdCard(const std::string& path);
    static bool isImageExtension(const std::string& name);

    // Whether disk45 can write this format back. G64/G71, NIB, Zipcode
    // (decoded to D64 sectors on load) and D90/D60/CMD (no correct BAM
    // writer) are read-only: saveToFile refuses them. A blank image from
    // create is still written for the formats that allow it (canCreateBlank)
    virtual bool writable() const { return true; }
    virtual bool canCreateBlank() const { return writable(); }
    void markBlank() { blank_ = true; }

    // Directory access for commands that build entries themselves (REL files)
    DirEntry findEntry(const std::string& name) const { return findFile(name); }
    TrackSector addEntry(DirEntry& entry) { return allocDirectoryEntry(entry); }
    // The first directory sector: the header sector's link (directory track,
    // sector 0); D80/D82 override (their header links to the BAM)
    virtual TrackSector firstDirectorySector() const;
    // The track of the header sector (sector 0) and the directory
    int headerTrack() const { return directoryTrack(); }
    // The directory file operations work in (find, add, remove, rename,
    // readFile, listFiles): the root (default) or a CBM subdirectory's
    // chain, its entry's first data sector. {0,0} returns to the root
    // (and, for a CMD native subdirectory, the header block that precedes it)
    void setDirectory(TrackSector start, TrackSector header = {0, 0}) { cwd_ = start; cwdHeader_ = header; }
    TrackSector directoryStart() const { return cwd_.isNull() ? firstDirectorySector() : cwd_; }
    TrackSector directoryHeader() const { return cwdHeader_; }
    bool inSubdirectory() const { return !cwd_.isNull(); }
    // The image's bytes as loaded or formatted (a sector image's sectors)
    const std::vector<uint8_t>& rawBytes() const { return image_; }
    // A subdirectory entry (CBM, or a CMD native DIR) and its directory chain
    // (a DIR entry points to a header block, whose link starts the chain)
    static bool isDirectory(const DirEntry& e) {
        return e.type() == CbmFileType::CBM || e.type() == CbmFileType::DIR;
    }
    TrackSector directoryChain(const DirEntry& e) const;
    // The working directory gained (or lost) directory blocks: a CMD native
    // subdirectory's entry counts them
    void directoryBlocksChanged(int delta);
    // Make a subdirectory entry of the working directory the working one
    void enterDirectory(const DirEntry& e);
    // Enter a path of subdirectories from the root (empty: the root); false
    // when a component is not a subdirectory (the working directory is then
    // the root)
    bool enterPath(const std::vector<std::string>& dirs);
    // The header sector (disk name, ID, GEOS signature): sector 0 of the
    // directory track; a CMD native partition's is 1/1
    virtual TrackSector headerSector() const { return {(uint8_t)directoryTrack(), 0}; }
    // Sectors the format reserves besides the header and directory (counted
    // as used by validate): a CMD native partition's boot block and BAM
    virtual std::vector<TrackSector> systemSectors() const { return {}; }
    // REL files use a 1581 super side sector (D81, D65)
    virtual bool superSideSectors() const { return false; }

    // --- File operations ---
    virtual std::vector<FileInfo> listFiles() const = 0;
    virtual bool addFile(const std::string& name, CbmFileType type,
                         const std::vector<uint8_t>& data) = 0;
    virtual bool removeFile(const std::string& name) = 0;
    virtual bool renameFile(const std::string& oldName, const std::string& newName);
    virtual bool lockFile(const std::string& name, bool locked);
    // Move a file of the working directory into the directory chain at `to`
    // (the root: firstDirectorySector()) as newName; refused for a
    // subdirectory entry and when `to` already has newName
    bool moveFile(const std::string& name, TrackSector to, const std::string& newName,
                  TrackSector toHeader = {0, 0});
    // A CBM subdirectory of the working directory: an entry of type CBM
    // whose first data sector starts the subdirectory's own directory chain.
    // makeDirectory fails when the name exists or the disk is full;
    // removeDirectory returns 0, -ENOENT (no such directory) or -ENOTEMPTY,
    // and frees the subdirectory's sectors
    virtual bool makeDirectory(const std::string& name);
    int removeDirectory(const std::string& name);
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

    // --- GEOS files (Schepers' GEOS.TXT / CVT.TXT) ---
    // A GEOS file has an info block (directory bytes $15/$16) and, when VLIR
    // (structure byte $17 = 1), a record block of up to 127 sector chains.
    // On a host it is kept as a Convert (CVT) file, as cbmconvert writes it
    bool isGeos(const DirEntry& e) const;
    bool readGeosCvt(const DirEntry& e, std::vector<uint8_t>& cvt, std::string& err) const;
    bool addGeosCvt(const std::vector<uint8_t>& cvt, const std::string& name, std::string& err);
    static bool isCvt(const std::vector<uint8_t>& data);
    static std::string cvtName(const std::vector<uint8_t>& cvt);
    static std::string geosTypeName(uint8_t type);
    // "GEOS format V1.x" at $AD of the header sector, or ""
    std::string geosSignature() const;
    // Make a D64/D71 a GEOS disk: the signature and a border sector
    bool makeGeosDisk(std::string& err);
    // Every sector a file occupies: its chain, a REL file's side sectors,
    // a GEOS file's info block, record block and records
    std::vector<TrackSector> fileSectors(const DirEntry& e) const;
    // The bytes of a sector chain (the last sector's link byte gives its length)
    std::vector<uint8_t> chainBytes(TrackSector start, bool* ok = nullptr) const;

    // --- Raw sector access ---
    uint8_t* sectorData(int track, int sector);
    const uint8_t* sectorData(int track, int sector) const;

    // --- Utility ---
    static std::string asciiToPetscii(const std::string& ascii);
    static std::string petsciiToAscii(const std::string& petscii);
    static void padPetsciiName(char* dest, const std::string& name, int len = 16);
    static DiskFormat formatFromExtension(const std::string& path);

protected:
    std::string zipMember_;       // the member this image was loaded from
    std::vector<uint8_t> image_;  // raw disk image bytes
    bool blank_ = false;          // freshly formatted by create (markBlank)
    TrackSector cwd_;             // setDirectory: a subdirectory, or {0,0}
    TrackSector cwdHeader_;       // a CMD native subdirectory's header block

    // The files of a directory chain (listFiles in a subdirectory)
    std::vector<FileInfo> listDirectory(TrackSector start) const;

    // Subclasses implement: convert (track,sector) to byte offset in image_
    virtual int sectorOffset(int track, int sector) const = 0;

    // --- BAM operations (implemented by subclass) ---
public:
    virtual bool isSectorFree(int track, int sector) const = 0;
    virtual TrackSector allocateNextFree(int nearTrack = -1) = 0;
    virtual bool freeSector(int track, int sector) = 0;
protected:
    virtual bool allocateSector(int track, int sector) = 0;

    // --- Directory helpers (common logic, uses virtual BAM/sector methods) ---
    DirEntry findFile(const std::string& name) const;
    bool findFileEntry(const std::string& name, TrackSector& dirTS, int& entryIdx) const;
    std::vector<TrackSector> getFileChain(TrackSector start) const;
    TrackSector allocDirectoryEntry(DirEntry& entry);
    // Write bytes as a new sector chain; returns its first sector (0/0: full)
    TrackSector writeChainBytes(const uint8_t* data, size_t len);

    // Directory track (subclass provides)
    virtual int directoryTrack() const = 0;
};
