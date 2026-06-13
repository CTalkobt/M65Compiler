#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

static const char* VERSION = "1.0.0";

static void usage() {
    std::cerr << "disk45 — CBM Disk Image Utility v" << VERSION << "\n"
              << "Usage:\n"
              << "  disk45 create <image> [-n name] [-i id]   Create empty disk image\n"
              << "  disk45 list <image>                       List directory\n"
              << "  disk45 info <image>                       Show disk info\n"
              << "  disk45 add <image> <file> [cbm_name]      Add file to image\n"
              << "  disk45 extract <image> <cbm_name> <file>  Extract file from image\n"
              << "  disk45 remove <image> <cbm_name>          Delete file from image\n"
              << "\n"
              << "Supported formats (auto-detected from extension):\n"
              << "  .d64  C64 1541 (170KB, 35 tracks)\n"
              << "  .d71  C128 1571 (340KB, 70 tracks)\n"
              << "  .d81  C65/MEGA65 1581 (800KB, 80 tracks)\n"
              << "  .d65  MEGA65 native (1.6MB, 162 tracks)\n"
              << "  .ark  Arkive (uncompressed archive)\n"
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
    if (cmd == "--version" || cmd == "-v") {
        std::cout << "disk45 v" << VERSION << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    usage();
    return 1;
}
