// disk45_sd.cpp — disk45 sd: MEGA65 SD card images (FAT32 + system partition)

#include "SdCardImage.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <filesystem>
#include <random>
#include <vector>

namespace {

void sdUsage() {
    std::cerr
        << "Usage: disk45 sd <command> <card.img> ...\n"
        << "  create <img> <size> [options] [config options]\n"
        << "      new card image laid out like the MEGA65's FDISK; size: bytes or K/M/G\n"
        << "      --label NAME          volume label (default M.E.G.A.65!)\n"
        << "      --no-syspart          no MEGA65 system partition (FAT32 only)\n"
        << "      --syspart SIZE        system partition size (default: half, at most 2G)\n"
        << "      --onboarded           configured as after the first-boot onboarding\n"
        << "                            (default: onboarding runs on the first boot, as FDISK)\n"
        << "      -f, --force           replace an existing file\n"
        << "  info <img>                       partitions, FAT32 volume, system partition\n"
        << "  list <img> [dir]                 directory listing\n"
        << "  add <img> <file> [card-path] [-f]  copy a file onto the card (contiguous)\n"
        << "  extract <img> <card-path> [file] copy a file off the card\n"
        << "  rm <img> <card-path>             delete a file or an empty directory\n"
        << "  mkdir <img> <dir>                create a directory\n"
        << "  mv <img> <from> <to>             rename or move\n"
        << "  check <img>                      check the FAT32 volume\n"
        << "  config <img> [options]           show or change the configuration sector (card\n"
        << "                                   sector 1, copied to the system partition):\n"
        << "      --pal | --ntsc               video standard (FDISK default NTSC)\n"
        << "      --hdmi-audio on|off          digital audio, or pure DVI\n"
        << "      --default-disk NAME          disk image mounted at boot (mega65.d81)\n"
        << "      --mac XX:XX:XX:XX:XX:XX | random   Ethernet MAC address\n"
        << "      --onboarding done|pending    whether the first-boot onboarding has run\n"
        << "      --lfn on|off                 long file names in the hypervisor\n"
        << "      --floppy sd|drive            F011 uses SD card disk images or the drive\n"
        << "      --dmagic old|new             DMAgic F018A or F018B\n"
        << "      --sid 6581|8580              SID type\n"
        << "      --crt-emulation on|off, --blur on|off   scanlines, horizontal blur\n"
        << "      --super-sfx on|off           Super SFX cartridge emulation\n"
        << "      --amiga-mouse on|off, --audio-amp on|off, --mono on|off,\n"
        << "      --swap-stereo on|off, --dac pdm|pwm\n";
}

bool parseMac(const std::string& s, uint8_t mac[6]) {
    unsigned v[6];
    if (std::sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return false;
    for (int i = 0; i < 6; i++) {
        if (v[i] > 255) return false;
        mac[i] = (uint8_t)v[i];
    }
    return true;
}

std::string macString(const uint8_t* m) {
    char b[32];
    std::snprintf(b, sizeof b, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    return b;
}

std::string sizeString(uint64_t bytes) {
    std::ostringstream o;
    if (bytes >= (1ull << 30) && bytes % (1ull << 30) == 0) o << (bytes >> 30) << " GB";
    else if (bytes >= (1ull << 20)) o << std::fixed << std::setprecision(bytes % (1ull << 20) ? 1 : 0)
                                      << bytes / 1048576.0 << " MB";
    else o << bytes / 1024 << " KB";
    return o.str();
}

std::string fatDate(uint16_t d, uint16_t t) {
    char b[32];
    std::snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d", 1980 + (d >> 9), (d >> 5) & 15, d & 31, t >> 11,
                  (t >> 5) & 63);
    return b;
}

bool openCard(SdCardImage& img, const std::string& path, bool writable) {
    std::string err;
    if (!img.open(path, writable, err)) {
        std::cerr << "Error: " << err << "\n";
        return false;
    }
    return true;
}

int finish(SdCardImage& img) {
    std::string err;
    if (!img.flush(err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    return 0;
}

bool onOff(const std::string& v, bool& out) {
    if (v == "on" || v == "1" || v == "yes") { out = true; return true; }
    if (v == "off" || v == "0" || v == "no") { out = false; return true; }
    return false;
}

void printConfig(const uint8_t* c) {
    std::string disk((const char*)c + 0x10, 16);
    disk = disk.substr(0, disk.find('\0'));
    auto on = [](bool b) { return b ? "on" : "off"; };
    std::cout << "  Structure version: " << (int)c[0] << "." << (int)c[1]
              << (c[0] == 1 && c[1] == 1 ? "" : " (the hypervisor needs 1.1)") << "\n"
              << "  Onboarding:        " << ((c[0x0e] & 0x80) ? "done" : "pending (runs on the next boot)") << "\n"
              << "  Video:             " << ((c[2] & 0x80) ? "NTSC" : "PAL") << ", "
              << ((c[0x0d] & 0x02) ? "pure DVI" : "HDMI with digital audio") << "\n"
              << "  Video filters:     CRT emulation " << on(c[0x21] & 0x20) << ", blur " << on(c[0x21] & 0x08)
              << "\n"
              << "  Audio:             amplifier " << on(c[3] & 0x01) << ", "
              << ((c[3] & 0x40) ? "mono to both channels" : (c[3] & 0x20) ? "stereo, channels swapped" : "stereo")
              << ", DAC " << ((c[3] & 0x80) ? "PWM" : "PDM") << ", SID " << (c[0x22] ? "8580" : "6581") << "\n"
              << "  Floppy:            " << ((c[4] & 1) ? "internal 3.5\" drive" : "SD card disk images") << "\n"
              << "  Default disk:      " << (disk.empty() ? "(none)" : disk) << "\n"
              << "  Long file names:   " << on(c[0x0f] & 0x80) << "\n"
              << "  Amiga mouse:       " << ((c[5] & 1) ? "as 1351" : "off") << "\n"
              << "  Super SFX:         " << on(c[0x0c]) << "\n"
              << "  Ethernet MAC:      " << macString(c + 6)
              << (((c[6] & 0x03) != 0x02) ? " (not locally administered unicast: onboarding replaces it)" : "")
              << "\n"
              << "  DMAgic:            " << (c[0x20] ? "new (F018B)" : "old (F018A)") << "\n";
}

// The configuration FDISK writes (onboarding pending)
void fdiskConfig(uint8_t* c) {
    std::memset(c, 0, 512);
    c[0x00] = 0x01; c[0x01] = 0x01;
    c[0x02] = 0x80;
    c[0x03] = 0x41;
    c[0x05] = 0x01;
    std::memset(c + 0x06, 0x41, 6);
    std::memcpy(c + 0x10, "mega65.d81", 10);
    c[0x20] = 0x01;
}

void randomMac(uint8_t* m) {
    std::random_device rd;
    for (int i = 0; i < 6; i++) m[i] = (uint8_t)rd();
    m[0] = (uint8_t)((m[0] & 0xFC) | 0x02);  // locally administered, unicast
}

// The configuration options (config, and create after the card is made);
// unknown options are left in rest
bool applyConfig(uint8_t* c, const std::vector<std::string>& args, std::vector<std::string>* rest) {
    for (size_t i = 0; i < args.size(); i++) {
        const std::string& a = args[i];
        bool valueless = a == "--pal" || a == "--ntsc" || a == "--onboarded";
        bool known = valueless || a == "--default-disk" || a == "--mac" || a == "--floppy" || a == "--dmagic" ||
                     a == "--dac" || a == "--sid" || a == "--onboarding" || a == "--amiga-mouse" ||
                     a == "--audio-amp" || a == "--swap-stereo" || a == "--mono" || a == "--lfn" ||
                     a == "--crt-emulation" || a == "--blur" || a == "--super-sfx" || a == "--hdmi-audio";
        if (!known) {
            if (!rest) { std::cerr << "Error: unknown option " << a << "\n"; return false; }
            rest->push_back(a);
            continue;
        }
        if (!valueless && i + 1 >= args.size()) {
            std::cerr << "Error: " << a << " needs a value\n";
            return false;
        }
        std::string v = valueless ? "" : args[++i];
        bool b = false;
        auto flag = [&](int byte, uint8_t mask) {
            if (!onOff(v, b)) { std::cerr << "Error: " << a << " takes on or off\n"; return false; }
            c[byte] = (uint8_t)(b ? c[byte] | mask : c[byte] & ~mask);
            return true;
        };
        auto choice = [&](const char* x, const char* y) {
            if (v != x && v != y) { std::cerr << "Error: " << a << " takes " << x << " or " << y << "\n"; return -1; }
            return v == y ? 1 : 0;
        };
        int k = 0;
        if (a == "--pal") c[2] = (uint8_t)(c[2] & 0x3F);
        else if (a == "--ntsc") c[2] = (uint8_t)((c[2] & 0x3F) | 0x80);
        else if (a == "--onboarded") {
            // what the onboarding utility sets
            c[0x0e] = 0x80;
            c[0x0f] = 0x80;
            c[0x20] = 0x01;
            c[0x22] = 0x01;
            if ((c[6] & 0x03) != 0x02) randomMac(c + 6);
        } else if (a == "--default-disk") {
            if (v.size() > 16) { std::cerr << "Error: the default disk name is at most 16 characters\n"; return false; }
            std::memset(c + 0x10, 0, 16);
            std::memcpy(c + 0x10, v.data(), v.size());
        } else if (a == "--mac") {
            if (v == "random") randomMac(c + 6);
            else if (!parseMac(v, c + 6)) { std::cerr << "Error: bad MAC address " << v << "\n"; return false; }
        } else if (a == "--floppy") {
            if ((k = choice("sd", "drive")) < 0) return false;
            c[4] = (uint8_t)((c[4] & ~1) | k);
        } else if (a == "--dmagic") {
            if ((k = choice("old", "new")) < 0) return false;
            c[0x20] = (uint8_t)k;
        } else if (a == "--dac") {
            if ((k = choice("pdm", "pwm")) < 0) return false;
            c[3] = (uint8_t)((c[3] & 0x7F) | (k ? 0x80 : 0));
        } else if (a == "--sid") {
            if ((k = choice("6581", "8580")) < 0) return false;
            c[0x22] = (uint8_t)k;
        } else if (a == "--onboarding") {
            if ((k = choice("pending", "done")) < 0) return false;
            c[0x0e] = (uint8_t)((c[0x0e] & 0x7F) | (k ? 0x80 : 0));
        } else if (a == "--hdmi-audio") {
            if (!onOff(v, b)) { std::cerr << "Error: " << a << " takes on or off\n"; return false; }
            c[0x0d] = (uint8_t)(b ? c[0x0d] & ~0x02 : c[0x0d] | 0x02);
        } else if (a == "--super-sfx") {
            if (!onOff(v, b)) { std::cerr << "Error: " << a << " takes on or off\n"; return false; }
            c[0x0c] = b ? 1 : 0;
        } else if (a == "--amiga-mouse") { if (!flag(5, 0x01)) return false; }
        else if (a == "--audio-amp") { if (!flag(3, 0x01)) return false; }
        else if (a == "--swap-stereo") { if (!flag(3, 0x20)) return false; }
        else if (a == "--mono") { if (!flag(3, 0x40)) return false; }
        else if (a == "--lfn") { if (!flag(0x0f, 0x80)) return false; }
        else if (a == "--crt-emulation") { if (!flag(0x21, 0x20)) return false; }
        else if (a == "--blur") { if (!flag(0x21, 0x08)) return false; }
    }
    return true;
}

int cmdCreate(int argc, char** argv) {
    if (argc < 2) { sdUsage(); return 1; }
    std::string path = argv[0];
    SdCardImage::CreateOptions opt;
    if (!SdCardImage::parseSize(argv[1], opt.bytes)) {
        std::cerr << "Error: bad size " << argv[1] << "\n";
        return 1;
    }
    bool force = false;
    std::vector<std::string> config;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::cerr << "Error: " << a << " needs a value\n"; std::exit(1); }
            return argv[++i];
        };
        if (a == "--label") opt.label = next();
        else if (a == "--no-syspart") opt.sysPart = false;
        else if (a == "--syspart") {
            uint64_t b;
            std::string v = next();
            if (!SdCardImage::parseSize(v, b) || b / 512 > 0xFFFFFFFFull) {
                std::cerr << "Error: bad size " << v << "\n";
                return 1;
            }
            opt.sysPartSectors = (uint32_t)(b / 512);
        } else if (a == "-f" || a == "--force") force = true;
        else config.push_back(a);
    }
    // the configuration options are checked before anything is written
    uint8_t probe[512] = {};
    std::vector<std::string> rest;
    if (!applyConfig(probe, config, &rest)) return 1;
    if (!rest.empty()) { std::cerr << "Error: unknown option " << rest[0] << "\n"; return 1; }
    for (char ch : opt.label == SdCardImage::CreateOptions().label ? std::string() : opt.label)
        if (std::strchr("\"*+,./:;<=>?[\\]|", ch) || (unsigned char)ch < 0x20) {
            std::cerr << "Warning: '" << ch << "' is not valid in a FAT volume label (fsck may remove the label)\n";
            break;
        }
    if (std::filesystem::exists(path) && !force) {
        std::cerr << "Error: " << path << " exists (use -f to replace it)\n";
        return 1;
    }
    std::string err;
    if (!SdCardImage::create(path, opt, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    SdCardImage img;
    if (!openCard(img, path, !config.empty())) return 1;
    if (!config.empty()) {
        uint8_t c[512];
        if (!img.readConfig(c, err) || !applyConfig(c, config, nullptr) || !img.writeConfig(c, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
    }
    std::cout << "Created " << path << " (" << sizeString(opt.bytes) << ")\n";
    for (const auto& p : img.partitions())
        std::cout << "  partition " << p.index + 1 << ": " << SdCardImage::partitionTypeName(p.type) << ", "
                  << sizeString((uint64_t)p.sectors * 512) << " from sector " << p.start << "\n";
    if (img.clusterCount() < 65525)
        std::cout << "  note: " << img.clusterCount()
                  << " clusters; FAT32 tools expect at least 65525 (cards of 512 MB and more)\n";
    return finish(img);
}

int cmdInfo(int argc, char** argv) {
    if (argc < 1) { sdUsage(); return 1; }
    SdCardImage img;
    if (!openCard(img, argv[0], false)) return 1;
    std::cout << "Image:  " << argv[0] << "\n";
    if (img.partitions().empty()) std::cout << "Partitions: none (FAT32 file system from sector 0)\n";
    else {
        std::cout << "Partitions:\n";
        for (const auto& p : img.partitions()) {
            char t[8];
            std::snprintf(t, sizeof t, "$%02X", p.type);
            std::cout << "  " << p.index + 1 << "  type " << t << " " << std::left << std::setw(14)
                      << SdCardImage::partitionTypeName(p.type) << std::right << " start " << std::setw(9)
                      << p.start << "  sectors " << std::setw(9) << p.sectors << "  ("
                      << sizeString((uint64_t)p.sectors * 512) << ")\n";
        }
    }
    if (img.hasFat()) {
        uint32_t freeC = img.freeClusters();
        std::cout << "FAT32 volume:\n"
                  << "  Label:            " << img.volumeLabel() << "\n"
                  << "  Cluster size:     " << img.clusterBytes() << " bytes\n"
                  << "  Clusters:         " << img.clusterCount() << "\n"
                  << "  Free:             " << freeC << " clusters ("
                  << sizeString((uint64_t)freeC * img.clusterBytes()) << ")\n"
                  << "  Reserved sectors: " << img.reservedSectors() << "\n"
                  << "  FATs:             " << (int)img.fatCount() << " x " << img.fatSectors() << " sectors\n";
        if (img.clusterBytes() > 65536) std::cout << "  warning: the MEGA65 handles clusters up to 64 KB\n";
    } else {
        std::cout << "FAT32 volume: none (the MEGA65 needs a partition of type $0B or $0C)\n";
    }
    SdCardImage::SysPart sp;
    std::string err;
    if (img.readSysPart(sp, err)) {
        std::cout << "MEGA65 system partition:\n";
        if (!sp.valid) std::cout << "  not set up (no MEGA65SYS00 header)\n";
        else {
            std::cout << "  Freeze slots:      " << sp.freezeSlots << " x " << sp.freezeSlotSize / 2
                      << " KB, directory " << sp.freezeDirSectors << " sectors\n"
                      << "  Service slots:     " << sp.serviceSlots << " x " << sp.serviceSlotSize / 2
                      << " KB, directory " << sp.serviceDirSectors << " sectors\n";
        }
    }
    uint8_t c[512];
    if (img.readConfig(c, err)) {
        if (c[0] == 0 && c[1] == 0) {
            std::cout << "Configuration (sector 1): none - the hypervisor reports it invalid and runs onboarding\n";
        } else {
            std::cout << "Configuration (sector 1):\n";
            printConfig(c);
        }
        if (sp.valid && sp.config[0] && std::memcmp(sp.config, c, 512))
            std::cout << "  note: the system partition's copy (read by older hypervisors) differs\n";
    }
    return 0;
}

int cmdList(int argc, char** argv) {
    if (argc < 1) { sdUsage(); return 1; }
    SdCardImage img;
    if (!openCard(img, argv[0], false)) return 1;
    std::string dir = argc > 1 ? argv[1] : "";
    std::vector<SdCardImage::Entry> entries;
    std::string err;
    if (!img.list(dir, entries, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    uint64_t total = 0;
    for (const auto& e : entries) {
        if (e.attr & 0x02) continue;  // hidden
        std::cout << fatDate(e.date, e.time) << "  ";
        if (e.isDir()) std::cout << std::setw(10) << "<DIR>";
        else std::cout << std::setw(10) << e.size;
        std::cout << "  " << e.name << (e.isDir() ? "/" : "");
        if (!e.isDir() && e.cluster && img.runs(e.cluster).size() > 1) std::cout << "  (fragmented)";
        std::cout << "\n";
        total += e.size;
    }
    std::cout << entries.size() << " entries, " << total << " bytes; "
              << sizeString((uint64_t)img.freeClusters() * img.clusterBytes()) << " free\n";
    return 0;
}

int cmdAdd(int argc, char** argv) {
    bool force = false;
    std::vector<std::string> pos;
    for (int i = 0; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-f" || a == "--force") force = true;
        else pos.push_back(a);
    }
    if (pos.size() < 2 || pos.size() > 3) { sdUsage(); return 1; }
    std::ifstream in(pos[1], std::ios::binary);
    if (!in) { std::cerr << "Error: cannot read " << pos[1] << "\n"; return 1; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    SdCardImage img;
    if (!openCard(img, pos[0], true)) return 1;
    std::string leaf = std::filesystem::path(pos[1]).filename().string();
    std::string target = pos.size() > 2 ? pos[2] : leaf;
    SdCardImage::Entry e;
    std::string err;
    if (!target.empty() && target.back() == '/') target += leaf;
    else if (img.stat(target, e, err) && e.isDir()) target += "/" + leaf;
    if (!img.writeFile(target, data, force, err)) {
        std::cerr << "Error: " << err << (err.find("exists") != std::string::npos ? " (use -f to replace it)" : "")
                  << "\n";
        return 1;
    }
    std::cout << "Added " << target << " (" << data.size() << " bytes)\n";
    return finish(img);
}

int cmdExtract(int argc, char** argv) {
    if (argc < 2) { sdUsage(); return 1; }
    SdCardImage img;
    if (!openCard(img, argv[0], false)) return 1;
    std::vector<uint8_t> data;
    std::string err;
    if (!img.readFile(argv[1], data, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    std::string out = argc > 2 ? argv[2] : std::filesystem::path(argv[1]).filename().string();
    std::ofstream o(out, std::ios::binary);
    o.write((const char*)data.data(), (std::streamsize)data.size());
    if (!o) { std::cerr << "Error: cannot write " << out << "\n"; return 1; }
    std::cout << "Extracted " << argv[1] << " -> " << out << " (" << data.size() << " bytes)\n";
    return 0;
}

int cmdSimple(int argc, char** argv, int nargs, const char* done,
              bool (*op)(SdCardImage&, char**, std::string&)) {
    if (argc < nargs) { sdUsage(); return 1; }
    SdCardImage img;
    if (!openCard(img, argv[0], true)) return 1;
    std::string err;
    if (!op(img, argv + 1, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    std::cout << done << " " << argv[1] << (nargs > 2 ? std::string(" -> ") + argv[2] : "") << "\n";
    return finish(img);
}

int cmdCheck(int argc, char** argv) {
    if (argc < 1) { sdUsage(); return 1; }
    SdCardImage img;
    if (!openCard(img, argv[0], false)) return 1;
    auto r = img.check();
    for (const auto& e : r.errors) std::cout << "error: " << e << "\n";
    for (const auto& w : r.warnings) std::cout << "warning: " << w << "\n";
    std::cout << r.files << " files, " << r.dirs << " directories, " << r.usedClusters << " clusters used, "
              << r.freeClusters << " free\n";
    SdCardImage::SysPart sp;
    std::string err;
    if (!img.readSysPart(sp, err)) std::cout << "note: " << err << "\n";
    else if (!sp.valid) std::cout << "note: the system partition has no MEGA65SYS00 header\n";
    std::cout << (r.errors.empty() ? "OK\n" : "ERRORS FOUND\n");
    return r.errors.empty() ? 0 : 1;
}

int cmdConfig(int argc, char** argv) {
    if (argc < 1) { sdUsage(); return 1; }
    bool change = argc > 1;
    SdCardImage img;
    if (!openCard(img, argv[0], change)) return 1;
    uint8_t c[512];
    std::string err;
    if (!img.readConfig(c, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    if (c[0] == 0 && c[1] == 0) {
        // no configuration yet: the system partition's (older location), else FDISK's
        SdCardImage::SysPart sp;
        if (img.readSysPart(sp, err) && sp.valid && sp.config[0]) std::memcpy(c, sp.config, 512);
        else fdiskConfig(c);
        if (!change) std::cout << "(no configuration sector yet; showing what would be written)\n";
    }
    if (!applyConfig(c, std::vector<std::string>(argv + 1, argv + argc), nullptr)) return 1;
    if (change && !img.writeConfig(c, err)) { std::cerr << "Error: " << err << "\n"; return 1; }
    std::cout << "MEGA65 system configuration:\n";
    printConfig(c);
    return change ? finish(img) : 0;
}

}  // namespace

int disk45_sd(int argc, char** argv) {
    if (argc < 1 || std::string(argv[0]) == "-h" || std::string(argv[0]) == "--help") {
        sdUsage();
        return argc < 1 ? 1 : 0;
    }
    std::string cmd = argv[0];
    argc--;
    argv++;
    if (cmd == "create") return cmdCreate(argc, argv);
    if (cmd == "info") return cmdInfo(argc, argv);
    if (cmd == "list" || cmd == "ls" || cmd == "dir") return cmdList(argc, argv);
    if (cmd == "add" || cmd == "put") return cmdAdd(argc, argv);
    if (cmd == "extract" || cmd == "get") return cmdExtract(argc, argv);
    if (cmd == "rm" || cmd == "remove" || cmd == "delete")
        return cmdSimple(argc, argv, 2, "Removed",
                         [](SdCardImage& i, char** a, std::string& e) { return i.remove(a[0], e); });
    if (cmd == "mkdir")
        return cmdSimple(argc, argv, 2, "Created",
                         [](SdCardImage& i, char** a, std::string& e) { return i.mkdir(a[0], e); });
    if (cmd == "mv" || cmd == "rename")
        return cmdSimple(argc, argv, 3, "Renamed",
                         [](SdCardImage& i, char** a, std::string& e) { return i.rename(a[0], a[1], e); });
    if (cmd == "check" || cmd == "validate") return cmdCheck(argc, argv);
    if (cmd == "config") return cmdConfig(argc, argv);
    std::cerr << "Error: unknown sd command " << cmd << "\n";
    sdUsage();
    return 1;
}
