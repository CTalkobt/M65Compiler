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
    if (ext == ".g71") return std::make_unique<G64Image>();
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

std::unique_ptr<DiskImage> DiskImage::load(const std::string& path) {
    auto img = createFromExtension(path);
    if (!img->loadFromFile(path)) return nullptr;
    return img;
}
