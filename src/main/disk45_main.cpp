#include "Version.hpp"
#include "Logger.hpp"
#include "BasicLister.hpp"
#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
#include "TapImage.hpp"
#include "GeosCvtImage.hpp"
#include "CmdImage.hpp"
#include "GzipHelper.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <zlib.h>
#include <iterator>
#include <unistd.h>
#include <set>
#include <cstdlib>
#include <termios.h>
#include <cerrno>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

static const char* VERSION = SUITE_VERSION;

// Forward declaration for FUSE mount (in disk45_fuse.cpp)
#ifdef HAVE_FUSE3
extern int disk45_fuse_main(const std::string& imagePath, const std::string& mountpoint,
                             bool readonly, int argc, char** argv);
#endif

// Forward declaration for catalog (in disk45_catalog.cpp)
extern int disk45_catalog(int argc, char** argv);
extern int disk45_sd(int argc, char** argv);

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
              << "  disk45 create <image> [-n name] [-i id] [-t tracks] [--geos]  Create disk image\n"
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
              << "  disk45 repair <image> [--dry]             Rebuild the BAM from the file chains\n"
              << "  disk45 diff <image1> <image2> [--sectors] Compare two images (files or sectors)\n"
              << "  disk45 list <image> --petscii             Names in the C64's characters (Unicode)\n"
              << "  disk45 basic <image> <name> [--v2|--v7|--65]  List a BASIC program\n"
              << "  disk45 shell                              Interactive: attach an image, run commands\n"
              << "  disk45 edit <image> <track> <sector>      Sector hex editor\n"
              << "  disk45 add <image> <file> [name] [-s N] [-b N] [-e] [--track T]  Interleave / placement\n"
              << "  disk45 mkdir <image> <dirname>            Create subdirectory\n"
              << "  disk45 rmdir <image> <dirname>            Remove empty subdirectory\n"
              << "  disk45 tree <image>                       Show directory tree\n"
              << "  disk45 dir-shrink <image> [path]          Reclaim empty dir sectors\n"
              << "  disk45 dir-ensure <image> <slots> [path]  Ensure min free dir slots\n"
              << "  disk45 rel-create <img> <name> <len> [n]  Create REL file\n"
              << "  disk45 rel-read <img> <name> <rec#>       Read REL record\n"
              << "  disk45 rel-write <img> <name> <rec#> <d>  Write REL record\n"
              << "  disk45 rel-list <img> <name>              List REL records\n"
              << "  disk45 sd <command> <card.img> ...      MEGA65 SD card images (disk45 sd --help)\n"
              << "  disk45 catalog build <dir>... [--db file]  Index image collection\n"
              << "  disk45 catalog search <pattern> [--db f]  Search indexed files\n"
              << "  disk45 catalog list [--db file]           List indexed images\n"
              << "  disk45 catalog duplicates [--db file]     Find duplicate files\n"
              << "  disk45 catalog stats [--db file]          Collection statistics\n"
              << "  disk45 -a2p                               ASCII→PETSCII filter (stdin→stdout)\n"
              << "  disk45 -p2a                               PETSCII→ASCII filter (stdin→stdout)\n"
              << "  disk45 batch <scriptfile>                 Run commands from file\n"
              << "\n"
              << "Options (anywhere): -v ... -vvvv  log to stderr ([disk45:INFO] ...): the images\n"
              << "  opened and written (-v), partitions and archive members (-vv); -v0 off.\n"
              << "  A lone -v as the first argument is --version.\n"
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
              << "  .g64  G64 GCR-encoded 1541 (unchanged tracks keep their GCR)\n"
              << "  .g71  G71 GCR-encoded 1571 (unchanged tracks keep their GCR)\n"
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
        case CbmFileType::DIR: return "DIR";
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
// The letters swap case (a-z: $41-$5A, A-Z: $C1-$DA, as BASIC's ASC("A") is
// 193), LF is CR, and the rest stays: the MEGA65's ASCII fonts (FONT A/B)
// show \ ^ _ { | } ~ at their ASCII codes, and cc45 encodes them the same.
// Back to ASCII, both ranges of capitals ($61-$7A, $C1-$DA) are A-Z.

static std::vector<uint8_t> asciiToPetsciiContent(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(data.size());
    for (uint8_t c : data) {
        if (c == '\n') out.push_back(0x0D);            // LF → CR
        else if (c == '\r') continue;                   // strip CR (handle CRLF)
        else if (c >= 'a' && c <= 'z') out.push_back(c - 32);  // lowercase → PETSCII $41-$5A
        else if (c >= 'A' && c <= 'Z') out.push_back(c + 128); // uppercase → PETSCII $C1-$DA
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
        else if (c >= 0x61 && c <= 0x7A) out.push_back(c - 32);  // PETSCII $61-$7A → uppercase
        else if (c >= 0xC1 && c <= 0xDA) out.push_back(c - 128); // PETSCII $C1-$DA → uppercase
        else out.push_back(c);
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

// Find a subdirectory entry (CBM, or a CMD native DIR) of a directory chain.
// Returns the subdirectory's directory chain start
static TrackSector findSubdir(DiskImage* img, TrackSector dirStart, const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    TrackSector ts = dirStart;
    for (int guard = 0; ts.track != 0 && guard < 65536; guard++) {
        const uint8_t* sec = img->sectorData(ts.track, ts.sector);
        if (!sec) break;
        for (int i = 0; i < 8; i++) {
            DirEntry e = DirEntry::fromSector(sec, i);
            if (e.isValid() && DiskImage::isDirectory(e) && e.name() == upper)
                return img->directoryChain(e);
        }
        uint8_t nextT = sec[0], nextS = sec[1];
        if (nextT == 0) break;
        ts = {nextT, nextS};
    }
    return {0, 0}; // not found
}

// Resolve a path like "SUBDIR/SUBDIR2", making it the image's working
// directory: its directory chain start ({0,0}: not found; an empty path is
// the root)
static TrackSector resolveDir(DiskImage* img, const std::vector<std::string>& pathParts) {
    if (!img->enterPath(pathParts)) return {0, 0};
    return img->directoryStart();
}

// A file name with a directory path ("SUB/NAME", "A/B/NAME"): the
// directory's chain start and header (the root for a plain name) and the
// last component. False, with a message, when the directory does not exist.
// The working directory is left as it was
static bool splitDirName(DiskImage* img, std::string& name, TrackSector& dir, TrackSector& header) {
    dir = img->firstDirectorySector();
    header = {0, 0};
    if (name.find('/') == std::string::npos) return true;
    auto parts = splitPath(name);
    if (parts.empty()) { std::cerr << "Error: empty file name\n"; return false; }
    // "SUB/": the directory, the name left empty (the caller's default)
    bool dirOnly = name.back() == '/';
    if (dirOnly) parts.push_back("");
    if (parts.size() > 1) {
        if (img->totalTracks() == 0) {
            std::cerr << "Error: subdirectories only supported on sector-based images\n";
            return false;
        }
        TrackSector cwd = img->inSubdirectory() ? img->directoryStart() : TrackSector{0, 0};
        TrackSector cwdHeader = img->directoryHeader();
        dir = resolveDir(img, std::vector<std::string>(parts.begin(), parts.end() - 1));
        header = img->directoryHeader();
        img->setDirectory(cwd, cwdHeader);
        if (dir.isNull()) {
            std::cerr << "Error: directory \"" << name.substr(0, name.rfind('/')) << "\" not found\n";
            return false;
        }
    }
    name = parts.back();
    return true;
}

// splitDirName, making the directory the image's working directory
static bool enterDirOf(DiskImage* img, std::string& name) {
    TrackSector dir, header;
    if (!splitDirName(img, name, dir, header)) return false;
    if (dir != img->firstDirectorySector()) img->setDirectory(dir, header);
    return true;
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
    return img->firstDirectorySector();
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

// GEOS files go through their Convert (CVT) form: extract writes the CVT,
// copy/convert between disk images rebuild the GEOS file (info block, VLIR
// records), other targets get the CVT as a PRG (as cbmconvert does)
static bool trackImage(DiskImage* img) {
    return img->totalTracks() > 0 && !dynamic_cast<GeosCvtImage*>(img);
}

static bool geosEntry(DiskImage* img, const std::string& name, DirEntry& e) {
    if (!trackImage(img)) return false;
    e = img->findEntry(name);
    return e.isValid() && img->isGeos(e);
}

// Copy one file between images; bytes gets its size
static bool copyImageFile(DiskImage* src, DiskImage* dst, const FileInfo& f, std::string& err, size_t& bytes) {
    DirEntry e;
    if (geosEntry(src, f.name, e)) {
        std::vector<uint8_t> cvt;
        if (!src->readGeosCvt(e, cvt, err)) return false;
        bytes = cvt.size();
        if (trackImage(dst)) return dst->addGeosCvt(cvt, "", err);
        if (!dst->addFile(f.name, CbmFileType::PRG, cvt)) { err = "disk full?"; return false; }
        return true;
    }
    auto data = src->readFile(f.name);
    if (data.empty() && !src->fileExists(f.name)) { err = "could not read it"; return false; }
    bytes = data.size();
    if (!dst->addFile(f.name, f.type, data)) { err = "disk full?"; return false; }
    return true;
}

static int cmdCreate(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string diskName, diskId;
    int tracks = 0; // 0 = use format default
    bool geos = false;
    std::string partitions;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-n" && i + 1 < argc) diskName = argv[++i];
        else if (std::string(argv[i]) == "-i" && i + 1 < argc) diskId = argv[++i];
        else if (std::string(argv[i]) == "-t" && i + 1 < argc) tracks = std::stoi(argv[++i]);
        else if (std::string(argv[i]) == "--geos") geos = true;
        else if ((std::string(argv[i]) == "--partitions" || std::string(argv[i]) == "-P") && i + 1 < argc)
            partitions = argv[++i];
    }

    std::string zipFile, zipMember;
    if (tracks > 0 && DiskImage::zipPath(imagePath, zipFile, zipMember)) {
        std::cerr << "Error: -t cannot be used with an image inside a zip\n";
        return 1;
    }
    auto img = DiskImage::createFromExtension(imagePath);
    if (!img->canCreateBlank()) {
        std::string ext = std::filesystem::path(imagePath).extension().string();
        std::cerr << "Error: disk45 cannot create " << (ext.empty() ? imagePath : ext)
                  << " images (a read-only format; create a .d64 or .d81)\n";
        return 1;
    }
    // a DNP's size is its track count
    if (auto* cmd = dynamic_cast<CmdImage*>(img.get()); cmd && tracks > 0 &&
        std::filesystem::path(gzip::hasGzExtension(imagePath) ? gzip::stripGzExtension(imagePath) : imagePath)
                .extension() == ".dnp") {
        if (tracks > 255) { std::cerr << "Error: a DNP has at most 255 tracks\n"; return 1; }
        cmd->setNativeTracks(tracks);
        tracks = 0;
    }
    // a D1M/D2M/D4M's partitions: TYPE[:NAME[:BLOCKS]],... (native, 1541,
    // 1571, 1581; a native partition's size in 512-byte blocks, default the
    // rest of the disk)
    if (!partitions.empty()) {
        auto* cmd = dynamic_cast<CmdImage*>(img.get());
        if (!cmd) { std::cerr << "Error: --partitions is for D1M/D2M/D4M images\n"; return 1; }
        std::vector<CmdImage::PartSpec> layout;
        std::stringstream list(partitions);
        for (std::string item; std::getline(list, item, ',');) {
            std::vector<std::string> f;
            std::stringstream fs(item);
            for (std::string x; std::getline(fs, x, ':');) f.push_back(x);
            CmdImage::PartSpec p;
            std::string t = f.empty() ? "" : f[0];
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            p.type = t == "native" ? 1 : t == "1541" ? 2 : t == "1571" ? 3 : t == "1581" ? 4 : 0;
            if (!p.type || f.size() > 3 || (f.size() == 3 && p.type != 1)) {
                std::cerr << "Error: partition \"" << item << "\": TYPE[:NAME[:BLOCKS]], TYPE native, 1541, 1571 or 1581 (BLOCKS for native only)\n";
                return 1;
            }
            if (f.size() >= 2) p.name = f[1];
            if (f.size() == 3) p.blocks = (uint32_t)std::stoul(f[2]);
            layout.push_back(p);
        }
        std::string err;
        if (!cmd->setLayout(layout, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    }
    img->format(diskName, diskId);
    img->markBlank();

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
            img->saveImage(imagePath);

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
            img->markBlank();
        }
    }

    if (geos) {
        std::string err;
        if (!img->makeGeosDisk(err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    }
    if (!img->saveImage(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Created " << formatStr(img->diskFormat()) << (geos ? " (GEOS)" : "") << " image: "
              << imagePath << " (" << img->totalTracks() << " tracks, "
              << img->totalBytes() << " bytes)\n";
    return 0;
}

// PETSCII (the C64's power-on upper case / graphics set) as Unicode, the
// graphics from Symbols for Legacy Computing (Unicode 13): a name shown as on
// the C64's screen
static std::string petsciiGlyph(uint8_t c) {
    static const char32_t g60[32] = {   // $60-$7F (= $C0-$DF)
        0x2500, 0x2660, 0x1FB72, 0x1FB78, 0x1FB77, 0x1FB76, 0x1FB7A, 0x1FB71,
        0x1FB74, 0x256E, 0x2570, 0x256F, 0x1FB7C, 0x2572, 0x2571, 0x1FB7D,
        0x1FB7E, 0x25CF, 0x1FB7B, 0x2665, 0x1FB70, 0x256D, 0x2573, 0x25CB,
        0x2663, 0x1FB75, 0x2666, 0x253C, 0x1FB8C, 0x2502, 0x03C0, 0x25E5};
    static const char32_t gA0[32] = {   // $A0-$BF (= $E0-$FE; $FF is pi)
        0x00A0, 0x258C, 0x2584, 0x2594, 0x2581, 0x258F, 0x2592, 0x2595,
        0x1FB8F, 0x25E4, 0x1FB87, 0x251C, 0x2597, 0x2514, 0x2510, 0x2582,
        0x250C, 0x2534, 0x252C, 0x2524, 0x258E, 0x258D, 0x1FB88, 0x1FB82,
        0x1FB83, 0x2583, 0x1FB7F, 0x2596, 0x259D, 0x2518, 0x2598, 0x259A};
    char32_t u;
    if (c >= 0x20 && c <= 0x5B) u = c;                 // space, digits, @, A-Z, [
    else if (c == 0x5C) u = 0x00A3;                    // pound
    else if (c == 0x5D) u = ']';
    else if (c == 0x5E) u = 0x2191;                    // up arrow
    else if (c == 0x5F) u = 0x2190;                    // left arrow
    else if (c >= 0x60 && c <= 0x7F) u = g60[c - 0x60];
    else if (c >= 0xC0 && c <= 0xDF) u = g60[c - 0xC0];
    else if (c >= 0xA0 && c <= 0xBF) u = gA0[c - 0xA0];
    else if (c >= 0xE0 && c <= 0xFE) u = gA0[c - 0xE0];
    else if (c == 0xFF) u = 0x03C0;
    else u = 0x00B7;                                   // a control code: a middle dot
    std::string out;                                   // UTF-8
    if (u < 0x80) out += (char)u;
    else if (u < 0x800) { out += (char)(0xC0 | (u >> 6)); out += (char)(0x80 | (u & 0x3F)); }
    else if (u < 0x10000) {
        out += (char)(0xE0 | (u >> 12)); out += (char)(0x80 | ((u >> 6) & 0x3F)); out += (char)(0x80 | (u & 0x3F));
    } else {
        out += (char)(0xF0 | (u >> 18)); out += (char)(0x80 | ((u >> 12) & 0x3F));
        out += (char)(0x80 | ((u >> 6) & 0x3F)); out += (char)(0x80 | (u & 0x3F));
    }
    return out;
}

static std::string petsciiText(const std::string& p) {
    std::string out;
    for (unsigned char c : p) out += petsciiGlyph(c);
    return out;
}

static int cmdList(int argc, char** argv) {
    // --petscii: names in the C64's characters (Unicode)
    bool petsciiView = false;
    for (int i = 0; i < argc; i++)
        if (std::string(argv[i]) == "--petscii") {
            petsciiView = true;
            for (int j = i; j + 1 < argc; j++) argv[j] = argv[j + 1];
            argc--;
            break;
        }
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

    if (petsciiView) {
        // as the C64 lists it: the name padded to 16 in quotes, the ID after
        std::string dn = DiskImage::asciiToPetscii(img->diskName());
        dn.resize(16, ' ');
        std::cout << "0 \"" << petsciiText(dn) << "\" " << petsciiText(DiskImage::asciiToPetscii(img->diskId())) << "\n";
        for (auto& f : files) {
            DirEntry e = img->totalTracks() > 0 ? img->findEntry(f.name) : DirEntry{};
            std::string raw = e.isValid() ? e.petsciiName() : DiskImage::asciiToPetscii(f.name);
            std::string blocks = std::to_string(f.sizeInSectors);
            std::string quoted = "\"" + petsciiText(raw) + "\"";
            std::cout << blocks << std::string(blocks.size() < 5 ? 5 - blocks.size() : 1, ' ') << quoted
                      << std::string(raw.size() < 17 ? 17 - raw.size() : 1, ' ') << (f.closed ? " " : "*")
                      << fileTypeStr(f.type) << (f.locked ? "<" : "") << "\n";
        }
        std::cout << img->freeSectors() << " BLOCKS FREE.\n";
        return 0;
    }
    std::cout << "0 \"" << img->diskName() << "\" " << img->diskId() << "\n";
    for (auto& f : files) {
        std::cout << f.sizeInSectors << "\t\"" << f.name << "\"\t";
        if (!f.closed) std::cout << "*"; // splat indicator
        std::cout << fileTypeStr(f.type);
        if (f.locked) std::cout << "<";
        DirEntry e;
        if (pathParts.size() <= 1 && geosEntry(img.get(), f.name, e))
            std::cout << "\tGEOS " << DiskImage::geosTypeName(e.unused[0]) << (e.recordLen ? ", VLIR" : "");
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
        if (!cvt->geosClassName().empty()) std::cout << "Class:        " << cvt->geosClassName() << "\n";
        if (!cvt->geosAuthor().empty()) std::cout << "Author:       " << cvt->geosAuthor() << "\n";
        if (cvt->isVlir())
            std::cout << "VLIR records: " << cvt->vlirRecordCount() << "\n";
        std::string desc = cvt->geosDescription();
        if (!desc.empty())
            std::cout << "Description:  " << desc << "\n";
        std::cout << "Files:        " << img->listFiles().size() << "\n";
    } else {
        auto* cmdImg = dynamic_cast<CmdImage*>(img.get());
        std::cout << "Format:       " << (cmdImg ? cmdImg->formatName() : formatStr(img->diskFormat())) << "\n"
                  << "Disk name:    " << img->diskName() << "\n"
                  << "Disk ID:      " << img->diskId() << "\n"
                  << "Tracks:       " << img->totalTracks() << "\n"
                  << "Total sectors:" << img->totalSectors() << "\n"
                  << "Free sectors: " << img->freeSectors() << "\n"
                  << "Image size:   " << img->totalBytes() << " bytes\n"
                  << "Files:        " << img->listFiles().size() << "\n";
        if (trackImage(img.get()) && !img->geosSignature().empty())
            std::cout << "GEOS:         " << img->geosSignature() << "\n";
        if (auto* cmd = dynamic_cast<CmdImage*>(img.get()); cmd && !cmd->partitions().empty()) {
            std::cout << "Partitions:   (open one as " << argv[0] << ":N or :NAME)\n";
            for (const auto& p : cmd->partitions())
                std::cout << "  " << std::setw(2) << p.number << "  " << std::left << std::setw(14)
                          << CmdImage::partitionTypeName(p.type) << std::right << " \"" << p.name << "\"  "
                          << p.sectors << " sectors at " << p.start
                          << (cmd->hasNative() && p.start == cmd->nativeStart() && p.type == 1 ? "  (the default)" : "")
                          << "\n";
        }
    }
    return 0;
}

static int cmdAdd(int argc, char** argv) {
    bool petscii = extractPetsciiFlag(argc, argv);
    // placement: -s N interleave, -b N first sector, -e an empty track,
    // --track T a track (demo loaders that read sectors in a set order)
    DiskImage::Placement place;
    {
        int out = 0;
        for (int i = 0; i < argc; i++) {
            std::string a = argv[i];
            auto num = [&](int& dst) {
                if (i + 1 >= argc) return false;
                dst = std::atoi(argv[++i]);
                return true;
            };
            if (a == "-s" && num(place.interleave)) continue;
            if (a == "-b" && num(place.beginSector)) continue;
            if (a == "--track" && num(place.track)) continue;
            if (a == "-e") { place.emptyTrack = true; continue; }
            argv[out++] = argv[i];
        }
        argc = out;
    }
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

    if (place.active()) {
        if (place.track && (place.track < 1 || place.track > img->totalTracks() || !img->sectorsOnTrack(place.track))) {
            std::cerr << "Error: the image has no track " << place.track << "\n";
            return 1;
        }
        img->setPlacement(place);
    }

    // "SUB/NAME" adds into a subdirectory ("SUB/" keeps the host name)
    if (argc >= 3) {
        if (!enterDirOf(img.get(), cbmName)) return 1;
        if (cbmName.empty()) cbmName = baseName(hostFile);
    }

    // A GEOS Convert file becomes a GEOS file (info block, VLIR records);
    // its name comes from the CVT unless one is given
    if (DiskImage::isCvt(data) && trackImage(img.get())) {
        std::string err;
        std::string name = argc >= 3 && std::string(argv[2]).back() != '/' ? cbmName : "";
        if (!img->addGeosCvt(data, name, err)) {
            std::cerr << "Error: failed to add the GEOS file: " << err << "\n";
            return 1;
        }
        if (!img->saveImage(imagePath)) {
            std::cerr << "Error: failed to write " << imagePath << "\n";
            return 1;
        }
        std::cout << "Added GEOS file \"" << (name.empty() ? DiskImage::cvtName(data) : name) << "\" (from "
                  << hostFile << ")\n";
        return 0;
    }

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
    if (!img->saveImage(imagePath)) {
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
    if (!enterDirOf(img.get(), cbmName)) return 1;

    DirEntry ge;
    if (geosEntry(img.get(), cbmName, ge)) {
        std::vector<uint8_t> cvt;
        std::string err;
        if (!img->readGeosCvt(ge, cvt, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
        std::ofstream f(hostFile, std::ios::binary);
        if (!f) { std::cerr << "Error: cannot write " << hostFile << "\n"; return 1; }
        f.write(reinterpret_cast<const char*>(cvt.data()), cvt.size());
        std::cout << "Extracted GEOS file \"" << cbmName << "\" as Convert (CVT) (" << cvt.size() << " bytes)\n";
        return 0;
    }

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
    if (!enterDirOf(img.get(), cbmName)) return 1;
    if (DiskImage::isDirectory(img->findEntry(cbmName))) {
        std::cerr << "Error: \"" << cbmName << "\" is a directory (use rmdir)\n";
        return 1;
    }

    if (!img->removeFile(cbmName)) {
        std::cerr << "Error: file \"" << cbmName << "\" not found\n";
        return 1;
    }
    if (!img->saveImage(imagePath)) {
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
    if (!enterDirOf(img.get(), oldName)) return 1;
    TrackSector toDir, toHeader;
    if (!splitDirName(img.get(), newName, toDir, toHeader)) return 1;
    if (newName.empty()) newName = oldName;  // "mv X SUB/"

    // A GEOS file keeps the case of its new name (GEOS names are ASCII)
    DirEntry oe;
    if (geosEntry(img.get(), oldName, oe)) {
        newName = argv[2];
        if (newName.size() >= 2 && newName.front() == '"' && newName.back() == '"')
            newName = newName.substr(1, newName.size() - 2);
        if (newName.find('/') != std::string::npos) newName = newName.substr(newName.rfind('/') + 1);
        if (newName.empty()) newName = oe.name();
    }

    // Into another directory ("mv SUB/X Y", "mv X SUB/Y"): the entry moves
    if (toDir != img->directoryStart()) {
        if (!img->moveFile(oldName, toDir, newName, toHeader)) {
            std::cerr << "Error: cannot move \"" << oldName << "\" (not found, a directory, or the name is taken)\n";
            return 1;
        }
        if (!img->saveImage(imagePath)) {
            std::cerr << "Error: failed to write " << imagePath << "\n";
            return 1;
        }
        std::cout << "Moved \"" << argv[1] << "\" → \"" << argv[2] << "\"\n";
        return 0;
    }

    // Try the standard renameFile first (handles regular files)
    if (img->renameFile(oldName, newName)) {
        if (!img->saveImage(imagePath)) {
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
    TrackSector ts = img->directoryStart();
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
    if (!img->saveImage(imagePath)) {
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
    if (!img->saveImage(imagePath)) {
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
    if (!enterDirOf(img.get(), cbmName)) return 1;

    if (!img->lockFile(cbmName, lock)) {
        std::cerr << "Error: file \"" << cbmName << "\" not found\n";
        return 1;
    }
    if (!img->saveImage(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << (lock ? "Locked" : "Unlocked") << " \"" << cbmName << "\"\n";
    return 0;
}

// The files of an image, subdirectories included ("DIR/NAME"): type,
// blocks, size and CRC-32 of the contents
struct DiffFile { CbmFileType type; int blocks; size_t size; uint32_t crc; };
static void diffFiles(DiskImage* img, const std::string& prefix, std::map<std::string, DiffFile>& out, int depth = 0) {
    TrackSector here = img->inSubdirectory() ? img->directoryStart() : TrackSector{0, 0};
    TrackSector hereHdr = img->directoryHeader();
    for (const auto& f : img->listFiles()) {
        std::string path = prefix + f.name;
        if (f.type == CbmFileType::CBM || f.type == CbmFileType::DIR) {
            out[path + "/"] = {f.type, f.sizeInSectors, 0, 0};
            DirEntry e = img->findEntry(f.name);
            if (e.isValid() && depth < 16 && !img->directoryChain(e).isNull()) {
                img->enterDirectory(e);
                diffFiles(img, path + "/", out, depth + 1);
                img->setDirectory(here, hereHdr);
            }
            continue;
        }
        auto data = img->readFile(f.name);
        out[path] = {f.type, f.sizeInSectors, data.size(),
                     (uint32_t)::crc32(0, data.data(), (uInt)data.size())};
    }
}

// Compare two images: their files (added, removed, changed), or with
// --sectors every sector. Exit status 0 when they are the same, 1 otherwise
static int cmdDiff(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Usage: disk45 diff <image1> <image2> [--sectors]\n"; return 2; }
    bool sectors = false;
    for (int i = 2; i < argc; i++)
        if (std::string(argv[i]) == "--sectors" || std::string(argv[i]) == "-s") sectors = true;
    auto a = DiskImage::load(argv[0]);
    if (!a) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 2; }
    auto b = DiskImage::load(argv[1]);
    if (!b) { std::cerr << "Error: failed to load " << argv[1] << "\n"; return 2; }
    int differences = 0;
    if (sectors) {
        if (a->totalTracks() != b->totalTracks() || a->totalSectors() != b->totalSectors() ||
            a->totalTracks() == 0) {
            std::cerr << "Error: --sectors needs two sector images of the same geometry\n";
            return 2;
        }
        int t0 = a->sectorData(0, 0) ? 0 : 1;   // D90/D60 have a track 0
        for (int t = t0; t <= a->totalTracks(); t++)
            for (int sec = 0; sec < a->sectorsOnTrack(t); sec++) {
                const uint8_t* x = a->sectorData(t, sec);
                const uint8_t* y = b->sectorData(t, sec);
                if (!x || !y || std::memcmp(x, y, 256) == 0) continue;
                int n = 0, first = -1;
                for (int i = 0; i < 256; i++)
                    if (x[i] != y[i]) { if (first < 0) first = i; n++; }
                std::cout << t << "/" << sec << ": " << n << " byte" << (n == 1 ? "" : "s") << " differ (first at $"
                          << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << first
                          << std::dec << std::setfill(' ') << ")\n";
                differences++;
            }
        std::cout << (differences ? std::to_string(differences) + " sector(s) differ" : std::string("The sectors are the same"))
                  << "\n";
        return differences ? 1 : 0;
    }
    std::map<std::string, DiffFile> fa, fb;
    diffFiles(a.get(), "", fa);
    diffFiles(b.get(), "", fb);
    auto crcHex = [](uint32_t c) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%08X", c);
        return std::string(buf);
    };
    for (const auto& [name, x] : fa) {
        auto it = fb.find(name);
        if (it == fb.end()) {
            std::cout << "- " << name << " (" << fileTypeStr(x.type) << ", " << x.blocks << " blocks)\n";
            differences++;
            continue;
        }
        const DiffFile& y = it->second;
        if (x.type != y.type || x.size != y.size || x.crc != y.crc) {
            std::cout << "~ " << name << ":";
            if (x.type != y.type) std::cout << " type " << fileTypeStr(x.type) << " -> " << fileTypeStr(y.type);
            if (x.size != y.size) std::cout << " size " << x.size << " -> " << y.size;
            if (x.crc != y.crc) std::cout << " crc " << crcHex(x.crc) << " -> " << crcHex(y.crc);
            std::cout << "\n";
            differences++;
        }
    }
    for (const auto& [name, y] : fb)
        if (!fa.count(name)) {
            std::cout << "+ " << name << " (" << fileTypeStr(y.type) << ", " << y.blocks << " blocks)\n";
            differences++;
        }
    if (a->diskName() != b->diskName() || a->diskId() != b->diskId()) {
        std::cout << "~ disk name/ID: \"" << a->diskName() << "\" " << a->diskId() << " -> \"" << b->diskName()
                  << "\" " << b->diskId() << "\n";
        differences++;
    }
    std::cout << (differences ? std::to_string(differences) + " difference(s)" : std::string("The files are the same")) << "\n";
    return differences ? 1 : 0;
}

// A BASIC program on an image (or a .prg on the host) as LIST shows it
static int cmdBasic(int argc, char** argv) {
    BasicDialect dialect = BasicDialect::AUTO;
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--v2" || a == "-2") dialect = BasicDialect::V2;
        else if (a == "--v7" || a == "-7") dialect = BasicDialect::V7;
        else if (a == "--65" || a == "-6") dialect = BasicDialect::B65;
        else args.push_back(a);
    }
    if (args.empty()) {
        std::cerr << "Usage: disk45 basic <image> <name> [--v2|--v7|--65]\n"
                  << "       disk45 basic <file.prg> [--v2|--v7|--65]\n";
        return 1;
    }
    std::vector<uint8_t> data;
    if (args.size() == 1) {
        std::ifstream f(args[0], std::ios::binary);
        if (!f) { std::cerr << "Error: cannot open " << args[0] << "\n"; return 1; }
        data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    } else {
        auto img = DiskImage::load(args[0]);
        if (!img) { std::cerr << "Error: failed to load " << args[0] << "\n"; return 1; }
        std::string name = normalizeName(args[1]);
        if (!enterDirOf(img.get(), name)) return 1;
        if (!img->fileExists(name)) { std::cerr << "Error: file \"" << name << "\" not found\n"; return 1; }
        data = img->readFile(name);
    }
    BasicListing l = listBasic(data, dialect);
    if (!l.ok) { std::cerr << "Error: " << l.error << "\n"; return 1; }
    for (const auto& w : l.warnings) std::cerr << "Warning: " << w << "\n";
    std::cout << l.text;
    return 0;
}

// Rebuild the BAM from the file chains (the directory, subdirectories,
// every file's sectors, the header and the format's own sectors)
static int cmdRepair(int argc, char** argv) {
    if (argc < 1) { std::cerr << "Usage: disk45 repair <image> [--dry]\n"; return 1; }
    bool dry = false;
    for (int i = 1; i < argc; i++)
        if (std::string(argv[i]) == "--dry" || std::string(argv[i]) == "-n") dry = true;
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    if (img->totalTracks() == 0) { std::cerr << "Error: " << argv[0] << " has no BAM (an archive)\n"; return 1; }
    if (!img->writable() && !dry) {
        std::cerr << "Error: disk45 cannot write this format (use --dry to see the changes)\n";
        return 1;
    }
    auto r = img->rebuildBam(dry);
    for (const auto& e : r.errors) std::cerr << "Warning: " << e << "\n";
    auto show = [](const char* what, const std::vector<TrackSector>& v) {
        std::cout << what << ": " << v.size() << " sector" << (v.size() == 1 ? "" : "s");
        for (size_t i = 0; i < v.size() && i < 16; i++)
            std::cout << (i ? ", " : " (") << (int)v[i].track << "/" << (int)v[i].sector;
        if (!v.empty()) std::cout << (v.size() > 16 ? ", ...)" : ")");
        std::cout << "\n";
    };
    show(dry ? "Would mark used" : "Marked used", r.allocated);
    show(dry ? "Would free" : "Freed", r.freed);
    if (dry || (r.allocated.empty() && r.freed.empty())) {
        if (!dry) std::cout << "The BAM matches the files\n";
        return 0;
    }
    if (!img->saveImage(argv[0])) { std::cerr << "Error: failed to write " << argv[0] << "\n"; return 1; }
    std::cout << img->freeSectors() << " blocks free.\n";
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
    if (!img->saveImage(imagePath)) {
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
        int dirT = img->headerTrack();
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
        if (dst->fileExists(f.name)) {
            std::cerr << "Warning: \"" << f.name << "\" already exists in destination, skipping\n";
            continue;
        }
        std::string err;
        size_t bytes = 0;
        if (!copyImageFile(src.get(), dst.get(), f, err, bytes)) {
            std::cerr << "Error: failed to copy \"" << f.name << "\": " << err << "\n";
            continue;
        }
        std::cout << "  " << f.name << " (" << bytes << " bytes, " << fileTypeStr(f.type) << ")\n";
        copied++;
    }

    if (copied > 0) {
        if (!dst->saveImage(dstPath)) {
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
        DirEntry ge;
        bool geos = geosEntry(img.get(), f.name, ge);
        std::vector<uint8_t> data;
        if (geos) {
            std::string err;
            if (!img->readGeosCvt(ge, data, err)) {
                std::cerr << "Warning: \"" << f.name << "\": " << err << ", skipping\n";
                continue;
            }
        } else {
            data = img->readFile(f.name);
            if (data.empty() && !img->fileExists(f.name)) {
                std::cerr << "Warning: could not read \"" << f.name << "\", skipping\n";
                continue;
            }
        }

        if (petscii && f.type == CbmFileType::SEQ && !geos) {
            data = petsciiToAsciiContent(data);
        }

        std::string ext = geos ? std::string(".cvt") : typeExtension(f.type);
        std::string hostName = sanitizeFilename(f.name) + ext;
        std::string fullPath = outDir + "/" + hostName;

        // Handle name collisions
        if (std::ifstream(fullPath).good()) {
            int suffix = 1;
            std::string base = sanitizeFilename(f.name);
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

    if (!img->saveImage(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
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

    if (!img->saveImage(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
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
    if (!img->saveImage(argv[0])) { std::cerr << "Error: failed to write image\n"; return 1; }
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
    TrackSector firstTS = {0, 0};
    TrackSector ts = img->firstDirectorySector();
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
//
// A REL (relative) file, as CBM DOS lays it out:
//   data sectors  the records packed one after another, 254 data bytes per
//                 sector (a record can span two sectors); an empty record is
//                 $FF and then $00s; the last sector's byte 1 is the offset of
//                 its last used byte
//   side sectors  $00-$01 the next side sector, $02 its number in its group
//                 (0-5), $03 the record length, $04-$0F the group's 6 side
//                 sectors, $10-$FF up to 120 data sector T/S pairs
//   1581 (D81, D65): the directory entry points at a super side sector:
//                 $00-$01 the first side sector, $02 $FE, $03.. the first side
//                 sector of each group of 6 (up to 126 groups)

// The data sectors of a REL file, in order
static std::vector<TrackSector> relDataSectors(const DiskImage* img, const DirEntry& e) {
    std::vector<TrackSector> sectors;
    TrackSector ss = e.sideTS;
    if (img->superSideSectors()) {
        const uint8_t* super = img->sectorData(ss.track, ss.sector);
        if (!super || super[2] != 0xFE) return sectors;
        ss = {super[0], super[1]};
    }
    int guard = 0;
    while (ss.track != 0 && guard++ < 126 * 6) {
        const uint8_t* sec = img->sectorData(ss.track, ss.sector);
        if (!sec) break;
        for (int i = 0x10; i < 0x100; i += 2) {
            if (sec[i] == 0) break;
            sectors.push_back({sec[i], sec[i + 1]});
        }
        ss = {sec[0], sec[1]};
    }
    return sectors;
}

// The REL file's directory entry, or an error message
static bool relFind(const DiskImage* img, const std::string& name, DirEntry& out) {
    out = img->findEntry(name);
    if (!out.isValid() || out.type() != CbmFileType::REL) {
        std::cerr << "Error: REL file \"" << name << "\" not found\n";
        return false;
    }
    if (out.recordLen < 1) {
        std::cerr << "Error: \"" << name << "\" has an invalid record length\n";
        return false;
    }
    return true;
}

// Bytes in a REL file's data sectors (the last one up to its last used byte)
static int relDataBytes(const DiskImage* img, const std::vector<TrackSector>& data) {
    if (data.empty()) return 0;
    const uint8_t* last = img->sectorData(data.back().track, data.back().sector);
    int inLast = (last && last[0] == 0 && last[1] >= 1) ? last[1] - 1 : 254;
    return (int)(data.size() - 1) * 254 + inLast;
}

// Byte i of the data (0-based): its sector and offset
static uint8_t* relByte(DiskImage* img, const std::vector<TrackSector>& data, int i) {
    size_t s = (size_t)(i / 254);
    if (s >= data.size()) return nullptr;
    uint8_t* sec = img->sectorData(data[s].track, data[s].sector);
    return sec ? sec + 2 + i % 254 : nullptr;
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
    if (numRecords < 1) {
        std::cerr << "Error: at least one record\n";
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

    const bool super = img->superSideSectors();
    int dataBytes = numRecords * recordLen;
    int dataSectors = (dataBytes + 253) / 254;
    int sideSectors = (dataSectors + 119) / 120;
    int groups = (sideSectors + 5) / 6;
    if (!super && sideSectors > 6) {
        std::cerr << "Error: too many records: a REL file on this disk holds at most 720 data sectors ("
                  << 720 * 254 / recordLen << " records of " << recordLen << " bytes)\n";
        return 1;
    }
    if (super && groups > 126) {
        std::cerr << "Error: too many records for a REL file\n";
        return 1;
    }
    if (img->freeSectors() < dataSectors + sideSectors + (super ? 1 : 0)) {
        std::cerr << "Error: disk full\n";
        return 1;
    }

    auto alloc = [&](std::vector<TrackSector>& v, int n) {
        for (int i = 0; i < n; i++) {
            TrackSector ts = img->allocateNextFree(-1);
            if (ts.isNull()) return false;
            v.push_back(ts);
        }
        return true;
    };
    std::vector<TrackSector> data, side, superSec;
    if (!alloc(data, dataSectors) || !alloc(side, sideSectors) || (super && !alloc(superSec, 1))) {
        std::cerr << "Error: disk full\n";
        return 1;
    }

    // As DOS does, the file fills its last data sector: as many records as
    // the sectors hold, an empty-record $FF at every record start in them
    // (also a last, partial one)
    int records = dataSectors * 254 / recordLen;
    dataBytes = records * recordLen;

    // Data: empty records ($FF, then $00s), the sectors linked
    for (int i = 0; i < dataSectors; i++) {
        uint8_t* sec = img->sectorData(data[i].track, data[i].sector);
        std::memset(sec, 0, 256);
        if (i + 1 < dataSectors) {
            sec[0] = data[i + 1].track;
            sec[1] = data[i + 1].sector;
        } else {
            sec[0] = 0;
            sec[1] = (uint8_t)(1 + (dataBytes - 254 * (dataSectors - 1)));   // last used byte
        }
    }
    for (int at = 0; at < dataSectors * 254; at += recordLen) *relByte(img.get(), data, at) = 0xFF;

    // Side sectors, chained; each lists its group and up to 120 data sectors
    for (int i = 0; i < sideSectors; i++) {
        uint8_t* sec = img->sectorData(side[i].track, side[i].sector);
        std::memset(sec, 0, 256);
        if (i + 1 < sideSectors) {
            sec[0] = side[i + 1].track;
            sec[1] = side[i + 1].sector;
        } else {
            sec[0] = 0;
            int pairs = dataSectors - 120 * i;
            sec[1] = (uint8_t)(0x10 + 2 * pairs - 1);   // last used byte
        }
        sec[2] = (uint8_t)(i % 6);
        sec[3] = (uint8_t)recordLen;
        int g0 = (i / 6) * 6;
        for (int g = 0; g < 6 && g0 + g < sideSectors; g++) {
            sec[0x04 + 2 * g] = side[g0 + g].track;
            sec[0x05 + 2 * g] = side[g0 + g].sector;
        }
        for (int k = 0; k < 120 && 120 * i + k < dataSectors; k++) {
            sec[0x10 + 2 * k] = data[120 * i + k].track;
            sec[0x11 + 2 * k] = data[120 * i + k].sector;
        }
    }
    if (super) {
        uint8_t* sec = img->sectorData(superSec[0].track, superSec[0].sector);
        std::memset(sec, 0, 256);
        sec[0] = side[0].track;
        sec[1] = side[0].sector;
        sec[2] = 0xFE;
        for (int g = 0; g < groups; g++) {
            sec[3 + 2 * g] = side[6 * g].track;
            sec[4 + 2 * g] = side[6 * g].sector;
        }
    }

    DirEntry entry;
    std::memset(&entry, 0, sizeof(entry));
    entry.fileType = 0x80 | (uint8_t)CbmFileType::REL;
    entry.firstDataTS = data[0];
    entry.sideTS = super ? superSec[0] : side[0];
    entry.recordLen = (uint8_t)recordLen;
    entry.sizeInSectors = (uint16_t)(dataSectors + sideSectors + (super ? 1 : 0));
    DiskImage::padPetsciiName(entry.filename, name);
    if (img->addEntry(entry).isNull()) { std::cerr << "Error: directory full\n"; return 1; }

    if (!img->saveImage(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Created REL file \"" << name << "\" (record length=" << recordLen
              << ", " << records << " records";
    if (records != numRecords) std::cout << " (" << numRecords << " asked; the last sector is filled)";
    std::cout << ", " << dataSectors << " data sectors)\n";
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
    DirEntry e;
    if (!relFind(img.get(), name, e)) return 1;
    int recordLen = e.recordLen;
    auto data = relDataSectors(img.get(), e);
    int records = relDataBytes(img.get(), data) / recordLen;
    if (recNum < 1 || recNum > records) {
        std::cerr << "Error: record " << recNum << " out of range (1-" << records << ")\n";
        return 1;
    }

    std::vector<uint8_t> record;
    for (int b = 0; b < recordLen; b++) {
        uint8_t* p = relByte(img.get(), data, (recNum - 1) * recordLen + b);
        record.push_back(p ? *p : 0);
    }

    std::cout << "Record " << recNum << " (" << recordLen << " bytes): ";
    std::cout << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t b : record) std::cout << std::setw(2) << (int)b << " ";
    std::cout << std::dec << "\n";
    std::cout << "  Text: \"";
    for (uint8_t b : record) {
        if (b >= 0x20 && b < 0x7F) std::cout << (char)b;
        else if (b >= 0xC1 && b <= 0xDA) std::cout << (char)(b - 0x60);
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
    std::vector<uint8_t> bytes;
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        // If it looks like a quoted string, use the bytes directly
        if (arg.size() >= 2 && arg[0] == '"' && arg.back() == '"') {
            for (size_t j = 1; j < arg.size() - 1; j++)
                bytes.push_back((uint8_t)arg[j]);
        } else {
            // Treat as numeric value
            bytes.push_back((uint8_t)std::stoul(arg, nullptr, 0));
        }
    }

    DirEntry e;
    if (!relFind(img.get(), name, e)) return 1;
    int recordLen = e.recordLen;
    auto data = relDataSectors(img.get(), e);
    int records = relDataBytes(img.get(), data) / recordLen;
    if (recNum < 1 || recNum > records) {
        std::cerr << "Error: record " << recNum << " out of range (1-" << records
                  << "; rel-create makes the records)\n";
        return 1;
    }
    if ((int)bytes.size() > recordLen) {
        std::cerr << "Error: " << bytes.size() << " bytes do not fit a " << recordLen << "-byte record\n";
        return 1;
    }
    // the rest of the record is $00, as DOS writes it
    bytes.resize(recordLen, 0);
    for (int b = 0; b < recordLen; b++) {
        uint8_t* p = relByte(img.get(), data, (recNum - 1) * recordLen + b);
        if (p) *p = bytes[b];
    }

    if (!img->saveImage(argv[0])) {
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
    DirEntry e;
    if (!relFind(img.get(), name, e)) return 1;
    int recordLen = e.recordLen;
    auto data = relDataSectors(img.get(), e);
    int totalRecords = relDataBytes(img.get(), data) / recordLen;

    std::cout << "REL file \"" << name << "\": " << totalRecords << " records, "
              << recordLen << " bytes/record, " << data.size() << " data sectors\n";

    // the records that are not empty ($FF then $00s)
    for (int r = 1; r <= totalRecords; r++) {
        std::vector<uint8_t> rec;
        for (int b = 0; b < recordLen; b++) {
            uint8_t* p = relByte(img.get(), data, (r - 1) * recordLen + b);
            rec.push_back(p ? *p : 0);
        }
        bool empty = rec[0] == 0xFF || rec[0] == 0;
        for (int b = 1; b < recordLen && empty; b++) if (rec[b] != 0) empty = false;
        if (empty) continue;
        std::cout << "  #" << std::setw(3) << r << ": ";
        for (uint8_t val : rec) {
            if (val >= 0x20 && val < 0x7F) std::cout << (char)val;
            else if (val >= 0xC1 && val <= 0xDA) std::cout << (char)(val - 0x60);
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
        parentDir = img->firstDirectorySector();
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

    if (!img->makeDirectory(newName)) {
        std::cerr << "Error: \"" << newName << "\" exists or the disk is full\n";
        return 1;
    }

    if (!img->saveImage(argv[0])) {
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
        parentDir = img->firstDirectorySector();
    } else {
        parentDir = resolveDir(img.get(), parentPath);
        if (parentDir.isNull()) { std::cerr << "Error: parent not found\n"; return 1; }
    }

    int rc = img->removeDirectory(dirName);
    if (rc == -ENOTEMPTY) {
        std::cerr << "Error: directory \"" << dirName << "\" is not empty\n";
        return 1;
    }
    if (rc != 0) { std::cerr << "Error: directory \"" << dirName << "\" not found\n"; return 1; }

    // Shrink the parent directory chain (remove trailing empty sectors)
    int reclaimed = shrinkDirChain(img.get(), parentDir);
    img->directoryBlocksChanged(-reclaimed);

    if (!img->saveImage(argv[0])) {
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
        if (f.type == CbmFileType::CBM || f.type == CbmFileType::DIR) {
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

    TrackSector rootDir = img->firstDirectorySector();

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
    img->directoryBlocksChanged(-reclaimed);
    if (reclaimed > 0) {
        if (!img->saveImage(argv[0])) {
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
    img->directoryBlocksChanged(added);
    if (added > 0) {
        if (!img->saveImage(argv[0])) {
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

        if (DiskImage::isCvt(data) && trackImage(dst.get())) {
            std::string err;
            if (!dst->addGeosCvt(data, "", err) || !dst->saveImage(dstPath)) {
                std::cerr << "Error: " << (err.empty() ? "failed to write " + dstPath : err) << "\n";
                return 1;
            }
            std::cout << "Converted " << srcPath << " → " << dstPath << " (GEOS file \""
                      << DiskImage::cvtName(data) << "\")\n";
            return 0;
        }
        if (!dst->addFile(cbmName, ftype, data)) {
            std::cerr << "Error: failed to add file to target image\n";
            return 1;
        }
        if (!dst->saveImage(dstPath)) {
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
    if (trackImage(src.get()) && !src->geosSignature().empty() && trackImage(dst.get())) {
        std::string err;
        if (!dst->makeGeosDisk(err)) std::cerr << "Note: the target is not GEOS-formatted (" << err << ")\n";
    }

    auto files = src->listFiles();
    int copied = 0;
    for (auto& f : files) {
        if (f.type == CbmFileType::CBM || f.type == CbmFileType::DIR) continue; // skip subdirectory entries
        std::string err;
        size_t bytes = 0;
        if (!copyImageFile(src.get(), dst.get(), f, err, bytes)) {
            std::cerr << "Warning: failed to add \"" << f.name << "\" (" << err << ")\n";
            continue;
        }
        copied++;
    }

    if (!dst->saveImage(dstPath)) {
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
// A command line as argv-style words (spaces separate, "..." keeps them)
static std::vector<std::string> splitCommandLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuote = false, any = false;
    for (char c : line) {
        if (c == '"') { inQuote = !inQuote; any = true; continue; }
        if ((c == ' ' || c == '\t') && !inQuote) {
            if (any || !current.empty()) { tokens.push_back(current); current.clear(); any = false; }
        } else {
            current += c;
        }
    }
    if (any || !current.empty()) tokens.push_back(current);
    return tokens;
}

// disk45 edit: a sector's 256 bytes as hex, edited in place. Keys: the
// arrows move, two hex digits set a byte, n / p (Page Down / Up) go to the
// next / previous sector, w writes the image, q quits (a second q discards
// unwritten changes). From a terminal it runs full screen (raw keys); keys
// piped in (tests) work the same, without the screen
static int cmdEdit(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Usage: disk45 edit <image> <track> <sector>\n"; return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }
    int t = std::atoi(argv[1]), sec = std::atoi(argv[2]);
    if (!img->sectorData(t, sec)) { std::cerr << "Error: no sector " << t << "/" << sec << "\n"; return 1; }
    bool tty = isatty(0) && isatty(1);
    struct termios saved{};
    if (tty) {
        tcgetattr(0, &saved);
        struct termios raw = saved;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &raw);
    }
    int cur = 0, nibble = -1;
    bool dirty = false, quitArmed = false;
    std::string status;
    auto draw = [&]() {
        if (!tty) return;
        const uint8_t* d = img->sectorData(t, sec);
        std::cout << "\x1b[H\x1b[2J" << argv[0] << "  track " << t << " sector " << sec
                  << (dirty ? "  (changed)" : "") << "\r\n\r\n";
        char buf[16];
        for (int row = 0; row < 16; row++) {
            std::snprintf(buf, sizeof buf, "%02X: ", row * 16);
            std::cout << buf;
            for (int c = 0; c < 16; c++) {
                int i = row * 16 + c;
                std::snprintf(buf, sizeof buf, "%02X", d[i]);
                if (i == cur) std::cout << "\x1b[7m" << buf << "\x1b[0m "; else std::cout << buf << " ";
            }
            std::cout << " ";
            for (int c = 0; c < 16; c++) {
                int i = row * 16 + c;
                std::string ch = petsciiGlyph(d[i] < 0x20 || (d[i] >= 0x80 && d[i] < 0xA0) ? 0x2E : d[i]);
                if (i == cur) std::cout << "\x1b[7m" << ch << "\x1b[0m"; else std::cout << ch;
            }
            std::cout << "\r\n";
        }
        std::cout << "\r\narrows move, hex digits set the byte, n/p next/previous sector, w write, q quit\r\n"
                  << status << std::flush;
    };
    auto step = [&](int dir) {
        int t2 = t, s2 = sec + dir;
        if (s2 >= img->sectorsOnTrack(t2)) { t2++; s2 = 0; }
        if (s2 < 0) { t2--; s2 = t2 >= 0 ? img->sectorsOnTrack(t2) - 1 : 0; }
        if (img->sectorData(t2, s2)) { t = t2; sec = s2; nibble = -1; }
    };
    int rc = 0;
    draw();
    while (true) {
        int ch = std::getchar();
        if (ch == EOF) break;
        status.clear();
        if (ch == 0x1B) {                                   // ESC [ A..D, 5~, 6~
            int c1 = std::getchar(), c2 = std::getchar();
            if (c1 == '[') {
                if (c2 == 'A') cur = (cur + 240) % 256;
                else if (c2 == 'B') cur = (cur + 16) % 256;
                else if (c2 == 'C') cur = (cur + 1) % 256;
                else if (c2 == 'D') cur = (cur + 255) % 256;
                else if (c2 == '5' || c2 == '6') { std::getchar(); step(c2 == '6' ? 1 : -1); }
                nibble = -1;
            }
        } else if (std::isxdigit(ch)) {
            int v = std::isdigit(ch) ? ch - '0' : std::tolower(ch) - 'a' + 10;
            uint8_t* d = img->sectorData(t, sec);
            if (nibble < 0) { d[cur] = (uint8_t)((v << 4) | (d[cur] & 0x0F)); nibble = v; }
            else { d[cur] = (uint8_t)((nibble << 4) | v); nibble = -1; cur = (cur + 1) % 256; }
            dirty = true;
        } else if (ch == 'n' || ch == 'p') {
            step(ch == 'n' ? 1 : -1);
        } else if (ch == 'w') {
            if (img->saveImage(argv[0])) { dirty = false; status = "written"; }
            else { status = "could not write the image"; rc = 1; }
        } else if (ch == 'q') {
            if (dirty && !quitArmed) { status = "unwritten changes: w writes them, q again discards them"; quitArmed = true; }
            else break;
            draw();
            continue;
        }
        quitArmed = false;
        draw();
    }
    if (tty) { tcsetattr(0, TCSANOW, &saved); std::cout << "\x1b[H\x1b[2J" << std::flush; }
    if (!tty && dirty) std::cerr << "disk45 edit: changes not written (no w)\n";
    return rc;
}

// The commands whose first argument is the image (the shell inserts the
// attached one)
static bool takesImage(const std::string& c) {
    static const std::set<std::string> cmds = {
        "list", "ls", "dir", "info", "add", "write", "extract", "read", "extract-all", "extractall",
        "remove", "delete", "rm", "rename", "mv", "lock", "unlock", "splat", "unsplat", "label",
        "validate", "check", "repair", "bam", "map", "dump", "hex", "mkdir", "rmdir", "tree",
        "basic", "dir-shrink", "dir-ensure", "rel-create", "rel-read", "rel-write", "rel-list",
        "bread", "bwrite", "bpeek", "bpoke", "bfill", "chain"};
    return cmds.count(c) > 0;
}

// disk45 shell: commands one per line, without reloading disk45; an
// attached image is the image of the commands that take one
static int cmdShell() {
    bool tty = isatty(0);
    std::string attached;
    std::vector<std::string> history;
    std::string line;
    int last = 0;
    if (tty) std::cout << "disk45 " << VERSION << " shell: help for the commands, quit to leave\n";
    while (true) {
        if (tty) std::cout << (attached.empty() ? std::string("disk45> ") : attached + "> ") << std::flush;
        if (!std::getline(std::cin, line)) break;
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;
        history.push_back(line.substr(start));
        if (line[start] == '!') {                       // a host shell command
            last = std::system(line.c_str() + start + 1) == 0 ? 0 : 1;
            continue;
        }
        std::vector<std::string> w = splitCommandLine(line.substr(start));
        if (w.empty()) continue;
        const std::string& c = w[0];
        if (c == "quit" || c == "exit" || c == "q") break;
        if (c == "help" || c == "?") {
            std::cout << "  attach <image>    the image of the following commands (detach: none)\n"
                         "  <command> [args]  any disk45 command; with an attached image it is left out\n"
                         "                    (list, add file.prg NAME, extract NAME out.prg, tree, ...)\n"
                         "  !<command>        run a host shell command\n"
                         "  history           the commands so far\n"
                         "  quit              leave the shell\n";
            continue;
        }
        if (c == "attach") {
            if (w.size() < 2) { std::cerr << "Usage: attach <image>\n"; last = 1; continue; }
            if (!DiskImage::load(w[1])) { std::cerr << "Error: cannot load " << w[1] << "\n"; last = 1; continue; }
            attached = w[1];
            last = 0;
            continue;
        }
        if (c == "detach") { attached.clear(); continue; }
        if (c == "history") {
            for (size_t i = 0; i + 1 < history.size(); i++) std::cout << "  " << i + 1 << "  " << history[i] << "\n";
            continue;
        }
        // the attached image goes in front of the arguments, unless the
        // first one already names an image
        if (!attached.empty() && takesImage(c)) {
            bool named = w.size() > 1 && DiskImage::isImageExtension(w[1].substr(0, w[1].find(':'))) &&
                         access(w[1].substr(0, w[1].find(':')).c_str(), F_OK) == 0;
            if (!named) w.insert(w.begin() + 1, attached);
        }
        std::vector<char*> args;
        std::string prog = "disk45";
        args.push_back(const_cast<char*>(prog.c_str()));
        for (auto& t : w) args.push_back(const_cast<char*>(t.c_str()));
        last = processCommand((int)args.size(), args.data());
    }
    if (tty) std::cout << "\n";
    return last;
}

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

        std::vector<std::string> tokens = splitCommandLine(line.substr(start));
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
    if (cmd == "repair")  return cmdRepair(subArgc, subArgv);
    if (cmd == "edit")    return cmdEdit(subArgc, subArgv);
    if (cmd == "basic")   return cmdBasic(subArgc, subArgv);
    if (cmd == "diff" || cmd == "compare") return cmdDiff(subArgc, subArgv);
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
    if (cmd == "sd")         return disk45_sd(subArgc, subArgv);
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
        std::cout << suiteVersionString("disk45") << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    usage();
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    // -v ... -vvvv anywhere: the log (a lone -v first stays --version)
    Logger::setTool("disk45");
    if (std::string(argv[1]) != "-v") argc = Logger::takeFlags(argc, argv);
    if (argc < 2) { usage(); return 1; }

    // Batch script mode: disk45 batch <scriptfile>
    std::string first = argv[1];
    if (first == "shell") return cmdShell();
    if (first == "batch" || first == "--batch") {
        if (argc < 3) { std::cerr << "Usage: disk45 batch <scriptfile>\n"; return 1; }
        return cmdBatch(argv[2]);
    }

    return processCommand(argc, argv);
}
