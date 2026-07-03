#include "X64Image.hpp"
#include "GzipHelper.hpp"
#include <cstring>
#include <fstream>

const uint8_t X64Image::X64_MAGIC[4] = {0x43, 0x15, 0x41, 0x64};

bool X64Image::isX64File(const std::vector<uint8_t>& data) {
    if (data.size() < X64_HEADER_SIZE) return false;
    return std::memcmp(data.data(), X64_MAGIC, 4) == 0;
}

bool X64Image::loadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto fileSize = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> raw((size_t)fileSize);
    f.read(reinterpret_cast<char*>(raw.data()), fileSize);
    if (!f.good()) return false;

    if (gzip::isGzipped(raw)) {
        raw = gzip::decompress(raw);
        if (raw.empty()) return false;
    }

    if (!isX64File(raw)) return false;

    // Strip the 64-byte header, load as D64
    image_.assign(raw.begin() + X64_HEADER_SIZE, raw.end());
    return true;
}

bool X64Image::saveToFile(const std::string& path) const {
    // Prepend X64 header when saving
    std::vector<uint8_t> out(X64_HEADER_SIZE, 0);
    std::memcpy(out.data(), X64_MAGIC, 4);
    out[4] = 0x02; out[5] = 0x06; // version
    out[6] = 0x01; // 1541
    out[7] = 35;   // tracks
    out.insert(out.end(), image_.begin(), image_.end());

    if (gzip::hasGzExtension(path)) {
        auto compressed = gzip::compress(out);
        if (compressed.empty()) return false;
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        return f.good();
    }
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), out.size());
    return f.good();
}
