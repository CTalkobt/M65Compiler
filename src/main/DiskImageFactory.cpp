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
#include "SdCardImage.hpp"
#include "Logger.hpp"
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

// An image held in memory (a zip member, a file on an SD card), loaded
// through a temporary file with the member's extension
static std::unique_ptr<DiskImage> loadFromBytes(const std::string& name, const std::vector<uint8_t>& data) {
    std::string ext = std::filesystem::path(name).extension().string();
    std::string tmp = (std::filesystem::temp_directory_path() / "disk45-XXXXXX").string() + ext;
    std::vector<char> tmpl(tmp.begin(), tmp.end());
    tmpl.push_back('\0');
    int fd = mkstemps(tmpl.data(), (int)ext.size());
    if (fd < 0) { std::cerr << "Error: cannot create a temporary file\n"; return nullptr; }
    bool ok = write(fd, data.data(), data.size()) == (ssize_t)data.size();
    close(fd);
    std::string tmpPath(tmpl.data());
    std::unique_ptr<DiskImage> img = ok ? DiskImage::createFromExtension(tmpPath) : nullptr;
    if (img && !img->loadFromFile(tmpPath)) img.reset();
    std::remove(tmpPath.c_str());
    return img;
}

// The bytes an image saves as a file named like `name`
static bool imageBytes(const DiskImage& img, const std::string& name, std::vector<uint8_t>& data) {
    std::string ext = std::filesystem::path(name).extension().string();
    std::string tmp = (std::filesystem::temp_directory_path() / "disk45-XXXXXX").string() + ext;
    std::vector<char> tmpl(tmp.begin(), tmp.end());
    tmpl.push_back('\0');
    int fd = mkstemps(tmpl.data(), (int)ext.size());
    if (fd < 0) { std::cerr << "Error: cannot create a temporary file\n"; return false; }
    close(fd);
    std::string tmpPath(tmpl.data());
    bool ok = img.saveToFile(tmpPath);
    if (ok) {
        std::ifstream f(tmpPath, std::ios::binary);
        data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    std::remove(tmpPath.c_str());
    return ok;
}

bool DiskImage::isSdCard(const std::string& path) {
    return looksLikeSdCard(path);
}

bool DiskImage::cardPath(const std::string& path, std::string& card, std::string& member) {
    // "card.img:/DIR/NAME.D81": the part before a ':' is an SD card image
    for (size_t i = path.find(':'); i != std::string::npos; i = path.find(':', i + 1)) {
        std::string prefix = path.substr(0, i);
        std::error_code ec;
        if (i + 1 < path.size() && std::filesystem::is_regular_file(prefix, ec) && looksLikeSdCard(prefix)) {
            card = prefix;
            member = path.substr(i + 1);
            return true;
        }
    }
    return false;
}

bool DiskImage::partitionPath(const std::string& path, std::string& image, std::string& part) {
    auto colon = path.rfind(':');
    if (colon == std::string::npos || colon + 1 >= path.size()) return false;
    std::string file = path.substr(0, colon);
    std::string ext = std::filesystem::path(file).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    std::error_code ec;
    if ((ext != ".d1m" && ext != ".d2m" && ext != ".d4m") || !std::filesystem::is_regular_file(file, ec))
        return false;
    image = file;
    part = path.substr(colon + 1);
    return true;
}

// A partition of a CMD image ("disk.d2m:N")
static std::unique_ptr<DiskImage> loadPartition(const std::string& file, const std::string& spec) {
    auto img = DiskImage::createFromExtension(file);
    auto* cmd = dynamic_cast<CmdImage*>(img.get());
    if (!cmd || !cmd->loadFromFile(file)) return nullptr;
    const CmdImage::Partition* p = cmd->findPartition(spec);
    if (!p) {
        std::cerr << "Error: " << file << " has no partition \"" << spec << "\" (disk45 info lists them)\n";
        return nullptr;
    }
    if (p->type == 1) {
        if (!cmd->selectPartition(*p)) return nullptr;
        return img;
    }
    uint32_t bytes = CmdImage::emulatedBytes(p->type);
    if (!bytes || p->sectors * 256 < bytes) {
        std::cerr << "Error: partition " << p->number << " is a " << CmdImage::partitionTypeName(p->type)
                  << " partition, which disk45 does not open\n";
        return nullptr;
    }
    std::ifstream f(file, std::ios::binary);
    std::vector<uint8_t> data(bytes);
    f.seekg((std::streamoff)p->start * 256);
    if (!f.read(reinterpret_cast<char*>(data.data()), bytes)) return nullptr;
    return loadFromBytes(std::string("partition") + CmdImage::emulatedExtension(p->type), data);
}

// (-v: what was opened)
static void logOpened(const std::string& path, const DiskImage* img, const std::string& how) {
    if (!img || !Logger::isEnabled(LogLevel::INFO)) return;
    LOG_INFO(Logger::tool(), "opened " << path << how << ": \"" << img->diskName() << "\", "
             << img->totalTracks() << " tracks, " << img->freeSectors() << " of " << img->totalSectors()
             << " sectors free");
}

std::unique_ptr<DiskImage> DiskImage::load(const std::string& path) {
    auto img = loadImage(path);
    logOpened(path, img.get(), "");
    return img;
}

std::unique_ptr<DiskImage> DiskImage::loadImage(const std::string& path) {
    if (looksLikeSdCard(path)) {
        std::cerr << "Error: " << path << " is a MEGA65 SD card image; use disk45 sd (disk45 sd --help)\n";
        return nullptr;
    }
    std::string zip, member;
    if (partitionPath(path, zip, member)) {
        LOG_DEBUG(Logger::tool(), path << ": partition " << member << " of " << zip);
        return loadPartition(zip, member);
    }
    if (cardPath(path, zip, member)) {
        LOG_DEBUG(Logger::tool(), path << ": " << member << " on the SD card image " << zip);
        SdCardImage card;
        std::string err;
        std::vector<uint8_t> data;
        if (!card.open(zip, false, err) || !card.readFile(member, data, err)) {
            std::cerr << "Error: " << err << "\n";
            return nullptr;
        }
        auto img = loadFromBytes(member, data);
        if (img) img->zipMember_ = member;
        return img;
    }
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
        LOG_DEBUG(Logger::tool(), path << ": " << name << " (" << data.size() << " bytes) from the zip " << zip);
        auto img = loadFromBytes(name, data);
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
    LOG_INFO(Logger::tool(), "writing " << path << ": \"" << diskName() << "\", " << freeSectors() << " of "
             << totalSectors() << " sectors free");
    std::string zip, member;
    if (partitionPath(path, zip, member)) {
        // a native partition is part of the whole image; an emulated one's
        // bytes go back where they came from
        if (dynamic_cast<const CmdImage*>(this)) return saveToFile(zip);
        auto whole = createFromExtension(zip);
        auto* cmd = dynamic_cast<CmdImage*>(whole.get());
        const CmdImage::Partition* p = cmd && cmd->loadFromFile(zip) ? cmd->findPartition(member) : nullptr;
        std::vector<uint8_t> data;
        if (!p || !imageBytes(*this, std::string("p") + CmdImage::emulatedExtension(p->type), data) ||
            data.size() != CmdImage::emulatedBytes(p->type)) {
            std::cerr << "Error: cannot write partition " << member << " of " << zip << "\n";
            return false;
        }
        std::fstream f(zip, std::ios::binary | std::ios::in | std::ios::out);
        f.seekp((std::streamoff)p->start * 256);
        return (bool)f.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    if (cardPath(path, zip, member)) {
        std::vector<uint8_t> data;
        SdCardImage card;
        std::string err;
        if (!imageBytes(*this, member, data)) return false;
        if (!card.open(zip, true, err) || !card.writeFile(member, data, true, err) || !card.flush(err)) {
            std::cerr << "Error: " << err << "\n";
            return false;
        }
        return true;
    }
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
    std::vector<uint8_t> data;
    if (!imageBytes(*this, member, data)) return false;
    za.put(member, data);
    if (!za.save(zip, err)) { std::cerr << "Error: " << err << "\n"; return false; }
    return true;
}
