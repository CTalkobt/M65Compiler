#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include "O45Archive.hpp"
#include "O45Reader.hpp"

static std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
}

static bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

// Extract just the filename from a path
static std::string basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos != std::string::npos) ? path.substr(pos + 1) : path;
}

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <command> <archive.lib> [files...]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  c   Create archive from .o45 files" << std::endl;
    std::cout << "  t   List archive contents" << std::endl;
    std::cout << "  x   Extract all members to current directory" << std::endl;
    std::cout << "  s   Print symbol index" << std::endl;
    std::cout << "  a   Add .o45 files to existing archive" << std::endl;
    std::cout << "  d   Delete members from archive" << std::endl;
    std::cout << "  -?  Display this help message" << std::endl;
}

// Build the symbol index by scanning each member's .o45 exports
static void buildSymbolIndex(Ar45Archive& archive) {
    archive.symbols.clear();
    for (uint32_t i = 0; i < (uint32_t)archive.members.size(); i++) {
        O45File obj;
        std::string err;
        if (!O45Reader::read(archive.members[i].data, obj, err)) continue;
        for (const auto& exp : obj.exports) {
            archive.symbols.push_back({exp.name, i});
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "-?" || std::string(argv[1]) == "--help") {
        printUsage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    std::string command = argv[1];

    if (argc < 3) {
        std::cerr << "ar45: missing archive name" << std::endl;
        return 1;
    }
    std::string archivePath = argv[2];

    // === CREATE ===
    if (command == "c") {
        if (argc < 4) {
            std::cerr << "ar45: c: no input files" << std::endl;
            return 1;
        }

        Ar45Archive archive;
        for (int i = 3; i < argc; i++) {
            auto data = readFile(argv[i]);
            if (data.empty()) {
                std::cerr << "ar45: cannot read " << argv[i] << std::endl;
                return 1;
            }
            archive.members.push_back({basename(argv[i]), data});
        }

        buildSymbolIndex(archive);

        auto blob = Ar45Writer::write(archive);
        if (!writeFile(archivePath, blob)) {
            std::cerr << "ar45: cannot write " << archivePath << std::endl;
            return 1;
        }

        std::cout << "Created " << archivePath << " (" << archive.members.size()
                  << " member" << (archive.members.size() != 1 ? "s" : "")
                  << ", " << archive.symbols.size() << " symbol"
                  << (archive.symbols.size() != 1 ? "s" : "") << ", "
                  << blob.size() << " bytes)" << std::endl;
        return 0;
    }

    // Commands that read an existing archive
    auto archiveData = readFile(archivePath);
    if (archiveData.empty()) {
        std::cerr << "ar45: cannot open " << archivePath << std::endl;
        return 1;
    }

    Ar45Archive archive;
    std::string err;
    if (!Ar45Reader::read(archiveData, archive, err)) {
        std::cerr << "ar45: " << archivePath << ": " << err << std::endl;
        return 1;
    }

    // === LIST ===
    if (command == "t") {
        for (const auto& member : archive.members) {
            std::cout << member.name << " (" << member.data.size() << " bytes)" << std::endl;
        }
        return 0;
    }

    // === SYMBOL INDEX ===
    if (command == "s") {
        for (const auto& sym : archive.symbols) {
            std::cout << sym.name << " [" << archive.members[sym.memberIndex].name << "]" << std::endl;
        }
        return 0;
    }

    // === EXTRACT ===
    if (command == "x") {
        for (const auto& member : archive.members) {
            if (!writeFile(member.name, member.data)) {
                std::cerr << "ar45: cannot write " << member.name << std::endl;
                return 1;
            }
            std::cout << "x " << member.name << " (" << member.data.size() << " bytes)" << std::endl;
        }
        return 0;
    }

    // === ADD ===
    if (command == "a") {
        if (argc < 4) {
            std::cerr << "ar45: a: no input files" << std::endl;
            return 1;
        }
        for (int i = 3; i < argc; i++) {
            auto data = readFile(argv[i]);
            if (data.empty()) {
                std::cerr << "ar45: cannot read " << argv[i] << std::endl;
                return 1;
            }
            std::string name = basename(argv[i]);
            // Replace existing member with same name
            bool replaced = false;
            for (auto& member : archive.members) {
                if (member.name == name) {
                    member.data = data;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) archive.members.push_back({name, data});
        }
        buildSymbolIndex(archive);
        auto blob = Ar45Writer::write(archive);
        if (!writeFile(archivePath, blob)) {
            std::cerr << "ar45: cannot write " << archivePath << std::endl;
            return 1;
        }
        std::cout << "Updated " << archivePath << " (" << archive.members.size()
                  << " member" << (archive.members.size() != 1 ? "s" : "")
                  << ", " << archive.symbols.size() << " symbol"
                  << (archive.symbols.size() != 1 ? "s" : "") << ")" << std::endl;
        return 0;
    }

    // === DELETE ===
    if (command == "d") {
        if (argc < 4) {
            std::cerr << "ar45: d: no members specified" << std::endl;
            return 1;
        }
        for (int i = 3; i < argc; i++) {
            std::string target = argv[i];
            auto it = std::remove_if(archive.members.begin(), archive.members.end(),
                                     [&](const Ar45Member& m) { return m.name == target; });
            if (it == archive.members.end()) {
                std::cerr << "ar45: " << target << ": not found in archive" << std::endl;
            }
            archive.members.erase(it, archive.members.end());
        }
        buildSymbolIndex(archive);
        auto blob = Ar45Writer::write(archive);
        if (!writeFile(archivePath, blob)) {
            std::cerr << "ar45: cannot write " << archivePath << std::endl;
            return 1;
        }
        std::cout << "Updated " << archivePath << " (" << archive.members.size()
                  << " member" << (archive.members.size() != 1 ? "s" : "") << ")" << std::endl;
        return 0;
    }

    std::cerr << "ar45: unknown command '" << command << "'" << std::endl;
    printUsage(argv[0]);
    return 1;
}
