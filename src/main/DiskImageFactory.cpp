#include "DiskImage.hpp"
#include "D64Image.hpp"
#include "D71Image.hpp"
#include "D81Image.hpp"
#include "D65Image.hpp"

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
    return create(formatFromExtension(path));
}

std::unique_ptr<DiskImage> DiskImage::load(const std::string& path) {
    auto img = createFromExtension(path);
    if (!img->loadFromFile(path)) return nullptr;
    return img;
}
