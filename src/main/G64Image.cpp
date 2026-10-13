#include "G64Image.hpp"
#include "GzipHelper.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

namespace {

bool readRaw(const std::string& path, std::vector<uint8_t>& raw) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto size = f.tellg();
    f.seekg(0);
    raw.resize((size_t)size);
    f.read(reinterpret_cast<char*>(raw.data()), size);
    if (!f.good()) return false;
    if (gzip::isGzipped(raw)) {
        raw = gzip::decompress(raw);
        if (raw.empty()) return false;
    }
    return true;
}

bool writeRaw(const std::string& path, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out = gzip::hasGzExtension(path) ? gzip::compress(data) : data;
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), out.size());
    return f.good();
}

}  // namespace

bool G64Image::isG64File(const std::vector<uint8_t>& data) {
    return GcrTracks::isGcrFile(data);
}

void G64Image::format(const std::string& diskName, const std::string& diskId) {
    D64Image::format(diskName, diskId);
    tracks_.reset(1);
}

bool G64Image::loadFromFile(const std::string& path) {
    std::vector<uint8_t> raw;
    bool doubleSided = false;
    if (!readRaw(path, raw) || !GcrTracks::isGcrFile(raw, &doubleSided)) return false;
    if (doubleSided) {
        std::cerr << "Error: " << path << " is a G71 (1571) image: use the .g71 extension\n";
        return false;
    }
    D64Image::format("", "");
    return tracks_.load(raw, *this, 1);
}

bool G64Image::saveToFile(const std::string& path) const {
    return writeRaw(path, tracks_.save(*this));
}

void G71Image::format(const std::string& diskName, const std::string& diskId) {
    D71Image::format(diskName, diskId);
    tracks_.reset(2);
}

bool G71Image::loadFromFile(const std::string& path) {
    std::vector<uint8_t> raw;
    if (!readRaw(path, raw) || !GcrTracks::isGcrFile(raw)) return false;
    D71Image::format("", "");
    return tracks_.load(raw, *this, 2);
}

bool G71Image::saveToFile(const std::string& path) const {
    return writeRaw(path, tracks_.save(*this));
}
