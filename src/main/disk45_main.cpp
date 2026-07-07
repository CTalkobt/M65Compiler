#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
#include "TapImage.hpp"
#include "GeosCvtImage.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

static const char* VERSION = "1.0.4";

// Forward declaration for FUSE mount (in disk45_fuse.cpp)
#ifdef HAVE_FUSE3
extern int disk45_fuse_main(const std::string& imagePath, const std::string& mountpoint,
                             bool readonly, int argc, char** argv);
#endif

// Forward declaration for catalog (in disk45_catalog.cpp)
extern int disk45_catalog(int argc, char** argv);

static void usage() {
    std::cerr << "disk45 — CBM Disk Image Utility v" << VERSION << "\n"
              << "Usage:\n"
              << "  disk45 create <image> [-n name] [-i id] [-t tracks]  Create disk image\n"
              << "  disk45 list <image>                       List directory\n"
              << "  disk45 info <image>                       Show disk info\n"
              << "  disk45 add <image> <file> [cbm_name] [-p]  Add file to image\n"
              << "  disk45 extract <image> <cbm_name> <file> [-p]  Extract file\n"
              << "  disk45 extract-all <image> [dir] [pattern]  Extract all files\n"
              << "  disk45 copy <src> <pattern> <dst>         Copy files between images\n"
              << "  disk45 remove <image> <cbm_name>          Delete file from image\n"
              << "  disk45 rename <image> <old> <new>         Rename file in image\n"
              << "  disk45 lock <image> <cbm_name>            Lock file (prevent delete)\n"
              << "  disk45 unlock <image> <cbm_name>          Unlock file\n"
              << "  disk45 splat <image> <cbm_name>           Mark file as unclosed (splat)\n"
              << "  disk45 unsplat <image> <cbm_name>         Mark file as closed\n"
              << "  disk45 label <image> [-n name] [-i id]    Change disk name/ID\n"
              << "  disk45 validate <image>                   Check BAM consistency\n"
              << "  disk45 bam <image>                        Visual BAM sector map\n"
              << "  disk45 dump <image> [track] [sector]      Hex dump sector(s)\n"
              << "  disk45 bread <image> <t> <s> [file]       Read raw sector\n"
              << "  disk45 bwrite <image> <t> <s> <file>      Write raw sector\n"
              << "  disk45 bpeek <image> <t> <s> <off> [n]    Read byte(s)\n"
              << "  disk45 bpoke <image> <t> <s> <off> <val>  Write byte(s)\n"
              << "  disk45 bfill <image> <t> <s> <val>        Fill sector with value\n"
#ifdef HAVE_FUSE3
              << "  disk45 mount <image> <mountpoint> [-ro]   FUSE mount as directory\n"
#endif
              << "  disk45 convert <source> <target>          Convert between formats\n"
              << "  disk45 chain <image> <filename>           Show file sector chain\n"
              << "  disk45 mkdir <image> <dirname>            Create subdirectory\n"
              << "  disk45 rmdir <image> <dirname>            Remove empty subdirectory\n"
              << "  disk45 tree <image>                       Show directory tree\n"
              << "  disk45 dir-shrink <image> [path]          Reclaim empty dir sectors\n"
              << "  disk45 dir-ensure <image> <slots> [path]  Ensure min free dir slots\n"
              << "  disk45 rel-create <img> <name> <len> [n]  Create REL file\n"
              << "  disk45 rel-read <img> <name> <rec#>       Read REL record\n"
              << "  disk45 rel-write <img> <name> <rec#> <d>  Write REL record\n"
              << "  disk45 rel-list <img> <name>              List REL records\n"
              << "  disk45 catalog build <dir>... [--db file]  Index image collection\n"
              << "  disk45 catalog search <pattern> [--db f]  Search indexed files\n"
              << "  disk45 catalog list [--db file]           List indexed images\n"
              << "  disk45 catalog duplicates [--db file]     Find duplicate files\n"
              << "  disk45 catalog stats [--db file]          Collection statistics\n"
              << "  disk45 -a2p                               ASCII→PETSCII filter (stdin→stdout)\n"
              << "  disk45 -p2a                               PETSCII→ASCII filter (stdin→stdout)\n"
              << "  disk45 batch <scriptfile>                 Run commands from file\n"
              << "\n"
              << "Supported formats (auto-detected from extension):\n"
              << "  .d64  C64 1541 (170KB, 35 tracks)\n"
              << "  .d71  C128 1571 (340KB, 70 tracks)\n"
              << "  .d81  C65/MEGA65 1581 (800KB, 80 tracks)\n"
              << "  .d65  MEGA65 native (1.6MB, 162 tracks)\n"
              << "  .ark  Arkive (uncompressed archive)\n"
              << "  .arc  ARC archive (stored/RLE/Huffman/LZW)\n"
              << "  .sda  Self-Dissolving ARC archive\n"
              << "  .lnx  Lynx archive (block-aligned, uncompressed)\n"
              << "  .tap  TAP tape image (read-only, pulse decoding)\n"
              << "  .t64  T64 tape container (read/write)\n"
              << "  .g64  G64 GCR-encoded 1541 (read-only, decodes to D64)\n"
              << "  .g71  G71 GCR-encoded 1571 (read-only, decodes to D71)\n"
              << "  .d80  D80 CBM 8050 (77 tracks, 533KB)\n"
              << "  .d82  D82 CBM 8250 (154 tracks, 1MB)\n"
              << "  .cvt  GEOS Convert (read-only, info/list/extract)\n"
              << "  .x64  X64 extended D64 (64-byte header + D64)\n"
              << "  .p00  P00/S00/U00/R00 PC64 single-file container\n"
              << "  1!xx  Zipcode 4-pack (read-only, decodes to D64)\n"
              << "  .d90  D9090 CBM hard drive (7.5MB, read-only)\n"
              << "  .d60  D9060 CBM hard drive (5MB, read-only)\n"
              << "  .d1m  CMD FD-2000 DD (207KB, read-only)\n"
              << "  .d2m  CMD FD-2000 HD (414KB, read-only)\n"
              << "  .d4m  CMD FD-4000 ED (829KB, read-only)\n"
              << "  .dnp  CMD Native Partition (up to 16MB, read-only)\n"
              << "  .nib  NIB raw nibble dump (read-only, decodes to D64)\n"
              << "\n"
              << "Wildcards: use * and ? in patterns for list, extract-all, copy, remove\n"
              << "\n"
              << "Flags:\n"
              << "  -p, --petscii   Convert SEQ content: ASCII→PETSCII (add) or\n"
              << "                  PETSCII→ASCII (extract). Swaps case, LF↔CR.\n"
              << "\n"
              << "GZ compression: append .gz to any format (e.g. .d81.gz)\n";
}

static const char* fileTypeStr(CbmFileType t) {
    switch (t) {
        case CbmFileType::CBM: return "CBM";
        case CbmFileType::DEL: return "DEL";
        case CbmFileType::SEQ: return "SEQ";
        case CbmFileType::PRG: return "PRG";
        case CbmFileType::USR: return "USR";
        case CbmFileType::REL: return "REL";
    }
    return "???";
}

static const char* formatStr(DiskFormat f) {
    switch (f) {
        case DiskFormat::D64: return "D64 (1541)";
        case DiskFormat::D71: return "D71 (1571)";
        case DiskFormat::D81: return "D81 (1581)";
        case DiskFormat::D65: return "D65 (MEGA65)";
    }
    return "???";
}

// --- PETSCII ↔ ASCII content conversion for SEQ files ---

static std::vector<uint8_t> asciiToPetsciiContent(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(data.size());
    for (uint8_t c : data) {
        if (c == '\n') out.push_back(0x0D);            // LF → CR
        else if (c == '\r') continue;                   // strip CR (handle CRLF)
        else if (c >= 'a' && c <= 'z') out.push_back(c - 32);  // lowercase → PETSCII $41-$5A
        else if (c >= 'A' && c <= 'Z') out.push_back(c + 128); // uppercase → PETSCII $C1-$DA
        else if (c == '~') out.push_back(0xA8);         // tilde → pi
        else if (c == '|') out.push_back(0x7D);         // pipe → bar
        else if (c == '\\') out.push_back(0x5C);        // backslash → pound
        else out.push_back(c);
    }
    return out;
}

static std::vector<uint8_t> petsciiToAsciiContent(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(data.size());
    for (uint8_t c : data) {
        if (c == 0x0D) out.push_back('\n');             // CR → LF
        else if (c >= 0x41 && c <= 0x5A) out.push_back(c + 32);  // PETSCII $41-$5A → lowercase
        else if (c >= 0xC1 && c <= 0xDA) out.push_back(c - 128); // PETSCII $C1-$DA → uppercase
        else if (c >= 0x20 && c < 0x7F) out.push_back(c);  // printable ASCII range
        else if (c == 0xA8) out.push_back('~');         // pi → tilde
        else if (c == 0x7D) out.push_back('|');         // bar → pipe
        else if (c == 0x00) break;                      // null terminator
        else out.push_back(c);                          // pass through
    }
    return out;
}

// Check if -p or --petscii flag is present, remove it from args
static bool extractPetsciiFlag(int& argc, char**& argv) {
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "-p" || std::string(argv[i]) == "--petscii") {
            // Shift remaining args down
            for (int j = i; j < argc - 1; j++) argv[j] = argv[j + 1];
            argc--;
            return true;
        }
    }
    return false;
}

static CbmFileType typeFromExtension(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".prg") return CbmFileType::PRG;
        if (ext == ".seq") return CbmFileType::SEQ;
        if (ext == ".usr") return CbmFileType::USR;
    }
    return CbmFileType::PRG; // default
}

// --- Filename normalization ---
// Input convention:
//   "NAME"      — ASCII input, converted to uppercase PETSCII for disk lookup
//   @"NAME"     — explicit ASCII (same as default)
//   p"NAME"     — already PETSCII, use as-is (no conversion)
//
// For lookups: always compare uppercase (CBM filenames are case-insensitive)

static std::string normalizeName(const std::string& input) {
    std::string name = input;

    // Strip prefix if present
    bool isPetscii = false;
    if (name.size() >= 2 && (name[0] == 'p' || name[0] == 'P') && name[1] == '"') {
        // p"..." — already PETSCII
        name = name.substr(2);
        if (!name.empty() && name.back() == '"') name.pop_back();
        isPetscii = true;
    } else if (name.size() >= 2 && name[0] == '@' && name[1] == '"') {
        // @"..." — explicit ASCII
        name = name.substr(2);
        if (!name.empty() && name.back() == '"') name.pop_back();
    } else if (name.size() >= 2 && name[0] == '"') {
        // "..." — strip quotes
        name = name.substr(1);
        if (!name.empty() && name.back() == '"') name.pop_back();
    }

    if (!isPetscii) {
        // Convert to uppercase for CBM lookup
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    }
    return name;
}

// --- Subdirectory path resolution ---

// Split "SUBDIR/SUBDIR2/FILENAME" into path components
static std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) { parts.push_back(current); current.clear(); }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

// Find a CBM subdirectory entry starting from a given directory chain.
// Returns the firstDataTS of the subdirectory (which is its directory chain start).
static TrackSector findSubdir(DiskImage* img, TrackSector dirStart, const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    TrackSector ts = dirStart;
    while (ts.track != 0) {
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.type() == CbmFileType::CBM && e.name() == upper) {
                return e.firstDataTS;
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return {0, 0}; // not found
}

// Resolve a path like "SUBDIR/SUBDIR2" to the directory chain start T/S.
// Returns the root directory start if path is empty.
static TrackSector resolveDir(DiskImage* img, const std::vector<std::string>& pathParts) {
    // Root directory start
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    const uint8_t* hdr = img->sectorData(dirTrack, 0);
    if (!hdr) return {0, 0};
    TrackSector current = {hdr[0], hdr[1]};

    for (const auto& part : pathParts) {
        TrackSector sub = findSubdir(img, current, part);
        if (sub.isNull()) return {0, 0}; // path not found
        current = sub;
    }
    return current;
}

// List files starting from a specific directory chain
static std::vector<FileInfo> listFilesAt(DiskImage* img, TrackSector dirStart) {
    std::vector<FileInfo> files;
    TrackSector ts = dirStart;
    while (ts.track != 0) {
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid()) {
                FileInfo fi;
                fi.name = e.name();
                fi.type = e.type();
                fi.sizeInSectors = e.sizeInSectors;
                fi.sizeInBytes = e.sizeInSectors * 254;
                fi.locked = (e.fileType & 0x40) != 0;
                fi.closed = e.isClosed();
                files.push_back(fi);
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return files;
}

// Shrink a directory chain: remove trailing empty sectors and free them.
// Returns the number of sectors reclaimed.
static int shrinkDirChain(DiskImage* img, TrackSector dirStart) {
    // Collect all sectors in the chain
    std::vector<TrackSector> chain;
    TrackSector ts = dirStart;
    while (ts.track != 0) {
        chain.push_back(ts);
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    // Never shrink below 1 sector (the directory must have at least one)
    int reclaimed = 0;
    while (chain.size() > 1) {
        TrackSector last = chain.back();
        const uint8_t* sec = img->sectorData(last.track, last.sector);
        if (!sec) break;

        // Check if all 8 entries are empty
        bool allEmpty = true;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid()) { allEmpty = false; break; }
        }
        if (!allEmpty) break;

        // Unlink from previous sector
        chain.pop_back();
        TrackSector prev = chain.back();
        uint8_t* prevSec = img->sectorData(prev.track, prev.sector);
        if (prevSec) {
            prevSec[0] = 0;
            prevSec[1] = 0xFF;
        }

        // Free the sector
        img->freeSector(last.track, last.sector);
        reclaimed++;
    }
    return reclaimed;
}

// Ensure a directory chain has at least minFreeSlots empty entry slots.
// Allocates new sectors as needed. Works for any directory (root or sub).
// Returns number of sectors added, or -1 on error.
static int ensureDirCapacity(DiskImage* img, TrackSector dirStart, int minFreeSlots) {
    // Count existing free slots
    int freeSlots = 0;
    TrackSector lastTS = {0, 0};
    TrackSector ts = dirStart;
    while (ts.track != 0) {
        lastTS = ts;
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (!e.isValid()) freeSlots++;
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    // Allocate additional sectors if needed
    int added = 0;
    while (freeSlots < minFreeSlots) {
        TrackSector ext = img->allocateNextFree(-1);
        if (ext.isNull()) return -1; // disk full

        // Initialize new sector
        uint8_t* extSec = img->sectorData(ext.track, ext.sector);
        std::memset(extSec, 0, 256);
        extSec[0] = 0;
        extSec[1] = 0xFF;

        // Link from previous last sector
        uint8_t* prevSec = img->sectorData(lastTS.track, lastTS.sector);
        if (prevSec) {
            prevSec[0] = ext.track;
            prevSec[1] = ext.sector;
        }

        lastTS = ext;
        freeSlots += 8;
        added++;
    }
    return added;
}

// Get root directory start T/S for an image
static TrackSector getRootDirStart(DiskImage* img) {
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    const uint8_t* hdr = img->sectorData(dirTrack, 0);
    if (!hdr) return {0, 0};
    return {hdr[0], hdr[1]};
}

// --- Wildcard pattern matching (case-insensitive) ---
// Supports * (match any) and ? (match one character)
static bool matchWildcard(const std::string& pattern, const std::string& str) {
    std::string p = pattern, s = str;
    std::transform(p.begin(), p.end(), p.begin(), ::toupper);
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    size_t pi = 0, si = 0;
    size_t starP = std::string::npos, starS = 0;

    while (si < s.size()) {
        if (pi < p.size() && (p[pi] == '?' || p[pi] == s[si])) {
            pi++; si++;
        } else if (pi < p.size() && p[pi] == '*') {
            starP = pi++;
            starS = si;
        } else if (starP != std::string::npos) {
            pi = starP + 1;
            si = ++starS;
        } else {
            return false;
        }
    }
    while (pi < p.size() && p[pi] == '*') pi++;
    return pi == p.size();
}

// Filter file list by optional wildcard pattern
static std::vector<FileInfo> filterFiles(const std::vector<FileInfo>& files,
                                          const std::string& pattern) {
    if (pattern.empty() || pattern == "*") return files;
    std::vector<FileInfo> result;
    for (auto& f : files) {
        if (matchWildcard(pattern, f.name)) result.push_back(f);
    }
    return result;
}

// File type extension for host filenames
static std::string typeExtension(CbmFileType t) {
    switch (t) {
        case CbmFileType::PRG: return ".prg";
        case CbmFileType::SEQ: return ".seq";
        case CbmFileType::USR: return ".usr";
        case CbmFileType::REL: return ".rel";
        default: return ".del";
    }
}

// Sanitize CBM filename for host filesystem
static std::string sanitizeFilename(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (c >= 32 && c < 127 && c != '/' && c != '\\' && c != ':') result += c;
        else result += '_';
    }
    // Trim trailing spaces/underscores
    while (!result.empty() && (result.back() == ' ' || result.back() == '_'))
        result.pop_back();
    if (result.empty()) result = "unnamed";
    return result;
}

static std::string baseName(const std::string& path) {
    auto slash = path.rfind('/');
    std::string name = (slash != std::string::npos) ? path.substr(slash + 1) : path;
    auto dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    // Truncate to 16 chars
    if (name.size() > 16) name.resize(16);
    return name;
}

// ============================================================================
// Commands
// ============================================================================

static int cmdCreate(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string diskName, diskId;
    int tracks = 0; // 0 = use format default

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-n" && i + 1 < argc) diskName = argv[++i];
        else if (std::string(argv[i]) == "-i" && i + 1 < argc) diskId = argv[++i];
        else if (std::string(argv[i]) == "-t" && i + 1 < argc) tracks = std::stoi(argv[++i]);
    }

    auto img = DiskImage::createFromExtension(imagePath);
    img->format(diskName, diskId);

    // Extend image for non-default track count (e.g., D64 with 40 tracks)
    if (tracks > 0 && tracks != img->totalTracks()) {
        int defaultSize = (int)img->totalBytes();
        // Calculate new size by adding sectors for extra tracks
        int extraSectors = 0;
        for (int t = img->totalTracks() + 1; t <= tracks; t++)
            extraSectors += img->sectorsOnTrack(t);

        if (extraSectors > 0) {
            // Resize the image to include extra tracks
            int newSize = defaultSize + extraSectors * 256;
            // Access the raw image data via a temporary save/reload
            // Simpler: just pad the image directly
            // The image_ member is protected, so we save, resize, reload
            // Actually, format() sets the image size. For D64 extended tracks,
            // we need to resize after format().
            // Use saveToFile + manual resize approach:
            img->saveToFile(imagePath);

            // Read back raw, extend, write
            std::ifstream f(imagePath, std::ios::binary | std::ios::ate);
            auto sz = f.tellg();
            f.seekg(0);
            std::vector<uint8_t> raw((size_t)sz);
            f.read(reinterpret_cast<char*>(raw.data()), sz);
            f.close();
            raw.resize(newSize, 0);
            std::ofstream out(imagePath, std::ios::binary);
            out.write(reinterpret_cast<const char*>(raw.data()), raw.size());
            out.close();

            // Reload to pick up the new size
            img = DiskImage::load(imagePath);
        }
    }

    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Created " << formatStr(img->diskFormat()) << " image: "
              << imagePath << " (" << img->totalTracks() << " tracks, "
              << img->totalBytes() << " bytes)\n";
    return 0;
}

static int cmdList(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string pattern = (argc >= 2) ? argv[1] : "";

    // Check if pattern contains a path (SUBDIR/ or SUBDIR/pattern)
    auto pathParts = splitPath(pattern);
    std::vector<FileInfo> files;
    if (pathParts.size() > 1 || (!pattern.empty() && pattern.back() == '/')) {
        // Navigate to subdirectory
        std::vector<std::string> dirPath;
        std::string filePattern = "";
        if (!pattern.empty() && pattern.back() == '/') {
            // "SUBDIR/" — list all in subdir
            dirPath = pathParts;
        } else {
            // "SUBDIR/pattern" — last component is the filter
            dirPath.assign(pathParts.begin(), pathParts.end() - 1);
            filePattern = pathParts.back();
        }
        TrackSector dirTS = resolveDir(img.get(), dirPath);
        if (dirTS.isNull()) {
            std::cerr << "Error: directory not found\n";
            return 1;
        }
        files = filterFiles(listFilesAt(img.get(), dirTS), filePattern);
    } else {
        files = filterFiles(img->listFiles(), pattern);
    }

    std::cout << "0 \"" << img->diskName() << "\" " << img->diskId() << "\n";
    for (auto& f : files) {
        std::cout << f.sizeInSectors << "\t\"" << f.name << "\"\t";
        if (!f.closed) std::cout << "*"; // splat indicator
        std::cout << fileTypeStr(f.type);
        if (f.locked) std::cout << "<";
        std::cout << "\n";
    }
    std::cout << img->freeSectors() << " blocks free.\n";
    return 0;
}

static int cmdInfo(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    // TAP-specific info
    auto* tap = dynamic_cast<TapImage*>(img.get());
    auto* cvt = dynamic_cast<GeosCvtImage*>(img.get());
    if (tap) {
        std::cout << "Format:       TAP (tape image)\n"
                  << "TAP version:  " << tap->tapVersion() << "\n"
                  << "Platform:     " << tap->platformName() << "\n"
                  << "Data size:    " << (img->totalBytes()) << " bytes\n"
                  << "Files:        " << img->listFiles().size() << "\n";
    } else if (cvt) {
        std::cout << "Format:       GEOS CVT (Convert)\n"
                  << "GEOS name:    " << cvt->geosFileName() << "\n"
                  << "GEOS type:    " << cvt->geosFileTypeName() << "\n"
                  << "Structure:    " << (cvt->isVlir() ? "VLIR" : "Sequential") << "\n";
        if (cvt->isVlir())
            std::cout << "VLIR records: " << cvt->vlirRecordCount() << "\n";
        std::string desc = cvt->geosDescription();
        if (!desc.empty())
            std::cout << "Description:  " << desc << "\n";
        std::cout << "Files:        " << img->listFiles().size() << "\n";
    } else {
        std::cout << "Format:       " << formatStr(img->diskFormat()) << "\n"
                  << "Disk name:    " << img->diskName() << "\n"
                  << "Disk ID:      " << img->diskId() << "\n"
                  << "Tracks:       " << img->totalTracks() << "\n"
                  << "Total sectors:" << img->totalSectors() << "\n"
                  << "Free sectors: " << img->freeSectors() << "\n"
                  << "Image size:   " << img->totalBytes() << " bytes\n"
                  << "Files:        " << img->listFiles().size() << "\n";
    }
    return 0;
}

static int cmdAdd(int argc, char** argv) {
    bool petscii = extractPetsciiFlag(argc, argv);
    if (argc < 2) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string hostFile = argv[1];
    std::string cbmName = (argc >= 3) ? argv[2] : baseName(hostFile);

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    // Read host file
    std::ifstream f(hostFile, std::ios::binary | std::ios::ate);
    if (!f) { std::cerr << "Error: cannot open " << hostFile << "\n"; return 1; }
    auto size = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data((size_t)size);
    f.read(reinterpret_cast<char*>(data.data()), size);

    CbmFileType ftype = typeFromExtension(hostFile);

    // Convert ASCII → PETSCII for SEQ file content
    if (petscii && ftype == CbmFileType::SEQ) {
        data = asciiToPetsciiContent(data);
        std::cout << "(converted ASCII → PETSCII)\n";
    }

    if (!img->addFile(cbmName, ftype, data)) {
        std::cerr << "Error: failed to add \"" << cbmName << "\" (disk full or duplicate?)\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Added \"" << cbmName << "\" (" << data.size() << " bytes, "
              << fileTypeStr(ftype) << ")\n";
    return 0;
}

static int cmdExtract(int argc, char** argv) {
    bool petscii = extractPetsciiFlag(argc, argv);
    if (argc < 3) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string cbmName = argv[1];
    std::string hostFile = argv[2];

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    auto data = img->readFile(cbmName);
    if (data.empty() && !img->fileExists(cbmName)) {
        std::cerr << "Error: file \"" << cbmName << "\" not found\n";
        return 1;
    }

    // Convert PETSCII → ASCII for SEQ file content
    if (petscii) {
        data = petsciiToAsciiContent(data);
        std::cout << "(converted PETSCII → ASCII)\n";
    }

    std::ofstream f(hostFile, std::ios::binary);
    if (!f) { std::cerr << "Error: cannot write " << hostFile << "\n"; return 1; }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    std::cout << "Extracted \"" << cbmName << "\" (" << data.size() << " bytes)\n";
    return 0;
}

static int cmdRemove(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string cbmName = argv[1];

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (!img->removeFile(cbmName)) {
        std::cerr << "Error: file \"" << cbmName << "\" not found\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Removed \"" << cbmName << "\"\n";
    return 0;
}

// Generic rename: works for files AND CBM subdirectory entries
static int cmdRename(int argc, char** argv) {
    if (argc < 3) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string oldName = normalizeName(argv[1]);
    std::string newName = normalizeName(argv[2]);

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    // Try the standard renameFile first (handles regular files)
    if (img->renameFile(oldName, newName)) {
        if (!img->saveToFile(imagePath)) {
            std::cerr << "Error: failed to write " << imagePath << "\n";
            return 1;
        }
        std::cout << "Renamed \"" << oldName << "\" → \"" << newName << "\"\n";
        return 0;
    }

    // Fall back to raw directory scan (for CBM subdirectories and other types)
    if (img->totalTracks() == 0) {
        std::cerr << "Error: rename failed (entry not found)\n";
        return 1;
    }
    TrackSector rootDir = getRootDirStart(img.get());
    TrackSector ts = rootDir;
    bool renamed = false;
    while (ts.track != 0 && !renamed) {
        uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.name() == oldName) {
                DiskImage::padPetsciiName(e.filename, newName);
                e.toSector(sec, i);
                renamed = true;
                break;
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    if (!renamed) {
        std::cerr << "Error: rename failed (entry not found)\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Renamed \"" << oldName << "\" → \"" << newName << "\"\n";
    return 0;
}

// Toggle splat (unclosed) status on a file
static int cmdSplat(int argc, char** argv, bool close) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 " << (close ? "unsplat" : "splat") << " <image> <filename>\n";
        return 1;
    }
    std::string imagePath = argv[0];
    std::string name = normalizeName(argv[1]);

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (img->totalTracks() == 0) {
        std::cerr << "Error: splat/unsplat only works on sector-based disk images\n";
        return 1;
    }

    TrackSector rootDir = getRootDirStart(img.get());
    TrackSector ts = rootDir;
    bool found = false;
    while (ts.track != 0 && !found) {
        uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            // Match by name — check both valid and invalid (splat) entries
            if ((e.fileType & 0x07) != 0 && e.name() == name) {
                uint8_t* p = sec + i * 32;
                if (close) {
                    p[2] |= 0x80;  // set closed bit
                } else {
                    p[2] &= ~0x80; // clear closed bit (make splat)
                }
                found = true;
                break;
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    if (!found) {
        std::cerr << "Error: file \"" << name << "\" not found\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << (close ? "Unsplatted" : "Splatted") << " \"" << name << "\" ("
              << (close ? "closed" : "unclosed/splat") << ")\n";
    return 0;
}

static int cmdLock(int argc, char** argv, bool lock) {
    if (argc < 2) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string cbmName = argv[1];

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (!img->lockFile(cbmName, lock)) {
        std::cerr << "Error: file \"" << cbmName << "\" not found\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << (lock ? "Locked" : "Unlocked") << " \"" << cbmName << "\"\n";
    return 0;
}

static int cmdLabel(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string diskName, diskId;
    bool setName = false, setId = false;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-n" && i + 1 < argc) { diskName = argv[++i]; setName = true; }
        else if (std::string(argv[i]) == "-i" && i + 1 < argc) { diskId = argv[++i]; setId = true; }
    }

    if (!setName && !setId) {
        std::cerr << "Usage: disk45 label <image> [-n name] [-i id]\n";
        return 1;
    }

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (setName && !img->setDiskName(diskName)) {
        std::cerr << "Error: cannot set disk name (format may not support it)\n";
        return 1;
    }
    if (setId && !img->setDiskId(diskId)) {
        std::cerr << "Error: cannot set disk ID (format may not support it)\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Disk label updated";
    if (setName) std::cout << " name=\"" << diskName << "\"";
    if (setId) std::cout << " id=\"" << diskId << "\"";
    std::cout << "\n";
    return 0;
}

static int cmdValidate(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    auto r = img->validate();

    std::cout << "Disk: \"" << img->diskName() << "\" " << img->diskId() << "\n"
              << "Files found:           " << r.filesFound << "\n"
              << "Sectors used by files: " << r.sectorsUsedByFiles << "\n"
              << "Sectors marked used:   " << r.sectorsMarkedUsed << "\n"
              << "Free sectors (BAM):    " << r.freeSectorsInBAM << "\n";

    if (r.crossLinked)
        std::cout << "CROSS-LINKED sectors:  " << r.crossLinked << "\n";
    if (r.orphanedSectors)
        std::cout << "Orphaned sectors:      " << r.orphanedSectors << "\n";
    if (r.brokenChains)
        std::cout << "Broken chains:         " << r.brokenChains << "\n";

    if (r.errors.empty()) {
        std::cout << "Status: OK\n";
    } else {
        std::cout << "\nErrors:\n";
        for (auto& e : r.errors)
            std::cout << "  - " << e << "\n";
    }
    return r.ok() ? 0 : 1;
}

static int cmdBAM(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    if (img->totalTracks() == 0) {
        std::cerr << "Error: BAM map requires a disk image, not an archive\n";
        return 1;
    }

    std::cout << "BAM: \"" << img->diskName() << "\" " << img->diskId()
              << "  (" << img->freeSectors() << " blocks free)\n\n";

    // Header
    int maxSpt = 0;
    for (int t = 1; t <= img->totalTracks(); t++)
        maxSpt = std::max(maxSpt, img->sectorsOnTrack(t));

    std::cout << "Track  ";
    for (int s = 0; s < maxSpt; s++) {
        if (s % 10 == 0) std::cout << (s / 10);
        else std::cout << " ";
    }
    std::cout << "\n       ";
    for (int s = 0; s < maxSpt; s++)
        std::cout << (s % 10);
    std::cout << "\n";

    // Map
    for (int t = 1; t <= img->totalTracks(); t++) {
        int spt = img->sectorsOnTrack(t);
        std::cout << std::setw(4) << t << "   ";
        int free = 0;
        for (int s = 0; s < spt; s++) {
            bool f = img->isSectorFree(t, s);
            if (f) free++;
            std::cout << (f ? '.' : '#');
        }
        // Pad short tracks
        for (int s = spt; s < maxSpt; s++)
            std::cout << ' ';
        std::cout << "  " << std::setw(2) << free << "/" << spt << "\n";
    }

    std::cout << "\nLegend: . = free, # = allocated\n";
    return 0;
}

static void hexDumpSector(const uint8_t* data, int track, int sector) {
    std::cout << "Track " << track << ", Sector " << sector << ":\n";
    for (int row = 0; row < 16; row++) {
        std::cout << std::hex << std::uppercase << std::setfill('0');
        std::cout << std::setw(2) << (row * 16) << ": ";
        for (int col = 0; col < 16; col++) {
            std::cout << std::setw(2) << (int)data[row * 16 + col];
            if (col == 7) std::cout << "  "; else std::cout << " ";
        }
        std::cout << " |";
        for (int col = 0; col < 16; col++) {
            uint8_t c = data[row * 16 + col];
            std::cout << (char)((c >= 0x20 && c < 0x7F) ? c : '.');
        }
        std::cout << "|\n";
    }
    std::cout << std::dec;
}

static int cmdDump(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    std::string imagePath = argv[0];

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (img->totalTracks() == 0) {
        // Archive format — just raw hex dump
        std::cerr << "Error: dump requires a disk image (D64/D71/D81/D65), not an archive\n";
        return 1;
    }

    if (argc >= 3) {
        // Dump specific track/sector
        int track = std::atoi(argv[1]);
        int sector = std::atoi(argv[2]);
        const uint8_t* data = img->sectorData(track, sector);
        if (!data) {
            std::cerr << "Error: invalid track " << track << " sector " << sector << "\n";
            return 1;
        }
        hexDumpSector(data, track, sector);
    } else if (argc == 2) {
        // Dump all sectors on a track
        int track = std::atoi(argv[1]);
        int spt = img->sectorsOnTrack(track);
        if (spt == 0) {
            std::cerr << "Error: invalid track " << track << "\n";
            return 1;
        }
        for (int s = 0; s < spt; s++) {
            const uint8_t* data = img->sectorData(track, s);
            if (data) {
                hexDumpSector(data, track, s);
                std::cout << "\n";
            }
        }
    } else {
        // No track/sector: dump BAM/header (directory track, sector 0)
        int dirT = img->totalTracks() > 40 ? 40 : 18; // D81/D65 vs D64/D71
        const uint8_t* data = img->sectorData(dirT, 0);
        if (data) {
            std::cout << "BAM / Header:\n";
            hexDumpSector(data, dirT, 0);
        }
    }
    return 0;
}

// ============================================================================
// Copy between images
// ============================================================================

static int cmdCopy(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: disk45 copy <src_image> <pattern> <dst_image>\n";
        return 1;
    }
    std::string srcPath = argv[0];
    std::string pattern = argv[1];
    std::string dstPath = argv[2];

    auto src = DiskImage::load(srcPath);
    if (!src) { std::cerr << "Error: failed to load " << srcPath << "\n"; return 1; }
    auto dst = DiskImage::load(dstPath);
    if (!dst) { std::cerr << "Error: failed to load " << dstPath << "\n"; return 1; }

    auto files = filterFiles(src->listFiles(), pattern);
    if (files.empty()) {
        std::cerr << "No files matching \"" << pattern << "\" in " << srcPath << "\n";
        return 1;
    }

    int copied = 0;
    for (auto& f : files) {
        auto data = src->readFile(f.name);
        if (data.empty() && !src->fileExists(f.name)) {
            std::cerr << "Warning: could not read \"" << f.name << "\", skipping\n";
            continue;
        }
        if (dst->fileExists(f.name)) {
            std::cerr << "Warning: \"" << f.name << "\" already exists in destination, skipping\n";
            continue;
        }
        if (!dst->addFile(f.name, f.type, data)) {
            std::cerr << "Error: failed to add \"" << f.name << "\" to destination (disk full?)\n";
            continue;
        }
        std::cout << "  " << f.name << " (" << data.size() << " bytes, " << fileTypeStr(f.type) << ")\n";
        copied++;
    }

    if (copied > 0) {
        if (!dst->saveToFile(dstPath)) {
            std::cerr << "Error: failed to write " << dstPath << "\n";
            return 1;
        }
    }
    std::cout << "Copied " << copied << " file(s) from " << srcPath << " to " << dstPath << "\n";
    return 0;
}

// ============================================================================
// Extract all files
// ============================================================================

static int cmdExtractAll(int argc, char** argv) {
    bool petscii = extractPetsciiFlag(argc, argv);
    if (argc < 1) {
        std::cerr << "Usage: disk45 extract-all <image> [output_dir] [pattern]\n";
        return 1;
    }
    std::string imagePath = argv[0];
    std::string outDir = (argc >= 2) ? argv[1] : ".";
    std::string pattern = (argc >= 3) ? argv[2] : "*";

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    // Create output directory if needed
    std::string mkdirCmd = "mkdir -p '" + outDir + "'";
    if (system(mkdirCmd.c_str()) != 0) {
        std::cerr << "Error: cannot create directory " << outDir << "\n";
        return 1;
    }

    auto files = filterFiles(img->listFiles(), pattern);
    int extracted = 0;
    for (auto& f : files) {
        auto data = img->readFile(f.name);
        if (data.empty() && !img->fileExists(f.name)) {
            std::cerr << "Warning: could not read \"" << f.name << "\", skipping\n";
            continue;
        }

        if (petscii && f.type == CbmFileType::SEQ) {
            data = petsciiToAsciiContent(data);
        }

        std::string hostName = sanitizeFilename(f.name) + typeExtension(f.type);
        std::string fullPath = outDir + "/" + hostName;

        // Handle name collisions
        if (std::ifstream(fullPath).good()) {
            int suffix = 1;
            std::string base = sanitizeFilename(f.name);
            std::string ext = typeExtension(f.type);
            do {
                fullPath = outDir + "/" + base + "_" + std::to_string(suffix++) + ext;
            } while (std::ifstream(fullPath).good());
        }

        std::ofstream out(fullPath, std::ios::binary);
        if (!out) {
            std::cerr << "Error: cannot write " << fullPath << "\n";
            continue;
        }
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        std::cout << "  " << f.name << " → " << fullPath << " (" << data.size() << " bytes)\n";
        extracted++;
    }

    std::cout << "Extracted " << extracted << " file(s) from " << imagePath << "\n";
    return 0;
}

// ============================================================================
// Block-level operations
// ============================================================================

static int cmdBread(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: disk45 bread <image> <track> <sector> [output_file]\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int track = std::stoi(argv[1]);
    int sector = std::stoi(argv[2]);

    const uint8_t* data = img->sectorData(track, sector);
    if (!data) { std::cerr << "Error: invalid track/sector " << track << "/" << sector << "\n"; return 1; }

    if (argc >= 4) {
        std::ofstream f(argv[3], std::ios::binary);
        if (!f) { std::cerr << "Error: cannot write " << argv[3] << "\n"; return 1; }
        f.write(reinterpret_cast<const char*>(data), 256);
        std::cout << "Read sector " << track << "/" << sector << " → " << argv[3] << " (256 bytes)\n";
    } else {
        hexDumpSector(data, track, sector);
    }
    return 0;
}

static int cmdBwrite(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: disk45 bwrite <image> <track> <sector> <input_file>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int track = std::stoi(argv[1]);
    int sector = std::stoi(argv[2]);

    uint8_t* data = img->sectorData(track, sector);
    if (!data) { std::cerr << "Error: invalid track/sector " << track << "/" << sector << "\n"; return 1; }

    std::ifstream f(argv[3], std::ios::binary);
    if (!f) { std::cerr << "Error: cannot read " << argv[3] << "\n"; return 1; }
    f.read(reinterpret_cast<char*>(data), 256);

    if (!img->saveToFile(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
    std::cout << "Wrote sector " << track << "/" << sector << " from " << argv[3] << "\n";
    return 0;
}

static int cmdBpeek(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: disk45 bpeek <image> <track> <sector> <offset> [count]\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int track = std::stoi(argv[1]);
    int sector = std::stoi(argv[2]);
    int offset = std::stoi(argv[3]);
    int count = (argc >= 5) ? std::stoi(argv[4]) : 1;

    const uint8_t* data = img->sectorData(track, sector);
    if (!data) { std::cerr << "Error: invalid track/sector\n"; return 1; }
    if (offset < 0 || offset + count > 256) { std::cerr << "Error: offset out of range\n"; return 1; }

    std::cout << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < count; i++) {
        if (i > 0) std::cout << " ";
        std::cout << std::setw(2) << (int)data[offset + i];
    }
    std::cout << std::dec << "\n";
    return 0;
}

static int cmdBpoke(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: disk45 bpoke <image> <track> <sector> <offset> <value> [value...]\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int track = std::stoi(argv[1]);
    int sector = std::stoi(argv[2]);
    int offset = std::stoi(argv[3]);

    uint8_t* data = img->sectorData(track, sector);
    if (!data) { std::cerr << "Error: invalid track/sector\n"; return 1; }

    for (int i = 4; i < argc; i++) {
        if (offset >= 256) { std::cerr << "Error: offset out of range\n"; return 1; }
        data[offset++] = (uint8_t)std::stoul(argv[i], nullptr, 0);
    }

    if (!img->saveToFile(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
    std::cout << "Poked " << (argc - 4) << " byte(s) at " << track << "/" << sector << "+" << std::stoi(argv[3]) << "\n";
    return 0;
}

static int cmdBfill(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: disk45 bfill <image> <track> <sector> <value>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int track = std::stoi(argv[1]);
    int sector = std::stoi(argv[2]);
    uint8_t val = (uint8_t)std::stoul(argv[3], nullptr, 0);

    uint8_t* data = img->sectorData(track, sector);
    if (!data) { std::cerr << "Error: invalid track/sector\n"; return 1; }

    std::memset(data, val, 256);
    if (!img->saveToFile(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
    std::cout << "Filled sector " << track << "/" << sector << " with $"
              << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)val
              << std::dec << "\n";
    return 0;
}

// ============================================================================
// Sector chain trace
// ============================================================================

static int cmdChain(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 chain <image> <filename>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    if (img->totalTracks() == 0) {
        std::cerr << "Error: chain command only works on sector-based disk images\n";
        return 1;
    }

    std::string name = normalizeName(argv[1]);

    // Find the file's first track/sector via directory scan
    // We need to walk the directory to find the file entry
    auto files = img->listFiles();
    bool found = false;
    for (auto& f : files) {
        if (f.name != name) continue;
        found = true;
        break;
    }
    if (!found) {
        std::cerr << "Error: file \"" << name << "\" not found\n";
        return 1;
    }

    // Walk directory to find the DirEntry with firstDataTS
    // Use the raw directory traversal approach
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    const uint8_t* bam = img->sectorData(dirTrack, 0);
    if (!bam) { std::cerr << "Error: cannot read directory\n"; return 1; }

    TrackSector firstTS = {0, 0};
    TrackSector ts = {bam[0], bam[1]};
    while (ts.track != 0) {
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.isClosed() && e.name() == name) {
                firstTS = e.firstDataTS;
                break;
            }
        }
        if (!firstTS.isNull()) break;
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    if (firstTS.isNull()) {
        std::cerr << "Error: could not find sector chain for \"" << name << "\"\n";
        return 1;
    }

    // Follow the chain
    std::cout << "File \"" << name << "\" chain:\n  ";
    int count = 0;
    uint32_t totalBytes = 0;
    ts = firstTS;
    while (ts.track != 0) {
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) { std::cout << " → [BROKEN]"; break; }
        if (count > 0) std::cout << " → ";
        std::cout << (int)ts.track << "/" << (int)ts.sector;
        count++;

        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) {
            // Last sector: nextS = number of bytes used
            totalBytes += nextS ? nextS - 1 : 255;
        } else {
            totalBytes += 254;
        }

        if (count % 10 == 0) std::cout << "\n  ";
        ts = {nextT, nextS};
    }
    std::cout << "\n  " << count << " sector(s), " << totalBytes << " bytes\n";
    return 0;
}

// ============================================================================
// REL file operations
// ============================================================================

// Side sector layout (D64/D71):
//   $00: Track of next side sector (0 if last)
//   $01: Sector of next side sector
//   $02: Side sector number (0-based)
//   $03: Record length
//   $04-$0F: T/S pairs for side sector group (up to 6 side sectors)
//   $10-$FF: 120 T/S pairs pointing to data sectors

// Get the list of data sector T/S pairs from the side-sector chain
static std::vector<TrackSector> getRelDataSectors(DiskImage* img, TrackSector sideTS) {
    std::vector<TrackSector> sectors;
    TrackSector ss = sideTS;
    while (ss.track != 0) {
        const uint8_t* sec = img->sectorData(ss.track, ss.sector);
        if (!sec) break;
        // Data sector T/S pairs at offsets $10-$FF (120 pairs)
        for (int i = 0x10; i < 0x100; i += 2) {
            if (sec[i] == 0 && sec[i + 1] == 0) break;
            sectors.push_back({sec[i], sec[i + 1]});
        }
        ss = {sec[0], sec[1]};
    }
    return sectors;
}

static int cmdRelCreate(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: disk45 rel-create <image> <name> <record_len> [num_records]\n";
        return 1;
    }
    std::string imagePath = argv[0];
    std::string name = normalizeName(argv[1]);
    int recordLen = std::stoi(argv[2]);
    int numRecords = (argc >= 4) ? std::stoi(argv[3]) : 10;

    if (recordLen < 1 || recordLen > 254) {
        std::cerr << "Error: record length must be 1-254\n";
        return 1;
    }

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }
    if (img->totalTracks() == 0) {
        std::cerr << "Error: REL files only supported on sector-based disk images\n";
        return 1;
    }
    if (img->fileExists(name)) {
        std::cerr << "Error: file \"" << name << "\" already exists\n";
        return 1;
    }

    // Calculate how many data sectors we need
    int bytesPerSector = 254; // usable bytes per data sector (minus 2-byte link)
    int recordsPerSector = bytesPerSector / recordLen;
    if (recordsPerSector < 1) recordsPerSector = 1;
    int dataSectors = (numRecords + recordsPerSector - 1) / recordsPerSector;

    // Allocate data sectors
    std::vector<TrackSector> dataChain;
    for (int i = 0; i < dataSectors; i++) {
        TrackSector ts = img->allocateNextFree(-1);
        if (ts.isNull()) { std::cerr << "Error: disk full\n"; return 1; }
        dataChain.push_back(ts);
    }

    // Initialize data sectors (linked chain, zeroed data)
    for (int i = 0; i < dataSectors; i++) {
        uint8_t* sec = img->sectorData(dataChain[i].track, dataChain[i].sector);
        std::memset(sec, 0, 256);
        if (i < dataSectors - 1) {
            sec[0] = dataChain[i + 1].track;
            sec[1] = dataChain[i + 1].sector;
        } else {
            sec[0] = 0;
            // Last used byte offset: depends on records that fit
            int recsInLast = numRecords - recordsPerSector * (dataSectors - 1);
            int lastByte = recsInLast * recordLen + 1;
            if (lastByte > 255) lastByte = 255;
            sec[1] = lastByte;
        }
    }

    // Allocate side sector(s)
    // Each side sector holds 120 data sector T/S pairs (at offsets $10-$FF)
    int sideSectorsNeeded = (dataSectors + 119) / 120;
    std::vector<TrackSector> sideChain;
    for (int i = 0; i < sideSectorsNeeded; i++) {
        TrackSector ts = img->allocateNextFree(-1);
        if (ts.isNull()) { std::cerr << "Error: disk full\n"; return 1; }
        sideChain.push_back(ts);
    }

    // Initialize side sectors
    int dataIdx = 0;
    for (int si = 0; si < sideSectorsNeeded; si++) {
        uint8_t* sec = img->sectorData(sideChain[si].track, sideChain[si].sector);
        std::memset(sec, 0, 256);
        // Link to next side sector
        if (si < sideSectorsNeeded - 1) {
            sec[0] = sideChain[si + 1].track;
            sec[1] = sideChain[si + 1].sector;
        } else {
            sec[0] = 0; sec[1] = 0;
        }
        sec[2] = si;           // side sector number
        sec[3] = recordLen;    // record length
        // Side sector group T/S pairs at $04-$0F (up to 6)
        for (int g = 0; g < sideSectorsNeeded && g < 6; g++) {
            sec[0x04 + g * 2] = sideChain[g].track;
            sec[0x05 + g * 2] = sideChain[g].sector;
        }
        // Data sector T/S pairs at $10-$FF
        for (int i = 0; i < 120 && dataIdx < dataSectors; i++, dataIdx++) {
            sec[0x10 + i * 2] = dataChain[dataIdx].track;
            sec[0x10 + i * 2 + 1] = dataChain[dataIdx].sector;
        }
    }

    // Create directory entry
    DirEntry entry;
    std::memset(&entry, 0, sizeof(entry));
    entry.fileType = 0x80 | (uint8_t)CbmFileType::REL;
    entry.firstDataTS = dataChain[0];
    entry.sideTS = sideChain[0];
    entry.recordLen = recordLen;
    entry.sizeInSectors = dataSectors + sideSectorsNeeded;
    DiskImage::padPetsciiName(entry.filename, name);

    // Use the protected allocDirectoryEntry - we need to go through addFile or manual dir write
    // Manual directory write: find free slot
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    TrackSector dts = {(uint8_t)dirTrack, 1};
    bool written = false;
    while (dts.track != 0 && !written) {
        uint8_t* sec = img->sectorData(dts.track, dts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (!e.isValid()) {
                entry.toSector(sec, i);
                written = true;
                break;
            }
        }
        if (!written) {
            uint8_t nextT = sec[0], nextS = sec[1];
            if (nextT == 0) break;
            dts = {nextT, nextS};
        }
    }

    if (!written) { std::cerr << "Error: directory full\n"; return 1; }

    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Created REL file \"" << name << "\" (record length=" << recordLen
              << ", " << numRecords << " records, " << dataSectors << " data sectors)\n";
    return 0;
}

static int cmdRelRead(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: disk45 rel-read <image> <name> <record_num>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string name = normalizeName(argv[1]);
    int recNum = std::stoi(argv[2]); // 1-based

    // Find the REL file
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    DirEntry found;
    bool ok = false;
    TrackSector dts = {(uint8_t)dirTrack, 1};
    while (dts.track != 0) {
        const uint8_t* sec = img->sectorData(dts.track, dts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.type() == CbmFileType::REL && e.name() == name) {
                found = e; ok = true; break;
            }
        }
        if (ok) break;
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        dts = {nextT, nextS};
    }
    if (!ok) { std::cerr << "Error: REL file \"" << name << "\" not found\n"; return 1; }

    int recordLen = found.recordLen;
    if (recordLen < 1) { std::cerr << "Error: invalid record length\n"; return 1; }

    // Get data sectors via side-sector chain
    auto dataSectors = getRelDataSectors(img.get(), found.sideTS);

    // Calculate which sector and offset the record is in
    int byteOffset = (recNum - 1) * recordLen;
    int sectorIdx = byteOffset / 254;
    int offsetInSector = (byteOffset % 254) + 2; // +2 for link bytes

    if (sectorIdx >= (int)dataSectors.size()) {
        std::cerr << "Error: record " << recNum << " out of range\n";
        return 1;
    }

    const uint8_t* sec = img->sectorData(dataSectors[sectorIdx].track,
                                          dataSectors[sectorIdx].sector);
    if (!sec) { std::cerr << "Error: cannot read data sector\n"; return 1; }

    // Read record (may span sector boundary)
    std::vector<uint8_t> record;
    int remaining = recordLen;
    int si = sectorIdx, off = offsetInSector;
    while (remaining > 0 && si < (int)dataSectors.size()) {
        sec = img->sectorData(dataSectors[si].track, dataSectors[si].sector);
        if (!sec) break;
        int avail = 256 - off;
        int take = std::min(remaining, avail);
        record.insert(record.end(), sec + off, sec + off + take);
        remaining -= take;
        si++;
        off = 2; // next sector starts after link
    }

    // Output
    std::cout << "Record " << recNum << " (" << recordLen << " bytes): ";
    std::cout << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t b : record) std::cout << std::setw(2) << (int)b << " ";
    std::cout << std::dec << "\n";

    // Also show as text (printable chars)
    std::cout << "  Text: \"";
    for (uint8_t b : record) {
        if (b >= 0x20 && b < 0x7F) std::cout << (char)b;
        else if (b >= 0x41 && b <= 0x5A) std::cout << (char)b;
        else if (b >= 0xC1 && b <= 0xDA) std::cout << (char)(b - 0x60);
        else if (b == 0) std::cout << '.';
        else std::cout << '.';
    }
    std::cout << "\"\n";
    return 0;
}

static int cmdRelWrite(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: disk45 rel-write <image> <name> <record_num> <data...>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string name = normalizeName(argv[1]);
    int recNum = std::stoi(argv[2]);

    // Collect data bytes from remaining args (hex or decimal)
    std::vector<uint8_t> data;
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        // If it looks like a quoted string, use the bytes directly
        if (arg.size() >= 2 && arg[0] == '"' && arg.back() == '"') {
            for (size_t j = 1; j < arg.size() - 1; j++)
                data.push_back((uint8_t)arg[j]);
        } else {
            // Treat as numeric value
            data.push_back((uint8_t)std::stoul(arg, nullptr, 0));
        }
    }

    // Find the REL file
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    DirEntry found;
    bool ok = false;
    TrackSector dts = {(uint8_t)dirTrack, 1};
    while (dts.track != 0) {
        const uint8_t* sec = img->sectorData(dts.track, dts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.type() == CbmFileType::REL && e.name() == name) {
                found = e; ok = true; break;
            }
        }
        if (ok) break;
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        dts = {nextT, nextS};
    }
    if (!ok) { std::cerr << "Error: REL file \"" << name << "\" not found\n"; return 1; }

    int recordLen = found.recordLen;
    if (recordLen < 1) { std::cerr << "Error: invalid record length\n"; return 1; }

    // Pad or truncate data to record length
    data.resize(recordLen, 0);

    auto dataSectors = getRelDataSectors(img.get(), found.sideTS);

    int byteOffset = (recNum - 1) * recordLen;
    int sectorIdx = byteOffset / 254;
    int offsetInSector = (byteOffset % 254) + 2;

    if (sectorIdx >= (int)dataSectors.size()) {
        std::cerr << "Error: record " << recNum << " out of range\n";
        return 1;
    }

    // Write record (may span sector boundary)
    int remaining = recordLen;
    int si = sectorIdx, off = offsetInSector, di = 0;
    while (remaining > 0 && si < (int)dataSectors.size()) {
        uint8_t* sec = img->sectorData(dataSectors[si].track, dataSectors[si].sector);
        if (!sec) break;
        int avail = 256 - off;
        int put = std::min(remaining, avail);
        std::memcpy(sec + off, data.data() + di, put);
        remaining -= put;
        di += put;
        si++;
        off = 2;
    }

    if (!img->saveToFile(argv[0])) {
        std::cerr << "Error: failed to write " << argv[0] << "\n";
        return 1;
    }
    std::cout << "Wrote record " << recNum << " (" << recordLen << " bytes)\n";
    return 0;
}

static int cmdRelList(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 rel-list <image> <name>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string name = normalizeName(argv[1]);

    // Find the REL file
    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    DirEntry found;
    bool ok = false;
    TrackSector dts = {(uint8_t)dirTrack, 1};
    while (dts.track != 0) {
        const uint8_t* sec = img->sectorData(dts.track, dts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.type() == CbmFileType::REL && e.name() == name) {
                found = e; ok = true; break;
            }
        }
        if (ok) break;
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        dts = {nextT, nextS};
    }
    if (!ok) { std::cerr << "Error: REL file \"" << name << "\" not found\n"; return 1; }

    int recordLen = found.recordLen;
    auto dataSectors = getRelDataSectors(img.get(), found.sideTS);

    // Calculate total records
    int totalBytes = 0;
    for (size_t i = 0; i < dataSectors.size(); i++) {
        const uint8_t* sec = img->sectorData(dataSectors[i].track, dataSectors[i].sector);
        if (!sec) break;
        if (i == dataSectors.size() - 1) {
            totalBytes += (sec[1] >= 2) ? sec[1] - 1 : 254;
        } else {
            totalBytes += 254;
        }
    }
    int totalRecords = totalBytes / recordLen;

    std::cout << "REL file \"" << name << "\": " << totalRecords << " records, "
              << recordLen << " bytes/record, " << dataSectors.size() << " data sectors\n";

    // Show non-empty records
    for (int r = 1; r <= totalRecords; r++) {
        int byteOff = (r - 1) * recordLen;
        int si = byteOff / 254;
        int off = (byteOff % 254) + 2;
        if (si >= (int)dataSectors.size()) break;

        const uint8_t* sec = img->sectorData(dataSectors[si].track, dataSectors[si].sector);
        if (!sec) break;

        // Check if record is all zeros (empty)
        bool empty = true;
        for (int b = 0; b < recordLen && empty; b++) {
            int co = off + b;
            int cs = si;
            if (co >= 256) { cs++; co = 2 + (co - 256); }
            if (cs >= (int)dataSectors.size()) break;
            const uint8_t* s2 = img->sectorData(dataSectors[cs].track, dataSectors[cs].sector);
            if (s2 && s2[co] != 0) empty = false;
        }
        if (empty) continue;

        std::cout << "  #" << std::setw(3) << r << ": ";
        for (int b = 0; b < recordLen; b++) {
            int co = off + b;
            int cs = si;
            if (co >= 256) { cs++; co = 2 + (co - 256); }
            if (cs >= (int)dataSectors.size()) break;
            const uint8_t* s2 = img->sectorData(dataSectors[cs].track, dataSectors[cs].sector);
            uint8_t val = s2 ? s2[co] : 0;
            if (val >= 0x20 && val < 0x7F) std::cout << (char)val;
            else if (val >= 0x41 && val <= 0x5A) std::cout << (char)val;
            else if (val >= 0xC1 && val <= 0xDA) std::cout << (char)(val - 0x60);
            else if (val == 0) std::cout << '.';
            else std::cout << '.';
        }
        std::cout << "\n";
    }
    return 0;
}

// ============================================================================
// Subdirectory operations
// ============================================================================

static int cmdMkdir(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 mkdir <image> <dirname>\n"
                  << "       disk45 mkdir <image> PARENT/NEWDIR\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    if (img->totalTracks() == 0) {
        std::cerr << "Error: subdirectories only supported on sector-based images\n";
        return 1;
    }

    std::string path = normalizeName(argv[1]);
    auto parts = splitPath(path);
    if (parts.empty()) { std::cerr << "Error: empty directory name\n"; return 1; }

    // Resolve parent directory
    std::vector<std::string> parentPath(parts.begin(), parts.end() - 1);
    std::string newName = parts.back();

    TrackSector parentDir;
    if (parentPath.empty()) {
        int dirTrack = img->totalTracks() > 40 ? 40 : 18;
        const uint8_t* hdr = img->sectorData(dirTrack, 0);
        if (!hdr) { std::cerr << "Error: cannot read directory\n"; return 1; }
        parentDir = {hdr[0], hdr[1]};
    } else {
        parentDir = resolveDir(img.get(), parentPath);
        if (parentDir.isNull()) { std::cerr << "Error: parent directory not found\n"; return 1; }
    }

    // Check if name already exists
    TrackSector existing = findSubdir(img.get(), parentDir, newName);
    if (!existing.isNull()) {
        std::cerr << "Error: \"" << newName << "\" already exists\n";
        return 1;
    }

    // Allocate a sector for the new subdirectory
    TrackSector newDirTS = img->allocateNextFree(-1);
    if (newDirTS.isNull()) { std::cerr << "Error: disk full\n"; return 1; }

    // Initialize the subdirectory sector (empty directory)
    uint8_t* newSec = img->sectorData(newDirTS.track, newDirTS.sector);
    std::memset(newSec, 0, 256);
    newSec[0] = 0;      // no next directory sector
    newSec[1] = 0xFF;   // end marker

    // Create a CBM directory entry in the parent
    TrackSector ts = parentDir;
    bool written = false;
    while (ts.track != 0 && !written) {
        uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (!e.isValid()) {
                // Found free slot — write the CBM entry
                DirEntry newEntry;
                std::memset(&newEntry, 0, sizeof(newEntry));
                newEntry.fileType = 0x80 | (uint8_t)CbmFileType::CBM;
                newEntry.firstDataTS = newDirTS;
                newEntry.sizeInSectors = 1;
                DiskImage::padPetsciiName(newEntry.filename, newName);
                newEntry.toSector(sec, i);
                written = true;
                break;
            }
        }
        if (!written) {
            uint8_t nextT = sec[0], nextS = sec[1];
            if (nextT == 0) {
                // Extend directory: allocate new sector
                TrackSector ext = img->allocateNextFree(-1);
                if (ext.isNull()) { std::cerr << "Error: disk full\n"; return 1; }
                sec[0] = ext.track; sec[1] = ext.sector;
                uint8_t* extSec = img->sectorData(ext.track, ext.sector);
                std::memset(extSec, 0, 256);
                extSec[0] = 0; extSec[1] = 0xFF;
                ts = ext;
            } else {
                ts = {nextT, nextS};
            }
        }
    }

    if (!written) { std::cerr << "Error: failed to create directory entry\n"; return 1; }

    if (!img->saveToFile(argv[0])) {
        std::cerr << "Error: failed to write " << argv[0] << "\n";
        return 1;
    }
    std::cout << "Created directory \"" << newName << "\"\n";
    return 0;
}

static int cmdRmdir(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 rmdir <image> <dirname>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string path = normalizeName(argv[1]);
    auto parts = splitPath(path);
    if (parts.empty()) { std::cerr << "Error: empty directory name\n"; return 1; }

    // Resolve parent
    std::vector<std::string> parentPath(parts.begin(), parts.end() - 1);
    std::string dirName = parts.back();

    TrackSector parentDir;
    if (parentPath.empty()) {
        int dirTrack = img->totalTracks() > 40 ? 40 : 18;
        const uint8_t* hdr = img->sectorData(dirTrack, 0);
        if (!hdr) { std::cerr << "Error: cannot read directory\n"; return 1; }
        parentDir = {hdr[0], hdr[1]};
    } else {
        parentDir = resolveDir(img.get(), parentPath);
        if (parentDir.isNull()) { std::cerr << "Error: parent not found\n"; return 1; }
    }

    // Find the subdirectory entry
    std::string upper = dirName;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    TrackSector ts = parentDir;
    bool removed = false;
    while (ts.track != 0 && !removed) {
        uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && e.type() == CbmFileType::CBM && e.name() == upper) {
                // Check if empty
                auto contents = listFilesAt(img.get(), e.firstDataTS);
                if (!contents.empty()) {
                    std::cerr << "Error: directory \"" << dirName << "\" is not empty ("
                              << contents.size() << " files)\n";
                    return 1;
                }
                // Free all subdirectory sectors
                TrackSector sub = e.firstDataTS;
                while (sub.track != 0) {
                    const uint8_t* ss = img->sectorData(sub.track, sub.sector);
                    TrackSector next = {0, 0};
                    if (ss) { next.track = ss[0]; next.sector = ss[1]; }
                    img->freeSector(sub.track, sub.sector);
                    if (next.track == 0) break;
                    sub = next;
                }
                // Clear the directory entry
                uint8_t* p = sec + i * 32;
                p[2] = 0; // clear file type
                removed = true;
                break;
            }
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }

    if (!removed) { std::cerr << "Error: directory \"" << dirName << "\" not found\n"; return 1; }

    // Shrink the parent directory chain (remove trailing empty sectors)
    int reclaimed = shrinkDirChain(img.get(), parentDir);

    if (!img->saveToFile(argv[0])) {
        std::cerr << "Error: failed to write " << argv[0] << "\n";
        return 1;
    }
    std::cout << "Removed directory \"" << dirName << "\"";
    if (reclaimed > 0) std::cout << " (" << reclaimed << " dir sector(s) reclaimed)";
    std::cout << "\n";
    return 0;
}

static void printTree(DiskImage* img, TrackSector dirStart, int depth) {
    auto files = listFilesAt(img, dirStart);
    for (auto& f : files) {
        for (int i = 0; i < depth; i++) std::cout << "  ";
        std::cout << f.name;
        if (f.type == CbmFileType::CBM) {
            std::cout << "/\n";
            TrackSector sub = findSubdir(img, dirStart, f.name);
            if (!sub.isNull()) printTree(img, sub, depth + 1);
        } else {
            std::cout << " (" << fileTypeStr(f.type) << ", " << f.sizeInSectors << " blocks)\n";
        }
    }
}

static int cmdTree(int argc, char** argv) {
    if (argc < 1) {
        std::cerr << "Usage: disk45 tree <image>\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::cout << img->diskName() << "/\n";

    int dirTrack = img->totalTracks() > 40 ? 40 : 18;
    const uint8_t* hdr = img->sectorData(dirTrack, 0);
    if (!hdr) { std::cerr << "Error: cannot read directory\n"; return 1; }
    TrackSector rootDir = {hdr[0], hdr[1]};

    printTree(img.get(), rootDir, 1);
    return 0;
}

static int cmdDirShrink(int argc, char** argv) {
    if (argc < 1) {
        std::cerr << "Usage: disk45 dir-shrink <image> [path]\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    TrackSector dirTS;
    std::string pathStr = (argc >= 2) ? normalizeName(argv[1]) : "";
    if (pathStr.empty()) {
        dirTS = getRootDirStart(img.get());
    } else {
        dirTS = resolveDir(img.get(), splitPath(pathStr));
    }
    if (dirTS.isNull()) { std::cerr << "Error: directory not found\n"; return 1; }

    int reclaimed = shrinkDirChain(img.get(), dirTS);
    if (reclaimed > 0) {
        if (!img->saveToFile(argv[0])) {
            std::cerr << "Error: failed to write " << argv[0] << "\n";
            return 1;
        }
        std::cout << "Reclaimed " << reclaimed << " empty directory sector(s)\n";
    } else {
        std::cout << "No empty trailing sectors to reclaim\n";
    }
    return 0;
}

static int cmdDirEnsure(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 dir-ensure <image> <min_free_slots> [path]\n"
                  << "  Ensures the directory has at least <min_free_slots> empty entry slots.\n"
                  << "  Allocates new directory sectors as needed (8 slots per sector).\n";
        return 1;
    }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    int minFree = std::stoi(argv[1]);
    std::string pathStr = (argc >= 3) ? normalizeName(argv[2]) : "";

    TrackSector dirTS;
    if (pathStr.empty()) {
        dirTS = getRootDirStart(img.get());
    } else {
        dirTS = resolveDir(img.get(), splitPath(pathStr));
    }
    if (dirTS.isNull()) { std::cerr << "Error: directory not found\n"; return 1; }

    int added = ensureDirCapacity(img.get(), dirTS, minFree);
    if (added < 0) {
        std::cerr << "Error: disk full, cannot allocate directory sectors\n";
        return 1;
    }
    if (added > 0) {
        if (!img->saveToFile(argv[0])) {
            std::cerr << "Error: failed to write " << argv[0] << "\n";
            return 1;
        }
        std::cout << "Added " << added << " directory sector(s) (" << (added * 8) << " slots)\n";
    } else {
        std::cout << "Directory already has sufficient free slots\n";
    }
    return 0;
}

// ============================================================================
// Format conversion
// ============================================================================

static bool isSingleFileFormat(const std::string& path) {
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
    return ext == ".prg" || ext == ".seq" || ext == ".usr" || ext == ".cvt";
}

static int cmdConvert(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: disk45 convert <source> <target>\n"
                  << "  Converts between disk/tape/archive formats, copying all files.\n"
                  << "  For single-file sources (.prg/.cvt), creates a blank target image.\n";
        return 1;
    }
    std::string srcPath = argv[0];
    std::string dstPath = argv[1];

    if (isSingleFileFormat(srcPath)) {
        // Single file → disk image: create blank target, add the file
        auto dst = DiskImage::createFromExtension(dstPath);
        dst->format("CONVERTED", "CV");

        // Read the source file
        std::ifstream f(srcPath, std::ios::binary | std::ios::ate);
        if (!f) { std::cerr << "Error: cannot read " << srcPath << "\n"; return 1; }
        auto size = f.tellg();
        f.seekg(0);
        std::vector<uint8_t> data((size_t)size);
        f.read(reinterpret_cast<char*>(data.data()), size);

        std::string cbmName = baseName(srcPath);
        std::transform(cbmName.begin(), cbmName.end(), cbmName.begin(), ::toupper);
        CbmFileType ftype = typeFromExtension(srcPath);

        if (!dst->addFile(cbmName, ftype, data)) {
            std::cerr << "Error: failed to add file to target image\n";
            return 1;
        }
        if (!dst->saveToFile(dstPath)) {
            std::cerr << "Error: failed to write " << dstPath << "\n";
            return 1;
        }
        std::cout << "Converted " << srcPath << " → " << dstPath
                  << " (1 file added)\n";
        return 0;
    }

    // Disk/tape/archive → disk/tape/archive: copy all files
    auto src = DiskImage::load(srcPath);
    if (!src) { std::cerr << "Error: failed to load " << srcPath << "\n"; return 1; }

    auto dst = DiskImage::createFromExtension(dstPath);
    std::string name = src->diskName();
    std::string id = src->diskId();
    dst->format(name, id);

    auto files = src->listFiles();
    int copied = 0;
    for (auto& f : files) {
        if (f.type == CbmFileType::CBM) continue; // skip subdirectory entries
        auto data = src->readFile(f.name);
        if (data.empty() && !src->fileExists(f.name)) continue;
        if (!dst->addFile(f.name, f.type, data)) {
            std::cerr << "Warning: failed to add \"" << f.name << "\" (disk full?)\n";
            continue;
        }
        copied++;
    }

    if (!dst->saveToFile(dstPath)) {
        std::cerr << "Error: failed to write " << dstPath << "\n";
        return 1;
    }
    std::cout << "Converted " << srcPath << " → " << dstPath
              << " (" << copied << " file(s) copied)\n";
    return 0;
}

// ============================================================================
// Main
// ============================================================================

// Process a single disk45 command line (for batch mode)
static int processCommand(int argc, char** argv);

// Batch script: read commands from file, execute each line
static int cmdBatch(const std::string& scriptFile) {
    std::ifstream f(scriptFile);
    if (!f) { std::cerr << "Error: cannot open script " << scriptFile << "\n"; return 1; }

    std::string line;
    int lineNum = 0, errors = 0;
    while (std::getline(f, line)) {
        lineNum++;
        // Skip empty lines and comments
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#' || line[start] == ';') continue;

        // Parse line into argv-style tokens (simple space splitting, respecting quotes)
        std::vector<std::string> tokens;
        std::string current;
        bool inQuote = false;
        for (size_t i = start; i < line.size(); i++) {
            char c = line[i];
            if (c == '"') { inQuote = !inQuote; continue; }
            if (c == ' ' && !inQuote) {
                if (!current.empty()) { tokens.push_back(current); current.clear(); }
            } else {
                current += c;
            }
        }
        if (!current.empty()) tokens.push_back(current);
        if (tokens.empty()) continue;

        // Build argv (prepend "disk45" as argv[0])
        std::vector<char*> args;
        std::string prog = "disk45";
        args.push_back(const_cast<char*>(prog.c_str()));
        for (auto& t : tokens) args.push_back(const_cast<char*>(t.c_str()));

        int rc = processCommand((int)args.size(), args.data());
        if (rc != 0) {
            std::cerr << "  (script line " << lineNum << " failed)\n";
            errors++;
        }
    }
    return errors > 0 ? 1 : 0;
}

static int processCommand(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    std::string cmd = argv[1];
    int subArgc = argc - 2;
    char** subArgv = argv + 2;

    // Stdin→stdout character encoding filters
    if (cmd == "-a2p" || cmd == "--ascii-to-petscii") {
        std::vector<uint8_t> input((std::istreambuf_iterator<char>(std::cin)),
                                    std::istreambuf_iterator<char>());
        auto output = asciiToPetsciiContent(input);
        std::cout.write(reinterpret_cast<const char*>(output.data()), output.size());
        return 0;
    }
    if (cmd == "-p2a" || cmd == "--petscii-to-ascii") {
        std::vector<uint8_t> input((std::istreambuf_iterator<char>(std::cin)),
                                    std::istreambuf_iterator<char>());
        auto output = petsciiToAsciiContent(input);
        std::cout.write(reinterpret_cast<const char*>(output.data()), output.size());
        return 0;
    }

    if (cmd == "create")  return cmdCreate(subArgc, subArgv);
    if (cmd == "list" || cmd == "ls" || cmd == "dir")
                          return cmdList(subArgc, subArgv);
    if (cmd == "info")    return cmdInfo(subArgc, subArgv);
    if (cmd == "add" || cmd == "write")
                          return cmdAdd(subArgc, subArgv);
    if (cmd == "extract" || cmd == "read")
                          return cmdExtract(subArgc, subArgv);
    if (cmd == "extract-all" || cmd == "extractall")
                          return cmdExtractAll(subArgc, subArgv);
    if (cmd == "copy" || cmd == "cp")
                          return cmdCopy(subArgc, subArgv);
    if (cmd == "remove" || cmd == "delete" || cmd == "rm")
                          return cmdRemove(subArgc, subArgv);
    if (cmd == "rename" || cmd == "mv")
                          return cmdRename(subArgc, subArgv);
    if (cmd == "lock")    return cmdLock(subArgc, subArgv, true);
    if (cmd == "unlock")  return cmdLock(subArgc, subArgv, false);
    if (cmd == "splat")   return cmdSplat(subArgc, subArgv, false);
    if (cmd == "unsplat") return cmdSplat(subArgc, subArgv, true);
    if (cmd == "label")   return cmdLabel(subArgc, subArgv);
    if (cmd == "validate" || cmd == "check")
                          return cmdValidate(subArgc, subArgv);
    if (cmd == "bam" || cmd == "map")
                          return cmdBAM(subArgc, subArgv);
    if (cmd == "dump" || cmd == "hex")
                          return cmdDump(subArgc, subArgv);
    if (cmd == "mkdir")      return cmdMkdir(subArgc, subArgv);
    if (cmd == "rmdir")      return cmdRmdir(subArgc, subArgv);
    if (cmd == "tree")       return cmdTree(subArgc, subArgv);
#ifdef HAVE_FUSE3
    if (cmd == "mount") {
        if (subArgc < 2) { std::cerr << "Usage: disk45 mount <image> <mountpoint> [-ro]\n"; return 1; }
        std::string imagePath = subArgv[0];
        std::string mountpoint = subArgv[1];
        bool ro = false;
        std::vector<char*> extraArgs;
        for (int i = 2; i < subArgc; i++) {
            if (std::string(subArgv[i]) == "-ro" || std::string(subArgv[i]) == "--readonly")
                ro = true;
            else
                extraArgs.push_back(subArgv[i]);
        }
        return disk45_fuse_main(imagePath, mountpoint, ro, (int)extraArgs.size(),
                                 extraArgs.empty() ? nullptr : extraArgs.data());
    }
#endif
    if (cmd == "catalog")    return disk45_catalog(subArgc, subArgv);
    if (cmd == "convert")    return cmdConvert(subArgc, subArgv);
    if (cmd == "dir-shrink") return cmdDirShrink(subArgc, subArgv);
    if (cmd == "dir-ensure") return cmdDirEnsure(subArgc, subArgv);
    if (cmd == "rel-create") return cmdRelCreate(subArgc, subArgv);
    if (cmd == "rel-read")   return cmdRelRead(subArgc, subArgv);
    if (cmd == "rel-write")  return cmdRelWrite(subArgc, subArgv);
    if (cmd == "rel-list")   return cmdRelList(subArgc, subArgv);
    if (cmd == "bread")   return cmdBread(subArgc, subArgv);
    if (cmd == "bwrite")  return cmdBwrite(subArgc, subArgv);
    if (cmd == "bpeek")   return cmdBpeek(subArgc, subArgv);
    if (cmd == "bpoke")   return cmdBpoke(subArgc, subArgv);
    if (cmd == "bfill")   return cmdBfill(subArgc, subArgv);
    if (cmd == "chain")   return cmdChain(subArgc, subArgv);
    if (cmd == "--version" || cmd == "-v") {
        std::cout << "disk45 v" << VERSION << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    usage();
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    // Batch script mode: disk45 batch <scriptfile>
    std::string first = argv[1];
    if (first == "batch" || first == "--batch") {
        if (argc < 3) { std::cerr << "Usage: disk45 batch <scriptfile>\n"; return 1; }
        return cmdBatch(argv[2]);
    }

    return processCommand(argc, argv);
}
