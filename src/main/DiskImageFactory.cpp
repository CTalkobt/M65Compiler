#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"
#include "ArkImage.hpp"
#include "ArcImage.hpp"
#include "LnxImage.hpp"
#include "TapImage.hpp"
#include "T64Image.hpp"
#include "G64Image.hpp"
#include "D80Image.hpp"
#include "GeosCvtImage.hpp"
#include "P00Image.hpp"
#include "X64Image.hpp"
#include "ZipcodeImage.hpp"
#include "D90Image.hpp"
#include "CmdImage.hpp"
#include "NibImage.hpp"
#include "GzipHelper.hpp"
#include "ZipArchive.hpp"
#include <cstdio>
#include <iterator>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

std::unique_ptr<DiskImage> DiskImage::create(DiskFormat fmt) {
    switch (fmt) {
        case DiskFormat::D64: return std::make_unique<D64Image>();
        case DiskFormat::D71: return std::make_unique<D71Image>();
        case DiskFormat::D81: return std::make_unique<D81Image>();
        case DiskFormat::D65: return std::make_unique<D65Image>();
    }
    return std::make_unique<D81Image>();
}

std::unique_ptr<DiskImage> DiskImage::createFromExtension(const std::string& path) {
    // A zip path: the format of the member (or of game.d81.zip's game.d81)
    std::string zip, member;
    if (zipPath(path, zip, member))
        return createFromExtension(member.empty() ? std::filesystem::path(zip).stem().string() : member);
    // Strip .gz for format detection
    std::string p = gzip::hasGzExtension(path) ? gzip::stripGzExtension(path) : path;
    std::string ext;
    auto dot = p.rfind('.');
    if (dot != std::string::npos) {
        ext = p.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }
    if (ext == ".ark") return std::make_unique<ArkImage>();
    if (ext == ".arc" || ext == ".sda") return std::make_unique<ArcImage>();
    if (ext == ".lnx") return std::make_unique<LnxImage>();
    if (ext == ".tap") return std::make_unique<TapImage>();
    if (ext == ".t64") return std::make_unique<T64Image>();
    if (ext == ".g64") return std::make_unique<G64Image>();
    if (ext == ".g71") return std::make_unique<G71Image>();
    if (ext == ".d80") return std::make_unique<D80Image>();
    if (ext == ".d82") return std::make_unique<D82Image>();
    if (ext == ".cvt") return std::make_unique<GeosCvtImage>();
    if (ext == ".x64") return std::make_unique<X64Image>();
    if (ext == ".p00" || ext == ".s00" || ext == ".u00" || ext == ".r00") return std::make_unique<P00Image>();
    // Check for P01-P99, S01-S99 etc.
    if (ext.size() == 4 && (ext[1] == 'p' || ext[1] == 's' || ext[1] == 'u' || ext[1] == 'r') &&
        std::isdigit(ext[2]) && std::isdigit(ext[3])) return std::make_unique<P00Image>();
    // Zipcode: detect if filename starts with "1!"
    if (ZipcodeImage::isZipcodeFile(p)) return std::make_unique<ZipcodeImage>();
    if (ext == ".d90") return std::make_unique<D90Image>(true);
    if (ext == ".d60") return std::make_unique<D90Image>(false);
    if (ext == ".d1m") return std::make_unique<CmdImage>(CmdImage::FD_1M);
    if (ext == ".d2m") return std::make_unique<CmdImage>(CmdImage::FD_2M);
    if (ext == ".d4m") return std::make_unique<CmdImage>(CmdImage::FD_4M);
    if (ext == ".dnp") return std::make_unique<CmdImage>(CmdImage::NATIVE);
    if (ext == ".nib" || ext == ".nbz") return std::make_unique<NibImage>();
    return create(formatFromExtension(path));
}

// A MEGA65 SD card image (an MBR with a FAT32 or MEGA65 system partition,
// far larger than any CBM image) is not a disk image: loading it as one (an
// unknown extension means D81) and saving would destroy the card
static bool looksLikeSdCard(const std::string& path) {
    std::error_code ec;
    if (std::filesystem::file_size(path, ec) < (32u << 20) || ec) return false;
    std::ifstream f(path, std::ios::binary);
    unsigned char b[512];
    if (!f.read((char*)b, 512) || b[510] != 0x55 || b[511] != 0xAA) return false;
    for (int i = 0; i < 4; i++) {
        unsigned char t = b[0x1be + 16 * i + 4];
        if (t == 0x0B || t == 0x0C || t == 0x41) return true;
    }
    return false;
}

std::unique_ptr<DiskImage> DiskImage::load(const std::string& path) {
    if (looksLikeSdCard(path)) {
        std::cerr << "Error: " << path << " is a MEGA65 SD card image; use disk45 sd (disk45 sd --help)\n";
        return nullptr;
    }
    std::string zip, member;
    if (zipPath(path, zip, member)) {
        ZipArchive za;
        std::string err;
        if (!za.load(zip, err)) { std::cerr << "Error: " << err << "\n"; return nullptr; }
        int idx = -1;
        if (!member.empty()) {
            idx = za.find(member);
        } else {
            // the member named like the zip (game.d81.zip: game.d81), else
            // its only disk image
            std::string stem = std::filesystem::path(zip).stem().string();
            idx = isImageExtension(stem) ? za.find(stem) : -1;
            if (idx < 0) {
                std::vector<int> images;
                for (size_t i = 0; i < za.members().size(); i++)
                    if (isImageExtension(za.members()[i].name)) images.push_back((int)i);
                if (images.size() == 1) idx = images[0];
                else if (images.size() > 1) {
                    std::cerr << "Error: " << zip << " holds several disk images; name one as " << zip << ":<member>:\n";
                    for (int i : images) std::cerr << "  " << za.members()[i].name << "\n";
                    return nullptr;
                }
            }
        }
        if (idx < 0) {
            std::cerr << "Error: no disk image " << (member.empty() ? "" : member + " ") << "in " << zip << "\n";
            return nullptr;
        }
        std::vector<uint8_t> data;
        if (!za.extract(idx, data, err)) { std::cerr << "Error: " << err << "\n"; return nullptr; }
        std::string name = za.members()[idx].name;
        std::string ext = std::filesystem::path(name).extension().string();
        std::string tmp = (std::filesystem::temp_directory_path() / "disk45-XXXXXX").string() + ext;
        std::vector<char> tmpl(tmp.begin(), tmp.end());
        tmpl.push_back('\0');
        int fd = mkstemps(tmpl.data(), (int)ext.size());
        if (fd < 0) { std::cerr << "Error: cannot create a temporary file\n"; return nullptr; }
        bool ok = write(fd, data.data(), data.size()) == (ssize_t)data.size();
        close(fd);
        std::string tmpPath(tmpl.data());
        std::unique_ptr<DiskImage> img = ok ? createFromExtension(tmpPath) : nullptr;
        if (img && !img->loadFromFile(tmpPath)) img.reset();
        std::remove(tmpPath.c_str());
        if (img) img->zipMember_ = name;
        return img;
    }
    auto img = createFromExtension(path);
    if (!img->loadFromFile(path)) return nullptr;
    return img;
}

bool DiskImage::isImageExtension(const std::string& name) {
    std::string p = gzip::hasGzExtension(name) ? gzip::stripGzExtension(name) : name;
    std::string ext = std::filesystem::path(p).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    static const char* exts[] = {".d64", ".d71", ".d81", ".d65", ".d80", ".d82", ".d90", ".d60", ".d1m",
                                 ".d2m", ".d4m", ".dnp", ".x64", ".g64", ".g71", ".nib", ".nbz", ".t64",
                                 ".tap", ".ark", ".arc", ".sda", ".lnx", ".p00", ".cvt"};
    for (const char* e : exts)
        if (ext == e) return true;
    return false;
}

bool DiskImage::zipPath(const std::string& path, std::string& zip, std::string& member) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t at = lower.find(".zip:");
    if (at != std::string::npos) {
        zip = path.substr(0, at + 4);
        member = path.substr(at + 5);
        return true;
    }
    if (lower.size() > 4 && lower.compare(lower.size() - 4, 4, ".zip") == 0) {
        zip = path;
        member.clear();
        return true;
    }
    return false;
}

bool DiskImage::saveImage(const std::string& path) const {
    std::string zip, member;
    if (!zipPath(path, zip, member)) return saveToFile(path);
    if (member.empty()) member = zipMember_;
    if (member.empty()) member = std::filesystem::path(zip).stem().string();  // game.d81.zip: game.d81
    if (!isImageExtension(member)) {
        std::cerr << "Error: name the disk image inside the zip: " << zip << ":<name.d81>, or use <name>.d81.zip\n";
        return false;
    }
    ZipArchive za;
    std::string err;
    std::error_code ec;
    if (std::filesystem::exists(zip, ec) && !za.load(zip, err)) { std::cerr << "Error: " << err << "\n"; return false; }
    std::string ext = std::filesystem::path(member).extension().string();
    std::string tmp = (std::filesystem::temp_directory_path() / "disk45-XXXXXX").string() + ext;
    std::vector<char> tmpl(tmp.begin(), tmp.end());
    tmpl.push_back('\0');
    int fd = mkstemps(tmpl.data(), (int)ext.size());
    if (fd < 0) { std::cerr << "Error: cannot create a temporary file\n"; return false; }
    close(fd);
    std::string tmpPath(tmpl.data());
    bool ok = saveToFile(tmpPath);
    std::vector<uint8_t> data;
    if (ok) {
        std::ifstream f(tmpPath, std::ios::binary);
        data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    std::remove(tmpPath.c_str());
    if (!ok) return false;
    za.put(member, data);
    if (!za.save(zip, err)) { std::cerr << "Error: " << err << "\n"; return false; }
    return true;
}
