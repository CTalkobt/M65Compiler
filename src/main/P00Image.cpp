#include "P00Image.hpp"
#include <cstring>
#include <algorithm>

static const char P00_MAGIC[] = "C64File";

bool P00Image::isP00File(const std::vector<uint8_t>& data) {
    if (data.size() < P00_HEADER_SIZE) return false;
    return std::memcmp(data.data(), P00_MAGIC, 7) == 0 && data[7] == 0;
}

std::string P00Image::originalName() const {
    if (image_.size() < P00_HEADER_SIZE) return "FILE";
    std::string name(reinterpret_cast<const char*>(image_.data() + 8), 16);
    while (!name.empty() && (name.back() == 0 || (unsigned char)name.back() == 0xA0))
        name.pop_back();
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name.empty() ? "FILE" : name;
}

std::string P00Image::diskName() const {
    return "P00: " + originalName();
}

void P00Image::format(const std::string& diskName, const std::string&) {
    image_.assign(P00_HEADER_SIZE, 0);
    std::memcpy(image_.data(), P00_MAGIC, 7);
    image_[7] = 0;
    std::string name = diskName.empty() ? "FILE" : diskName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    int len = std::min((int)name.size(), 16);
    std::memcpy(image_.data() + 8, name.data(), len);
}

std::vector<FileInfo> P00Image::listFiles() const {
    if (image_.size() <= P00_HEADER_SIZE) return {};
    FileInfo fi;
    fi.name = originalName();
    fi.type = CbmFileType::PRG;
    fi.sizeInBytes = (uint32_t)(image_.size() - P00_HEADER_SIZE);
    fi.sizeInSectors = (fi.sizeInBytes + 253) / 254;
    fi.closed = true;
    return {fi};
}

bool P00Image::fileExists(const std::string& name) const {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return upper == originalName();
}

std::vector<uint8_t> P00Image::readFile(const std::string&) const {
    if (image_.size() <= P00_HEADER_SIZE) return {};
    return std::vector<uint8_t>(image_.begin() + P00_HEADER_SIZE, image_.end());
}

bool P00Image::addFile(const std::string& name, CbmFileType, const std::vector<uint8_t>& data) {
    // P00 is single-file — overwrite
    image_.resize(P00_HEADER_SIZE);
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    std::memset(image_.data() + 8, 0, 16);
    int len = std::min((int)upper.size(), 16);
    std::memcpy(image_.data() + 8, upper.data(), len);
    image_.insert(image_.end(), data.begin(), data.end());
    return true;
}

bool P00Image::removeFile(const std::string&) {
    image_.resize(P00_HEADER_SIZE);
    return true;
}
