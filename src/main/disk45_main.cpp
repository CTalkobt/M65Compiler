#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
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
              << "  disk45 remove <image> <cbm_name>          Delete file from image\n"
              << "  disk45 rename <image> <old> <new>         Rename file in image\n"
              << "  disk45 label <image> [-n name] [-i id]    Change disk name/ID\n"
              << "  disk45 validate <image>                   Check BAM consistency\n"
              << "  disk45 bam <image>                        Visual BAM sector map\n"
              << "  disk45 dump <image> [track] [sector]      Hex dump sector(s)\n"
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

    std::cout << "0 \"" << img->diskName() << "\" " << img->diskId() << "\n";
    auto files = img->listFiles();
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

    std::cout << "Format:       " << formatStr(img->diskFormat()) << "\n"
              << "Disk name:    " << img->diskName() << "\n"
              << "Disk ID:      " << img->diskId() << "\n"
              << "Tracks:       " << img->totalTracks() << "\n"
              << "Total sectors:" << img->totalSectors() << "\n"
              << "Free sectors: " << img->freeSectors() << "\n"
              << "Image size:   " << img->totalBytes() << " bytes\n"
              << "Files:        " << img->listFiles().size() << "\n";
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
    if (cmd == "remove" || cmd == "delete" || cmd == "rm")
                          return cmdRemove(subArgc, subArgv);
    if (cmd == "rename" || cmd == "mv")
                          return cmdRename(subArgc, subArgv);
    if (cmd == "label")   return cmdLabel(subArgc, subArgv);
    if (cmd == "validate" || cmd == "check")
                          return cmdValidate(subArgc, subArgv);
    if (cmd == "bam" || cmd == "map")
                          return cmdBAM(subArgc, subArgv);
    if (cmd == "dump" || cmd == "hex")
                          return cmdDump(subArgc, subArgv);
    if (cmd == "--version" || cmd == "-v") {
        std::cout << "disk45 v" << VERSION << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    usage();
    return 1;
}
