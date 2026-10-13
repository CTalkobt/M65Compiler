#include "SdCardImage.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <set>

namespace {

const uint32_t EOC = 0x0FFFFFFF;
const uint32_t BAD = 0x0FFFFFF7;

uint16_t get16(const uint8_t* p) { return p[0] | (p[1] << 8); }
uint32_t get32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
void put16(uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
void put32(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (v >> (8 * i)) & 0xFF; }

// mega65-fdisk's boot sector (as copied by xemu's sdcontent.c): BPB for 512
// byte sectors, 8 sectors per cluster, 568 reserved sectors, 2 FATs, root
// directory in cluster 2, FS information sector 1, backup boot sector 6;
// volume label "M.E.G.A. 65", boot code printing "MEGA65 HYPPOBOOT V00.11"
const uint8_t bootBytes[258] = {
    0xeb, 0x58, 0x90, 0x4d, 0x45, 0x47, 0x41, 0x36, 0x35, 0x72, 0x31, 0x00, 0x02, 0x08, 0x38, 0x02,
    0x02, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe8, 0x0f, 0x00, 0xf8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x29, 0x6d, 0x66, 0x62, 0x61, 0x4d, 0x2e, 0x45, 0x2e, 0x47, 0x2e, 0x41, 0x2e, 0x20,
    0x36, 0x35, 0x46, 0x41, 0x54, 0x33, 0x32, 0x20, 0x20, 0x20, 0x0e, 0x1f, 0xbe, 0x77, 0x7c, 0xac,
    0x22, 0xc0, 0x74, 0x0b, 0x56, 0xb4, 0x0e, 0xbb, 0x07, 0x00, 0xcd, 0x10, 0x5e, 0xeb, 0xf0, 0x32,
    0xe4, 0xcd, 0x16, 0xcd, 0x19, 0xeb, 0xfe, 0x4d, 0x45, 0x47, 0x41, 0x36, 0x35, 0x20, 0x48, 0x59,
    0x50, 0x50, 0x4f, 0x42, 0x4f, 0x4f, 0x54, 0x20, 0x56, 0x30, 0x30, 0x2e, 0x31, 0x31, 0x0d, 0x0a,
    0x0d, 0x3f, 0x4e, 0x4f, 0x20, 0x34, 0x35, 0x47, 0x53, 0x30, 0x32, 0x2c, 0x20, 0x34, 0x35, 0x31,
    0x30, 0x2c, 0x20, 0x36, 0x35, 0x5b, 0x63, 0x65, 0x5d, 0x30, 0x32, 0x2c, 0x20, 0x36, 0x35, 0x31,
    0x30, 0x20, 0x4f, 0x52, 0x20, 0x38, 0x35, 0x31, 0x30, 0x20, 0x50, 0x52, 0x4f, 0x43, 0x45, 0x53,
    0x53, 0x4f, 0x52, 0x20, 0x20, 0x45, 0x52, 0x52, 0x4f, 0x52, 0x0d, 0x0a, 0x49, 0x4e, 0x53, 0x45,
    0x52, 0x54, 0x20, 0x44, 0x49, 0x53, 0x4b, 0x20, 0x49, 0x4e, 0x20, 0x52, 0x45, 0x41, 0x4c, 0x20,
    0x43, 0x4f, 0x4d, 0x50, 0x55, 0x54, 0x45, 0x52, 0x20, 0x41, 0x4e, 0x44, 0x20, 0x54, 0x52, 0x59,
    0x20, 0x41, 0x47, 0x41, 0x49, 0x4e, 0x2e, 0x0a, 0x0a, 0x52, 0x45, 0x41, 0x44, 0x59, 0x2e, 0x0d,
    0x0a,
};

// The root directory's volume label entry after the 11-byte name
const uint8_t labelEntry[15] = {0x08, 0x00, 0x00, 0x53, 0xae, 0x93, 0x4a, 0x93,
                                0x4a, 0x00, 0x00, 0x53, 0xae, 0x93, 0x4a};

void fatNow(uint16_t& date, uint16_t& time) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    int y = std::max(0, tm.tm_year + 1900 - 1980);
    date = (uint16_t)((y << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday);
    time = (uint16_t)((tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2));
}

std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

bool iequal(const std::string& a, const std::string& b) { return upper(a) == upper(b); }

// UTF-8 <-> UCS-2 for long names (invalid UTF-8 bytes are taken as Latin-1)
std::vector<uint16_t> toUcs2(const std::string& s) {
    std::vector<uint16_t> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        if (c < 0x80) { out.push_back(c); i++; continue; }
        int n = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : 0;
        bool ok = n && i + n < s.size() + 0;
        uint32_t cp = n == 1 ? (c & 0x1F) : (c & 0x0F);
        for (int k = 1; ok && k <= n; k++) {
            if (i + k >= s.size() || (s[i + k] & 0xC0) != 0x80) ok = false;
            else cp = (cp << 6) | (s[i + k] & 0x3F);
        }
        if (ok) { out.push_back((uint16_t)cp); i += n + 1; }
        else { out.push_back(c); i++; }
    }
    return out;
}

std::string fromUcs2(const std::vector<uint16_t>& u) {
    std::string out;
    for (uint16_t c : u) {
        if (c < 0x80) out += (char)c;
        else if (c < 0x800) { out += (char)(0xC0 | (c >> 6)); out += (char)(0x80 | (c & 0x3F)); }
        else {
            out += (char)(0xE0 | (c >> 12));
            out += (char)(0x80 | ((c >> 6) & 0x3F));
            out += (char)(0x80 | (c & 0x3F));
        }
    }
    return out;
}

uint8_t lfnChecksum(const uint8_t* shortName) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) sum = (uint8_t)(((sum & 1) << 7) + (sum >> 1) + shortName[i]);
    return sum;
}

// "NAME.EXT" -> 11 padded bytes
void packShort(const std::string& s, uint8_t* out) {
    std::memset(out, ' ', 11);
    size_t dot = s.find('.');
    std::string base = s.substr(0, dot), ext = dot == std::string::npos ? "" : s.substr(dot + 1);
    std::memcpy(out, base.data(), std::min<size_t>(8, base.size()));
    std::memcpy(out + 8, ext.data(), std::min<size_t>(3, ext.size()));
    if (out[0] == 0xE5) out[0] = 0x05;
}

bool shortChar(char c) {
    return std::isalnum((unsigned char)c) || std::strchr("!#$%&'()-@^_`{}~", c);
}

void splitPath(const std::string& path, std::string& parent, std::string& leaf) {
    std::string p = path;
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    size_t s = p.rfind('/');
    if (s == std::string::npos) { parent = ""; leaf = p; }
    else { parent = p.substr(0, s); leaf = p.substr(s + 1); }
}

}  // namespace

SdCardImage::~SdCardImage() {
    std::string err;
    if (writable_) flush(err);
}

bool SdCardImage::parseSize(const std::string& s, uint64_t& bytes) {
    if (s.empty() || !std::isdigit((unsigned char)s[0])) return false;
    size_t end = 0;
    unsigned long long v = std::stoull(s, &end);
    std::string unit = upper(s.substr(end));
    if (unit == "" || unit == "B") bytes = v;
    else if (unit == "K" || unit == "KB" || unit == "KIB") bytes = v << 10;
    else if (unit == "M" || unit == "MB" || unit == "MIB") bytes = v << 20;
    else if (unit == "G" || unit == "GB" || unit == "GIB") bytes = v << 30;
    else return false;
    return true;
}

const char* SdCardImage::partitionTypeName(uint8_t type) {
    switch (type) {
    case 0x0B: return "FAT32 (CHS)";
    case 0x0C: return "FAT32 (LBA)";
    case 0x41: return "MEGA65 system";
    case 0x06: case 0x0E: return "FAT16";
    case 0x07: return "NTFS/exFAT";
    case 0x83: return "Linux";
    default: return "other";
    }
}

bool SdCardImage::readSector(uint64_t lba, uint8_t* buf) {
    if (lba >= imageSectors_) return false;
    f_.clear();
    f_.seekg((std::streamoff)(lba * 512));
    f_.read((char*)buf, 512);
    return (bool)f_;
}

bool SdCardImage::writeSector(uint64_t lba, const uint8_t* buf) {
    if (!writable_ || lba >= imageSectors_) return false;
    f_.clear();
    f_.seekp((std::streamoff)(lba * 512));
    f_.write((const char*)buf, 512);
    return (bool)f_;
}

// ---------------------------------------------------------------------------
// Creating a card: mega65-fdisk's layout
// ---------------------------------------------------------------------------

bool SdCardImage::create(const std::string& path, const CreateOptions& opt, std::string& err) {
    if (opt.bytes % 512) { err = "the size must be a multiple of 512 bytes"; return false; }
    uint64_t total = opt.bytes / 512;
    if (total < 32 * 2048) { err = "an SD card image needs at least 32 MB"; return false; }
    if (total > 0xFFFFFFFFull) { err = "an MBR card holds at most 2 TB"; return false; }
    uint32_t device = (uint32_t)total;

    // Partitions as mega65-fdisk sizes them: FAT32 from sector 2048, the
    // system partition (half the card, at most 2 GB, whole MB) at the end
    uint32_t sysSects = 0;
    if (opt.sysPart) {
        if (opt.sysPartSectors) sysSects = opt.sysPartSectors & 0xFFFFF800u;
        else {
            sysSects = (device - 0x0800) >> 1;
            if (sysSects > 0x400000) sysSects = 0x400000;
            sysSects &= 0xFFFFF800u;
        }
        if (sysSects < 2048 * 2) { err = "the system partition needs at least 2 MB"; return false; }
        if ((uint64_t)sysSects + 0x0800 + 16 * 2048 > device) {
            err = "the system partition leaves less than 16 MB for the FAT32 partition";
            return false;
        }
    }
    uint32_t usrStart = 0x0800, usrSects = device - 0x0800 - sysSects;
    uint32_t sysStart = usrStart + usrSects;

    {
        std::ofstream o(path, std::ios::binary | std::ios::trunc);
        if (!o) { err = "cannot create " + path; return false; }
    }
    std::error_code ec;
    std::filesystem::resize_file(path, opt.bytes, ec);
    if (ec) { err = "cannot size " + path + ": " + ec.message(); return false; }

    SdCardImage img;
    img.f_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!img.f_) { err = "cannot open " + path; return false; }
    img.writable_ = true;
    img.imageSectors_ = total;
    uint8_t b[512];

    // MBR
    std::memset(b, 0, 512);
    b[0x1b8] = 0x83; b[0x1b9] = 0x7d; b[0x1ba] = 0xcb; b[0x1bb] = 0xa6;  // disk signature
    b[0x1c2] = 0x0c;
    put32(b + 0x1c6, usrStart);
    put32(b + 0x1ca, usrSects);
    if (sysSects) {
        b[0x1d2] = 0x41;
        put32(b + 0x1d6, sysStart);
        put32(b + 0x1da, sysSects);
    }
    b[0x1fe] = 0x55; b[0x1ff] = 0xaa;
    img.writeSector(0, b);

    // FAT32: as many 4 KB clusters as fit beside their two FATs
    const uint32_t spc = 8, reserved = 0x238;
    uint32_t avail = usrSects - reserved;
    uint32_t clusters = avail / spc;
    auto fatFor = [](uint32_t c) { return c / 128 + (c % 128 ? 1 : 0); };
    uint32_t fatSects = fatFor(clusters);
    uint32_t required = 2 * fatSects + (clusters - 2) * spc;
    while (required > avail) {
        uint32_t delta = std::max<uint32_t>(1, (required - avail) / (1 + spc));
        clusters -= delta;
        fatSects = fatFor(clusters);
        required = 2 * fatSects + (clusters - 2) * spc;
    }
    std::string label = opt.label.substr(0, 11);
    label.resize(11, ' ');
    label = upper(label);
    bool fdiskLabel = opt.label == CreateOptions().label;

    std::memset(b, 0, 512);
    std::memcpy(b, bootBytes, sizeof bootBytes);
    put32(b + 0x20, usrSects);
    put32(b + 0x24, fatSects);
    if (!fdiskLabel) std::memcpy(b + 0x47, label.data(), 11);  // FDISK leaves "M.E.G.A. 65"
    b[510] = 0x55; b[511] = 0xaa;
    img.writeSector(usrStart, b);
    img.writeSector(usrStart + 6, b);

    std::memset(b, 0, 512);                 // FS information sector
    b[0] = 0x52; b[1] = 0x52; b[2] = 0x61; b[3] = 0x41;
    b[0x1e4] = 0x72; b[0x1e5] = 0x72; b[0x1e6] = 0x41; b[0x1e7] = 0x61;
    put32(b + 0x1e8, 0xFFFFFFFF);           // free count / next free: unknown
    put32(b + 0x1ec, 0xFFFFFFFF);
    b[510] = 0x55; b[511] = 0xaa;
    img.writeSector(usrStart + 1, b);
    img.writeSector(usrStart + 7, b);

    std::memset(b, 0, 512);                 // FATs: media, reserved, root directory end
    const uint8_t fatBytes[12] = {0xf8, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0xf8, 0xff, 0xff, 0x0f};
    std::memcpy(b, fatBytes, 12);
    img.writeSector(usrStart + reserved, b);
    img.writeSector(usrStart + reserved + fatSects, b);

    std::memset(b, 0, 512);                 // root directory: the volume label
    std::memcpy(b, label.data(), 11);
    std::memcpy(b + 11, labelEntry, sizeof labelEntry);
    img.writeSector(usrStart + reserved + 2 * fatSects, b);

    if (sysSects) {
        // System partition: 1 MB reserved (header, configuration), then the
        // freeze slots and the service slots (512 KB each) with directories
        const uint32_t slotSize = 1024, reservedArea = 2048;
        uint32_t slots = (sysSects - reservedArea) / (slotSize * 2 + 1);
        if (slots >= 0xffff) slots = 0xffff;
        uint16_t dirSize = (uint16_t)(1 + slots / 4);
        std::memset(b, 0, 512);
        std::memcpy(b, "MEGA65SYS00", 11);
        put32(b + 0x10, 0);
        put32(b + 0x14, slotSize * slots + dirSize);
        put32(b + 0x18, slotSize);
        put16(b + 0x1c, (uint16_t)slots);
        put16(b + 0x1e, dirSize);
        put32(b + 0x20, slotSize * slots + dirSize);
        put32(b + 0x24, slotSize * slots + dirSize);
        put32(b + 0x28, slotSize);
        put16(b + 0x2c, (uint16_t)slots);
        put16(b + 0x2e, dirSize);
        img.writeSector(sysStart, b);
        // the rest (reserved area, slot directories) is already zero
    }

    // Configuration sector (FDISK's values; onboarding runs on the first
    // boot) in card sector 1, and in the system partition's sector 1
    std::memset(b, 0, 512);
    b[0x000] = 0x01; b[0x001] = 0x01;   // structure version
    b[0x002] = opt.ntsc ? 0x80 : 0x00;
    b[0x003] = 0x41;                    // audio amplifier on, mono to both channels
    b[0x004] = 0x00;                    // floppies from SD card images
    b[0x005] = 0x01;                    // Amiga mice as 1351
    std::memcpy(b + 0x006, opt.mac, 6);
    std::memcpy(b + 0x010, opt.defaultDisk.data(), std::min<size_t>(16, opt.defaultDisk.size()));
    b[0x020] = 0x01;                    // new DMAgic (F018B)
    img.writeSector(1, b);
    if (sysSects) img.writeSector(sysStart + 1, b);
    img.f_.flush();
    if (!img.f_) { err = "write error on " + path; return false; }
    img.writable_ = false;
    return true;
}

// ---------------------------------------------------------------------------
// Opening: the partition table and the FAT32 volume
// ---------------------------------------------------------------------------

bool SdCardImage::open(const std::string& path, bool writable, std::string& err) {
    std::error_code ec;
    uint64_t size = std::filesystem::file_size(path, ec);
    if (ec) { err = "cannot open " + path; return false; }
    f_.open(path, writable ? (std::ios::in | std::ios::out | std::ios::binary)
                           : (std::ios::in | std::ios::binary));
    if (!f_) { err = "cannot open " + path; return false; }
    writable_ = false;  // until loaded
    imageSectors_ = size / 512;

    uint8_t b[512];
    if (!readSector(0, b)) { err = path + " is too small for an SD card image"; return false; }
    bool superFloppy = std::memcmp(b + 0x52, "FAT32   ", 8) == 0 && get16(b + 0x0b) == 512;
    if (!superFloppy) {
        if (b[0x1fe] != 0x55 || b[0x1ff] != 0xaa) { err = path + " has no MBR partition table"; return false; }
        for (int i = 0; i < 4; i++) {
            const uint8_t* e = b + 0x1be + 16 * i;
            if (e[4] == 0) continue;
            Partition p;
            p.index = i;
            p.type = e[4];
            p.start = get32(e + 8);
            p.sectors = get32(e + 12);
            parts_.push_back(p);
        }
        for (const auto& p : parts_)
            if ((p.type == 0x0B || p.type == 0x0C) && !fatOk_) {
                volStart_ = p.start;
                volSectors_ = p.sectors;
                fatOk_ = true;
            }
    } else {
        volStart_ = 0;
        volSectors_ = (uint32_t)std::min<uint64_t>(imageSectors_, 0xFFFFFFFF);
        fatOk_ = true;
    }
    if (fatOk_ && !loadFat(err)) return false;
    writable_ = writable;
    return true;
}

bool SdCardImage::loadFat(std::string& err) {
    uint8_t b[512];
    if (!readSector(volStart_, b) || b[510] != 0x55 || b[511] != 0xaa || get16(b + 0x0b) != 512 ||
        get16(b + 0x16) != 0 || get32(b + 0x24) == 0) {
        err = "the FAT32 partition has no valid FAT32 boot sector";
        fatOk_ = false;
        return false;
    }
    spc_ = b[0x0d];
    reserved_ = get16(b + 0x0e);
    nFats_ = b[0x10];
    uint32_t total = get16(b + 0x13) ? get16(b + 0x13) : get32(b + 0x20);
    fatSize_ = get32(b + 0x24);
    rootCluster_ = get32(b + 0x2c);
    fsInfo_ = get16(b + 0x30);
    if (spc_ == 0 || (spc_ & (spc_ - 1)) || nFats_ == 0 || reserved_ == 0) {
        err = "the FAT32 boot sector has invalid parameters";
        fatOk_ = false;
        return false;
    }
    if (total < volSectors_ || volSectors_ == 0) volSectors_ = total;
    dataStart_ = (uint64_t)volStart_ + reserved_ + (uint64_t)nFats_ * fatSize_;
    uint64_t dataSects = (uint64_t)volStart_ + volSectors_ > dataStart_ ? volStart_ + volSectors_ - dataStart_ : 0;
    clusters_ = (uint32_t)std::min<uint64_t>(dataSects / spc_, (uint64_t)fatSize_ * 128 - 2);
    if (dataStart_ + (uint64_t)clusters_ * spc_ > imageSectors_) {
        err = "the image is shorter than its FAT32 file system";
        fatOk_ = false;
        return false;
    }
    fat_.assign(clusters_ + 2, 0);
    std::vector<uint8_t> buf(512);
    uint32_t needSects = (uint32_t)(((uint64_t)(clusters_ + 2) * 4 + 511) / 512);
    for (uint32_t s = 0; s < needSects; s++) {
        if (!readSector(volStart_ + reserved_ + s, buf.data())) { err = "cannot read the FAT"; return false; }
        for (int i = 0; i < 128; i++) {
            uint64_t c = (uint64_t)s * 128 + i;
            if (c < fat_.size()) fat_[c] = get32(buf.data() + 4 * i);
        }
    }
    fatDirty_.assign(needSects, false);
    if (rootCluster_ < 2 || rootCluster_ >= clusters_ + 2) {
        err = "the FAT32 root directory cluster is invalid";
        return false;
    }
    return true;
}

bool SdCardImage::flush(std::string& err) {
    if (!writable_ || !fatOk_) return true;
    uint8_t b[512];
    for (size_t s = 0; s < fatDirty_.size(); s++) {
        if (!fatDirty_[s]) continue;
        // keep entries past the last cluster as they are
        readSector(volStart_ + reserved_ + s, b);
        for (int i = 0; i < 128; i++) {
            uint64_t c = (uint64_t)s * 128 + i;
            if (c < fat_.size()) put32(b + 4 * i, fat_[c]);
        }
        for (int k = 0; k < nFats_; k++)
            if (!writeSector(volStart_ + reserved_ + (uint64_t)k * fatSize_ + s, b)) {
                err = "cannot write the FAT";
                return false;
            }
        fatDirty_[s] = false;
    }
    if (fsInfoDirty_ && fsInfo_ && fsInfo_ != 0xFFFF) {
        if (readSector(volStart_ + fsInfo_, b) && get32(b) == 0x41615252 && get32(b + 0x1e4) == 0x61417272) {
            put32(b + 0x1e8, freeClusters());
            uint32_t next = 0xFFFFFFFF;
            for (uint32_t c = 2; c < clusters_ + 2; c++)
                if (fatEntry(c) == 0) { next = c; break; }
            put32(b + 0x1ec, next);
            writeSector(volStart_ + fsInfo_, b);
        }
        fsInfoDirty_ = false;
    }
    f_.flush();
    if (!f_) { err = "write error"; return false; }
    return true;
}

// ---------------------------------------------------------------------------
// FAT and cluster chains
// ---------------------------------------------------------------------------

void SdCardImage::setFat(uint32_t c, uint32_t v) {
    fat_[c] = (fat_[c] & 0xF0000000) | (v & 0x0FFFFFFF);
    fatDirty_[c / 128] = true;
    fsInfoDirty_ = true;
}

uint32_t SdCardImage::freeClusters() const {
    uint32_t n = 0;
    for (uint32_t c = 2; c < clusters_ + 2; c++)
        if (fatEntry(c) == 0) n++;
    return n;
}

std::vector<uint32_t> SdCardImage::chain(uint32_t c) const {
    std::vector<uint32_t> out;
    while (c >= 2 && c < clusters_ + 2 && out.size() <= clusters_) {
        out.push_back(c);
        uint32_t n = fatEntry(c);
        if (n >= 0x0FFFFFF8 || n == 0 || n == BAD) break;
        c = n;
    }
    return out;
}

std::vector<std::pair<uint32_t, uint32_t>> SdCardImage::runs(uint32_t cluster) const {
    std::vector<std::pair<uint32_t, uint32_t>> r;
    for (uint32_t c : chain(cluster)) {
        if (!r.empty() && r.back().first + r.back().second == c) r.back().second++;
        else r.push_back({c, 1});
    }
    return r;
}

bool SdCardImage::allocRun(uint32_t n, uint32_t& first) {
    first = 0;
    if (n == 0) return true;
    uint32_t run = 0;
    for (uint32_t c = 2; c < clusters_ + 2; c++) {
        if (fatEntry(c) != 0) { run = 0; continue; }
        if (++run == n) {
            first = c - n + 1;
            for (uint32_t k = 0; k < n; k++) setFat(first + k, k + 1 < n ? first + k + 1 : EOC);
            return true;
        }
    }
    return false;
}

void SdCardImage::freeChain(uint32_t c) {
    for (uint32_t k : chain(c)) setFat(k, 0);
}

bool SdCardImage::readChain(uint32_t c, std::vector<uint8_t>& buf) {
    buf.clear();
    for (auto [start, len] : runs(c)) {
        size_t bytes = (size_t)len * spc_ * 512;
        size_t at = buf.size();
        buf.resize(at + bytes);
        f_.clear();
        f_.seekg((std::streamoff)(clusterSector(start) * 512));
        f_.read((char*)buf.data() + at, (std::streamsize)bytes);
        if (!f_) return false;
    }
    return true;
}

bool SdCardImage::writeChain(uint32_t c, const std::vector<uint8_t>& buf) {
    size_t at = 0;
    size_t cb = (size_t)spc_ * 512;
    for (auto [start, len] : runs(c)) {
        if (at >= buf.size()) break;
        size_t bytes = std::min(buf.size() - at, (size_t)len * cb);
        f_.clear();
        f_.seekp((std::streamoff)(clusterSector(start) * 512));
        f_.write((const char*)buf.data() + at, (std::streamsize)bytes);
        if (bytes % cb) {
            std::vector<char> pad(cb - bytes % cb, 0);
            f_.write(pad.data(), (std::streamsize)pad.size());
        }
        if (!f_) return false;
        at += bytes;
    }
    return at >= buf.size();
}

// ---------------------------------------------------------------------------
// Directories
// ---------------------------------------------------------------------------

bool SdCardImage::readDir(uint32_t cluster, std::vector<uint8_t>& raw) {
    return readChain(cluster ? cluster : rootCluster_, raw);
}

std::vector<SdCardImage::Entry> SdCardImage::parseDir(const std::vector<uint8_t>& raw) const {
    std::vector<Entry> out;
    std::vector<uint16_t> lfn;
    int lfnCount = 0, lfnSeen = 0;
    uint8_t lfnSum = 0;
    uint32_t lfnFirst = 0;
    for (uint32_t slot = 0; slot * 32 < raw.size(); slot++) {
        const uint8_t* d = raw.data() + slot * 32;
        if (d[0] == 0x00) break;
        if (d[0] == 0xE5) { lfnCount = 0; continue; }
        if ((d[11] & 0x3F) == 0x0F) {
            int seq = d[0] & 0x1F;
            if (d[0] & 0x40) {
                lfnCount = seq;
                lfnSeen = 0;
                lfnSum = d[13];
                lfnFirst = slot;
                lfn.assign((size_t)seq * 13, 0xFFFF);
            }
            if (lfnCount && seq >= 1 && seq <= lfnCount && d[13] == lfnSum) {
                static const int offs[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
                for (int k = 0; k < 13; k++) lfn[(seq - 1) * 13 + k] = get16(d + offs[k]);
                lfnSeen++;
            } else {
                lfnCount = 0;
            }
            continue;
        }
        if (d[11] & 0x08) { lfnCount = 0; continue; }  // volume label
        Entry e;
        std::string base((const char*)d, 8), ext((const char*)d + 8, 3);
        if ((uint8_t)base[0] == 0x05) base[0] = (char)0xE5;
        while (!base.empty() && base.back() == ' ') base.pop_back();
        while (!ext.empty() && ext.back() == ' ') ext.pop_back();
        e.shortName = ext.empty() ? base : base + "." + ext;
        std::string b2 = base, e2 = ext;
        if (d[12] & 0x08) for (auto& c : b2) c = (char)std::tolower((unsigned char)c);
        if (d[12] & 0x10) for (auto& c : e2) c = (char)std::tolower((unsigned char)c);
        e.name = e2.empty() ? b2 : b2 + "." + e2;
        if (lfnCount && lfnSeen == lfnCount && lfnSum == lfnChecksum(d)) {
            std::vector<uint16_t> u;
            for (uint16_t c : lfn) {
                if (c == 0 || c == 0xFFFF) break;
                u.push_back(c);
            }
            e.name = fromUcs2(u);
            e.firstSlot = lfnFirst;
        } else {
            e.firstSlot = slot;
        }
        lfnCount = 0;
        e.slot = slot;
        e.attr = d[11];
        e.cluster = ((uint32_t)get16(d + 20) << 16) | get16(d + 26);
        e.size = get32(d + 28);
        e.time = get16(d + 22);
        e.date = get16(d + 24);
        out.push_back(e);
    }
    return out;
}

bool SdCardImage::findIn(uint32_t dirCluster, const std::string& name, Entry& e, std::vector<uint8_t>* raw) {
    std::vector<uint8_t> local;
    std::vector<uint8_t>& r = raw ? *raw : local;
    if (!readDir(dirCluster, r)) return false;
    for (const auto& x : parseDir(r))
        if (iequal(x.name, name) || iequal(x.shortName, name)) { e = x; return true; }
    return false;
}

bool SdCardImage::resolveDir(const std::string& path, uint32_t& cluster, std::string& err) {
    if (!fatOk_) { err = "the image has no FAT32 partition"; return false; }
    cluster = 0;
    size_t i = 0;
    while (i <= path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos) j = path.size();
        std::string part = path.substr(i, j - i);
        i = j + 1;
        if (part.empty() || part == ".") continue;
        Entry e;
        if (!findIn(cluster, part, e, nullptr)) { err = "no directory " + path; return false; }
        if (!e.isDir()) { err = part + " is not a directory"; return false; }
        cluster = e.cluster == rootCluster_ ? 0 : e.cluster;
    }
    return true;
}

std::string SdCardImage::makeShortName(const std::vector<Entry>& existing, const std::string& name,
                                       bool& needLfn, uint8_t& ntCase) const {
    ntCase = 0;
    needLfn = false;
    size_t dot = name.rfind('.');
    std::string base = dot == std::string::npos || dot == 0 ? name : name.substr(0, dot);
    std::string ext = dot == std::string::npos || dot == 0 ? "" : name.substr(dot + 1);
    auto caseOf = [](const std::string& s) {  // 0 none, 1 upper, 2 lower, 3 mixed
        int c = 0;
        for (char ch : s) {
            if (std::isupper((unsigned char)ch)) c |= 1;
            if (std::islower((unsigned char)ch)) c |= 2;
        }
        return c;
    };
    bool valid = !base.empty() && base.size() <= 8 && ext.size() <= 3 &&
                 std::count(name.begin(), name.end(), '.') <= 1 && name[0] != '.' &&
                 std::all_of(base.begin(), base.end(), shortChar) && std::all_of(ext.begin(), ext.end(), shortChar) &&
                 caseOf(base) != 3 && caseOf(ext) != 3;
    std::set<std::string> taken;
    for (const auto& e : existing) taken.insert(upper(e.shortName));
    if (valid) {
        if (caseOf(base) == 2) ntCase |= 0x08;
        if (caseOf(ext) == 2) ntCase |= 0x10;
        return upper(name);
    }
    needLfn = true;
    auto clean = [](const std::string& s) {
        std::string o;
        for (char c : s) {
            if (c == ' ' || c == '.') continue;
            o += shortChar(c) ? (char)std::toupper((unsigned char)c) : '_';
        }
        return o;
    };
    std::string b = clean(base), x = clean(ext).substr(0, 3);
    if (b.empty()) b = "_";
    for (int n = 1; n < 1000000; n++) {
        std::string tail = "~" + std::to_string(n);
        std::string s = b.substr(0, 8 - tail.size()) + tail;
        if (!x.empty()) s += "." + x;
        if (!taken.count(s)) return s;
    }
    return "";
}

bool SdCardImage::addEntry(uint32_t dirCluster, const std::string& name, uint8_t attr, uint32_t cluster,
                           uint32_t size, std::string& err) {
    std::vector<uint8_t> raw;
    if (!readDir(dirCluster, raw)) { err = "cannot read the directory"; return false; }
    auto existing = parseDir(raw);
    bool needLfn;
    uint8_t ntCase;
    std::string sname = makeShortName(existing, name, needLfn, ntCase);
    if (sname.empty()) { err = "no free short name for " + name; return false; }
    std::vector<uint16_t> u = toUcs2(name);
    if (u.size() > 255) { err = "name longer than 255 characters: " + name; return false; }
    uint32_t lfnSlots = needLfn ? (uint32_t)((u.size() + 12) / 13) : 0;
    uint32_t need = lfnSlots + 1;

    // a run of free slots (deleted entries, or from the end marker on)
    uint32_t slots = (uint32_t)(raw.size() / 32), run = 0, at = UINT32_MAX;
    bool atEnd = false;
    for (uint32_t s = 0; s < slots; s++) {
        uint8_t first = raw[s * 32];
        if (first == 0x00) atEnd = true;
        if (atEnd || first == 0xE5) {
            if (++run == need) { at = s + 1 - need; break; }
        } else {
            run = 0;
        }
    }
    if (at == UINT32_MAX) {
        // grow the directory by clusters (zeroed)
        uint32_t dirStart = dirCluster ? dirCluster : rootCluster_;
        uint32_t cb = spc_ * 512 / 32;
        uint32_t more = (need - run + cb - 1) / cb;
        auto ch = chain(dirStart);
        uint32_t last = ch.back();
        for (uint32_t k = 0; k < more; k++) {
            uint32_t c;
            if (!allocRun(1, c)) { err = "the card is full"; return false; }
            setFat(last, c);
            last = c;
        }
        at = slots - run;
        raw.resize(raw.size() + (size_t)more * spc_ * 512, 0);
    }

    uint8_t sn[11];
    packShort(sname, sn);
    uint8_t sum = lfnChecksum(sn);
    for (uint32_t i = 0; i < lfnSlots; i++) {
        uint32_t seq = lfnSlots - i;  // highest first
        uint8_t* d = raw.data() + (size_t)(at + i) * 32;
        std::memset(d, 0, 32);
        d[0] = (uint8_t)(seq | (i == 0 ? 0x40 : 0));
        d[11] = 0x0F;
        d[13] = sum;
        static const int offs[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
        for (int k = 0; k < 13; k++) {
            size_t idx = (size_t)(seq - 1) * 13 + k;
            uint16_t c = idx < u.size() ? u[idx] : idx == u.size() ? 0x0000 : 0xFFFF;
            put16(d + offs[k], c);
        }
    }
    uint8_t* d = raw.data() + (size_t)(at + lfnSlots) * 32;
    std::memset(d, 0, 32);
    std::memcpy(d, sn, 11);
    d[11] = attr;
    d[12] = ntCase;
    uint16_t date, time;
    fatNow(date, time);
    put16(d + 14, time);
    put16(d + 16, date);
    put16(d + 18, date);
    put16(d + 20, (uint16_t)(cluster >> 16));
    put16(d + 22, time);
    put16(d + 24, date);
    put16(d + 26, (uint16_t)cluster);
    put32(d + 28, size);
    if (!writeChain(dirCluster ? dirCluster : rootCluster_, raw)) { err = "cannot write the directory"; return false; }
    return true;
}

bool SdCardImage::removeEntry(uint32_t dirCluster, const Entry& e, std::string& err) {
    std::vector<uint8_t> raw;
    if (!readDir(dirCluster, raw)) { err = "cannot read the directory"; return false; }
    for (uint32_t s = e.firstSlot; s <= e.slot; s++) raw[(size_t)s * 32] = 0xE5;
    if (!writeChain(dirCluster ? dirCluster : rootCluster_, raw)) { err = "cannot write the directory"; return false; }
    return true;
}

// ---------------------------------------------------------------------------
// Public file operations
// ---------------------------------------------------------------------------

std::string SdCardImage::volumeLabel() const {
    if (!fatOk_) return "";
    auto self = const_cast<SdCardImage*>(this);
    std::vector<uint8_t> raw;
    if (self->readDir(0, raw))
        for (size_t s = 0; s * 32 < raw.size() && raw[s * 32]; s++) {
            const uint8_t* d = raw.data() + s * 32;
            if (d[0] != 0xE5 && (d[11] & 0x3F) != 0x0F && (d[11] & 0x08)) {
                std::string l((const char*)d, 11);
                while (!l.empty() && l.back() == ' ') l.pop_back();
                return l;
            }
        }
    uint8_t b[512];
    if (!self->readSector(volStart_, b)) return "";
    std::string l((const char*)b + 0x47, 11);
    while (!l.empty() && l.back() == ' ') l.pop_back();
    return l;
}

bool SdCardImage::list(const std::string& dir, std::vector<Entry>& out, std::string& err) {
    uint32_t c;
    if (!resolveDir(dir, c, err)) return false;
    std::vector<uint8_t> raw;
    if (!readDir(c, raw)) { err = "cannot read the directory"; return false; }
    out.clear();
    for (auto& e : parseDir(raw))
        if (e.shortName != "." && e.shortName != "..") out.push_back(e);
    return true;
}

bool SdCardImage::stat(const std::string& path, Entry& out, std::string& err) {
    std::string parent, leaf;
    splitPath(path, parent, leaf);
    uint32_t c;
    if (!resolveDir(parent, c, err)) return false;
    if (!findIn(c, leaf, out, nullptr)) { err = "no file " + path; return false; }
    return true;
}

bool SdCardImage::readFile(const std::string& path, std::vector<uint8_t>& data, std::string& err) {
    Entry e;
    if (!stat(path, e, err)) return false;
    if (e.isDir()) { err = path + " is a directory"; return false; }
    if (!readChain(e.cluster, data)) { err = "cannot read " + path; return false; }
    if (data.size() < e.size) { err = path + ": the cluster chain is shorter than the file"; return false; }
    data.resize(e.size);
    return true;
}

bool SdCardImage::writeFile(const std::string& path, const std::vector<uint8_t>& data, bool overwrite,
                            std::string& err) {
    if (!writable_) { err = "the image is open read-only"; return false; }
    std::string parent, leaf;
    splitPath(path, parent, leaf);
    if (leaf.empty()) { err = "no file name"; return false; }
    uint32_t dir;
    if (!resolveDir(parent, dir, err)) return false;
    if (data.size() > 0xFFFFFFFFull) { err = "files on FAT32 are at most 4 GB"; return false; }
    uint32_t cb = spc_ * 512;
    uint32_t n = (uint32_t)((data.size() + cb - 1) / cb);

    Entry old;
    std::vector<uint8_t> raw;
    bool exists = findIn(dir, leaf, old, &raw);
    if (exists && old.isDir()) { err = path + " is a directory"; return false; }
    if (exists && !overwrite) { err = path + " already exists"; return false; }

    // a contiguous run of clusters; a replaced file's clusters can be reused
    std::vector<uint32_t> oldChain = exists ? chain(old.cluster) : std::vector<uint32_t>{};
    for (uint32_t c : oldChain) setFat(c, 0);
    uint32_t first;
    if (!allocRun(n, first)) {
        for (size_t k = 0; k < oldChain.size(); k++)
            setFat(oldChain[k], k + 1 < oldChain.size() ? oldChain[k + 1] : EOC);
        err = "no contiguous space for " + std::to_string(data.size()) + " bytes";
        return false;
    }
    if (n && !writeChain(first, data)) { err = "cannot write " + path; return false; }
    if (exists) {
        uint8_t* d = raw.data() + (size_t)old.slot * 32;
        uint16_t date, time;
        fatNow(date, time);
        put16(d + 18, date);
        put16(d + 20, (uint16_t)(first >> 16));
        put16(d + 22, time);
        put16(d + 24, date);
        put16(d + 26, (uint16_t)first);
        put32(d + 28, (uint32_t)data.size());
        d[11] |= 0x20;
        if (!writeChain(dir ? dir : rootCluster_, raw)) { err = "cannot write the directory"; return false; }
        return true;
    }
    if (!addEntry(dir, leaf, 0x20, first, (uint32_t)data.size(), err)) {
        freeChain(first);
        return false;
    }
    return true;
}

bool SdCardImage::remove(const std::string& path, std::string& err) {
    if (!writable_) { err = "the image is open read-only"; return false; }
    std::string parent, leaf;
    splitPath(path, parent, leaf);
    uint32_t dir;
    if (!resolveDir(parent, dir, err)) return false;
    Entry e;
    if (!findIn(dir, leaf, e, nullptr)) { err = "no file " + path; return false; }
    if (e.isDir()) {
        std::vector<uint8_t> raw;
        readDir(e.cluster, raw);
        for (const auto& x : parseDir(raw))
            if (x.shortName != "." && x.shortName != "..") { err = path + " is not empty"; return false; }
    }
    if (!removeEntry(dir, e, err)) return false;
    freeChain(e.cluster);
    return true;
}

bool SdCardImage::mkdir(const std::string& path, std::string& err) {
    if (!writable_) { err = "the image is open read-only"; return false; }
    std::string parent, leaf;
    splitPath(path, parent, leaf);
    uint32_t dir;
    if (!resolveDir(parent, dir, err)) return false;
    Entry e;
    if (leaf.empty() || findIn(dir, leaf, e, nullptr)) { err = path + " already exists"; return false; }
    uint32_t c;
    if (!allocRun(1, c)) { err = "the card is full"; return false; }
    std::vector<uint8_t> raw((size_t)spc_ * 512, 0);
    uint16_t date, time;
    fatNow(date, time);
    auto dot = [&](int slot, const char* n, uint32_t cl) {
        uint8_t* d = raw.data() + slot * 32;
        std::memset(d, ' ', 11);
        std::memcpy(d, n, std::strlen(n));
        d[11] = 0x10;
        put16(d + 14, time); put16(d + 16, date); put16(d + 18, date);
        put16(d + 20, (uint16_t)(cl >> 16));
        put16(d + 22, time); put16(d + 24, date);
        put16(d + 26, (uint16_t)cl);
    };
    dot(0, ".", c);
    dot(1, "..", dir);
    if (!writeChain(c, raw) || !addEntry(dir, leaf, 0x10, c, 0, err)) {
        if (err.empty()) err = "cannot write " + path;
        freeChain(c);
        return false;
    }
    return true;
}

bool SdCardImage::rename(const std::string& from, const std::string& to, std::string& err) {
    if (!writable_) { err = "the image is open read-only"; return false; }
    std::string p1, l1, p2, l2;
    splitPath(from, p1, l1);
    splitPath(to, p2, l2);
    uint32_t d1, d2;
    if (!resolveDir(p1, d1, err) || !resolveDir(p2, d2, err)) return false;
    Entry e, x;
    if (!findIn(d1, l1, e, nullptr)) { err = "no file " + from; return false; }
    if (findIn(d2, l2, x, nullptr) && !(d1 == d2 && x.slot == e.slot)) { err = to + " already exists"; return false; }
    if (!removeEntry(d1, e, err)) return false;
    if (!addEntry(d2, l2, e.attr, e.cluster, e.size, err)) return false;
    if (e.isDir() && d1 != d2) {  // the moved directory's ".."
        std::vector<uint8_t> raw;
        readDir(e.cluster, raw);
        for (const auto& y : parseDir(raw))
            if (y.shortName == "..") {
                uint8_t* d = raw.data() + (size_t)y.slot * 32;
                put16(d + 20, (uint16_t)(d2 >> 16));
                put16(d + 26, (uint16_t)d2);
                writeChain(e.cluster, raw);
            }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Consistency check
// ---------------------------------------------------------------------------

SdCardImage::CheckResult SdCardImage::check() {
    CheckResult r;
    if (!fatOk_) { r.errors.push_back("no FAT32 partition"); return r; }
    std::vector<uint8_t> owner(clusters_ + 2, 0);
    auto claim = [&](uint32_t start, const std::string& what, uint64_t size, bool isDir) {
        uint32_t c = start, n = 0;
        while (true) {
            if (c < 2 || c >= clusters_ + 2) {
                r.errors.push_back(what + ": cluster " + std::to_string(c) + " out of range");
                break;
            }
            if (owner[c]) {
                r.errors.push_back(what + ": cluster " + std::to_string(c) + " is cross-linked");
                break;
            }
            owner[c] = 1;
            n++;
            uint32_t next = fatEntry(c);
            if (next == 0) { r.errors.push_back(what + ": chain runs into a free cluster"); break; }
            if (next == BAD) { r.errors.push_back(what + ": chain runs into a bad cluster"); break; }
            if (next >= 0x0FFFFFF8) break;
            c = next;
        }
        if (!isDir) {
            uint64_t cb = (uint64_t)spc_ * 512;
            uint64_t want = (size + cb - 1) / cb;
            if (n != want)
                r.errors.push_back(what + ": " + std::to_string(n) + " clusters for " + std::to_string(size) +
                                   " bytes (expected " + std::to_string(want) + ")");
        }
        return n;
    };
    claim(rootCluster_, "root directory", 0, true);
    std::function<void(uint32_t, const std::string&, int)> walk = [&](uint32_t dir, const std::string& prefix,
                                                                      int depth) {
        std::vector<uint8_t> raw;
        if (!readDir(dir, raw)) { r.errors.push_back(prefix + ": cannot read the directory"); return; }
        for (const auto& e : parseDir(raw)) {
            if (e.shortName == "." || e.shortName == "..") continue;
            std::string p = prefix + "/" + e.name;
            if (e.name.size() > 64) r.warnings.push_back(p + ": the MEGA65 sees at most 64 characters of a name");
            if (e.isDir()) {
                r.dirs++;
                if (e.cluster == 0) { r.errors.push_back(p + ": directory without a cluster"); continue; }
                claim(e.cluster, p, 0, true);
                if (depth < 32) walk(e.cluster, p, depth + 1);
                continue;
            }
            r.files++;
            if (e.cluster == 0) {
                if (e.size) r.errors.push_back(p + ": " + std::to_string(e.size) + " bytes but no cluster");
                continue;
            }
            claim(e.cluster, p, e.size, false);
            if (runs(e.cluster).size() > 1) {
                std::string ext = upper(e.name.size() > 4 ? e.name.substr(e.name.size() - 4) : "");
                bool image = ext == ".D81" || ext == ".D64" || ext == ".D71" || ext == ".D65";
                r.warnings.push_back(p + ": fragmented (" + std::to_string(runs(e.cluster).size()) + " pieces)" +
                                     (image ? " - the MEGA65 cannot mount it" : ""));
            }
        }
    };
    walk(0, "", 0);
    uint32_t lost = 0;
    for (uint32_t c = 2; c < clusters_ + 2; c++) {
        uint32_t v = fatEntry(c);
        if (v == 0) r.freeClusters++;
        else if (owner[c]) r.usedClusters++;
        else if (v != BAD) lost++;
    }
    if (lost) r.errors.push_back(std::to_string(lost) + " lost clusters (allocated, in no file)");
    // the FAT copies agree
    uint8_t a[512], b[512];
    uint32_t needSects = (uint32_t)(((uint64_t)(clusters_ + 2) * 4 + 511) / 512);
    for (int k = 1; k < nFats_; k++)
        for (uint32_t s = 0; s < needSects; s++) {
            readSector(volStart_ + reserved_ + s, a);
            readSector(volStart_ + reserved_ + (uint64_t)k * fatSize_ + s, b);
            if (std::memcmp(a, b, 512)) {
                r.errors.push_back("FAT copy " + std::to_string(k + 1) + " differs from FAT 1 (sector " +
                                   std::to_string(s) + ")");
                break;
            }
        }
    if (fsInfo_ && fsInfo_ != 0xFFFF && readSector(volStart_ + fsInfo_, a) && get32(a) == 0x41615252) {
        uint32_t fc = get32(a + 0x1e8);
        if (fc != 0xFFFFFFFF && fc != r.freeClusters)
            r.warnings.push_back("FS information sector: " + std::to_string(fc) + " free clusters recorded, " +
                                 std::to_string(r.freeClusters) + " free");
    }
    return r;
}

// ---------------------------------------------------------------------------
// System partition
// ---------------------------------------------------------------------------

bool SdCardImage::readSysPart(SysPart& sp, std::string& err) {
    const Partition* p = nullptr;
    for (const auto& x : parts_)
        if (x.type == 0x41) { p = &x; break; }
    if (!p) { err = "the card has no MEGA65 system partition"; return false; }
    uint8_t b[512];
    if (!readSector(p->start, b)) { err = "cannot read the system partition"; return false; }
    sp.valid = std::memcmp(b, "MEGA65SYS00", 11) == 0;
    sp.freezeStart = get32(b + 0x10);
    sp.freezeSize = get32(b + 0x14);
    sp.freezeSlotSize = get32(b + 0x18);
    sp.freezeSlots = get16(b + 0x1c);
    sp.freezeDirSectors = get16(b + 0x1e);
    sp.serviceStart = get32(b + 0x20);
    sp.serviceSize = get32(b + 0x24);
    sp.serviceSlotSize = get32(b + 0x28);
    sp.serviceSlots = get16(b + 0x2c);
    sp.serviceDirSectors = get16(b + 0x2e);
    if (!readSector(p->start + 1, sp.config)) { err = "cannot read the configuration sector"; return false; }
    return true;
}

bool SdCardImage::readConfig(uint8_t config[512], std::string& err) {
    if (parts_.empty()) { err = "the card has no partition table (no sector 1 for the configuration)"; return false; }
    if (!readSector(1, config)) { err = "cannot read the configuration sector"; return false; }
    return true;
}

bool SdCardImage::writeConfig(const uint8_t config[512], std::string& err) {
    if (!writable_) { err = "the image is open read-only"; return false; }
    if (parts_.empty()) { err = "the card has no partition table (no sector 1 for the configuration)"; return false; }
    for (const auto& x : parts_)
        if (x.start <= 1) { err = "a partition starts at sector 1, where the configuration sector belongs"; return false; }
    if (!writeSector(1, config)) { err = "cannot write the configuration sector"; return false; }
    SysPart sp;
    std::string ignore;
    for (const auto& x : parts_)
        if (x.type == 0x41 && readSysPart(sp, ignore) && sp.valid && !writeSector(x.start + 1, config)) {
            err = "cannot write the system partition's configuration sector";
            return false;
        }
    return true;
}
