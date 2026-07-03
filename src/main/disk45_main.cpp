#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
#include "TapImage.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

static const char* VERSION = "1.0.0";

static void usage() {
    std::cerr << "disk45 — CBM Disk Image Utility v" << VERSION << "\n"
              << "Usage:\n"
              << "  disk45 create <image> [-n name] [-i id]   Create empty disk image\n"
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
              << "  disk45 label <image> [-n name] [-i id]    Change disk name/ID\n"
              << "  disk45 validate <image>                   Check BAM consistency\n"
              << "  disk45 bam <image>                        Visual BAM sector map\n"
              << "  disk45 dump <image> [track] [sector]      Hex dump sector(s)\n"
              << "  disk45 bread <image> <t> <s> [file]       Read raw sector\n"
              << "  disk45 bwrite <image> <t> <s> <file>      Write raw sector\n"
              << "  disk45 bpeek <image> <t> <s> <off> [n]    Read byte(s)\n"
              << "  disk45 bpoke <image> <t> <s> <off> <val>  Write byte(s)\n"
              << "  disk45 bfill <image> <t> <s> <val>        Fill sector with value\n"
              << "  disk45 chain <image> <filename>           Show file sector chain\n"
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

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-n" && i + 1 < argc) diskName = argv[++i];
        else if (std::string(argv[i]) == "-i" && i + 1 < argc) diskId = argv[++i];
    }

    auto img = DiskImage::createFromExtension(imagePath);
    img->format(diskName, diskId);
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Created " << formatStr(img->diskFormat()) << " image: "
              << imagePath << " (" << img->totalBytes() << " bytes)\n";
    return 0;
}

static int cmdList(int argc, char** argv) {
    if (argc < 1) { usage(); return 1; }
    auto img = DiskImage::load(argv[0]);
    if (!img) { std::cerr << "Error: failed to load " << argv[0] << "\n"; return 1; }

    std::string pattern = (argc >= 2) ? argv[1] : "";

    std::cout << "0 \"" << img->diskName() << "\" " << img->diskId() << "\n";
    auto files = filterFiles(img->listFiles(), pattern);
    for (auto& f : files) {
        std::cout << f.sizeInSectors << "\t\"" << f.name << "\"\t" << fileTypeStr(f.type);
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
    if (tap) {
        std::cout << "Format:       TAP (tape image)\n"
                  << "TAP version:  " << tap->tapVersion() << "\n"
                  << "Platform:     " << tap->platformName() << "\n"
                  << "Data size:    " << (img->totalBytes()) << " bytes\n"
                  << "Files:        " << img->listFiles().size() << "\n";
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

static int cmdRename(int argc, char** argv) {
    if (argc < 3) { usage(); return 1; }
    std::string imagePath = argv[0];
    std::string oldName = argv[1];
    std::string newName = argv[2];

    auto img = DiskImage::load(imagePath);
    if (!img) { std::cerr << "Error: failed to load " << imagePath << "\n"; return 1; }

    if (!img->renameFile(oldName, newName)) {
        std::cerr << "Error: rename failed (file not found or target name exists)\n";
        return 1;
    }
    if (!img->saveToFile(imagePath)) {
        std::cerr << "Error: failed to write " << imagePath << "\n";
        return 1;
    }
    std::cout << "Renamed \"" << oldName << "\" → \"" << newName << "\"\n";
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
// Main
// ============================================================================

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    std::string cmd = argv[1];
    int subArgc = argc - 2;
    char** subArgv = argv + 2;

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
    if (cmd == "label")   return cmdLabel(subArgc, subArgv);
    if (cmd == "validate" || cmd == "check")
                          return cmdValidate(subArgc, subArgv);
    if (cmd == "bam" || cmd == "map")
                          return cmdBAM(subArgc, subArgv);
    if (cmd == "dump" || cmd == "hex")
                          return cmdDump(subArgc, subArgv);
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
