// SdCardImage — MEGA65 SD card images (xemu's mega65.img, or a dump of a
// real card): an MBR partition table, a FAT32 data partition (type $0B/$0C)
// and the MEGA65 system partition (type $41).
//
// The image is read and written sector by sector (cards are gigabytes), the
// FAT is kept in memory. Files are always stored in one contiguous run of
// clusters: the MEGA65 mounts only unfragmented D81 images.
//
// create() lays out a new card the way the MEGA65's FDISK utility does
// (mega65-fdisk, also copied by xemu's sdcontent.c): MBR with the FAT32
// partition from sector 2048 and the system partition at the end, FAT32 with
// 568 reserved sectors and 4 KB clusters, the system partition header
// ("MEGA65SYS00", freeze and service slot areas), and the configuration
// sector in card sector 1 (copied to the system partition's sector 1, where
// older hypervisors read it).
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class SdCardImage {
public:
    struct Partition {
        int index = 0;          // 0..3 (MBR slot)
        uint8_t type = 0;
        uint32_t start = 0;     // first sector (LBA)
        uint32_t sectors = 0;
    };

    struct CreateOptions {
        uint64_t bytes = 0;                 // image size
        std::string label = "M.E.G.A.65!";  // volume label (11 chars)
        bool sysPart = true;                // add the MEGA65 system partition
        uint32_t sysPartSectors = 0;        // 0: FDISK's policy (half, at most 2 GB)
        bool ntsc = true;                   // mega65-fdisk's default is $80 (NTSC)
        std::string defaultDisk = "mega65.d81";
        uint8_t mac[6] = {0x41, 0x41, 0x41, 0x41, 0x41, 0x41};
    };

    // A directory entry (long name if there is one)
    struct Entry {
        std::string name;       // long name, else the short name (case per NT flags)
        std::string shortName;  // 8.3 as stored ("NAME.EXT")
        uint8_t attr = 0;
        uint32_t cluster = 0;
        uint32_t size = 0;
        uint16_t date = 0, time = 0;
        bool isDir() const { return attr & 0x10; }
        // where the entries are in the directory (for removal)
        uint32_t firstSlot = 0, slot = 0;
    };

    // The MEGA65 system partition's header (config: its sector 1, the old
    // location of the configuration sector)
    struct SysPart {
        bool valid = false;             // "MEGA65SYS00" found
        uint32_t freezeStart = 0, freezeSize = 0, freezeSlotSize = 0;
        uint16_t freezeSlots = 0, freezeDirSectors = 0;
        uint32_t serviceStart = 0, serviceSize = 0, serviceSlotSize = 0;
        uint16_t serviceSlots = 0, serviceDirSectors = 0;
        uint8_t config[512] = {};
    };

    struct CheckResult {
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        uint32_t files = 0, dirs = 0, usedClusters = 0, freeClusters = 0;
    };

    ~SdCardImage();

    static bool create(const std::string& path, const CreateOptions& opt, std::string& err);
    // Partition 0 = the first FAT32 partition
    bool open(const std::string& path, bool writable, std::string& err);
    bool flush(std::string& err);

    const std::vector<Partition>& partitions() const { return parts_; }
    bool hasFat() const { return fatOk_; }

    // FAT32 volume
    std::string volumeLabel() const;
    uint32_t clusterBytes() const { return spc_ * 512; }
    uint32_t clusterCount() const { return clusters_; }
    uint32_t freeClusters() const;
    uint32_t partitionStart() const { return volStart_; }
    uint32_t reservedSectors() const { return reserved_; }
    uint32_t fatSectors() const { return fatSize_; }
    uint8_t fatCount() const { return nFats_; }

    bool list(const std::string& dir, std::vector<Entry>& out, std::string& err);
    bool stat(const std::string& path, Entry& out, std::string& err);
    bool readFile(const std::string& path, std::vector<uint8_t>& data, std::string& err);
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data, bool overwrite,
                   std::string& err);
    bool remove(const std::string& path, std::string& err);
    bool mkdir(const std::string& path, std::string& err);
    bool rename(const std::string& from, const std::string& to, std::string& err);
    // The clusters of a file in runs: 1 run = contiguous
    std::vector<std::pair<uint32_t, uint32_t>> runs(uint32_t cluster) const;
    uint64_t clusterSector(uint32_t cluster) const { return dataStart_ + (uint64_t)(cluster - 2) * spc_; }

    CheckResult check();

    // System partition
    bool readSysPart(SysPart& sp, std::string& err);
    // The configuration sector: card sector 1, where the hypervisor reads it
    // (it moved there from the system partition's sector 1; writeConfig
    // updates that copy too when the card has a system partition)
    bool readConfig(uint8_t config[512], std::string& err);
    bool writeConfig(const uint8_t config[512], std::string& err);

    // Sector access (whole image)
    bool readSector(uint64_t lba, uint8_t* buf);
    bool writeSector(uint64_t lba, const uint8_t* buf);

    static bool parseSize(const std::string& s, uint64_t& bytes);
    static const char* partitionTypeName(uint8_t type);

private:
    bool loadFat(std::string& err);
    uint32_t fatEntry(uint32_t c) const { return fat_[c] & 0x0FFFFFFF; }
    void setFat(uint32_t c, uint32_t v);
    std::vector<uint32_t> chain(uint32_t c) const;
    bool allocRun(uint32_t n, uint32_t& first);
    void freeChain(uint32_t c);
    bool readChain(uint32_t c, std::vector<uint8_t>& buf);
    bool writeChain(uint32_t c, const std::vector<uint8_t>& buf);

    // Directory helpers: a directory's cluster (0 = root)
    bool resolveDir(const std::string& path, uint32_t& cluster, std::string& err);
    bool readDir(uint32_t cluster, std::vector<uint8_t>& raw);
    std::vector<Entry> parseDir(const std::vector<uint8_t>& raw) const;
    bool findIn(uint32_t dirCluster, const std::string& name, Entry& e, std::vector<uint8_t>* raw);
    bool addEntry(uint32_t dirCluster, const std::string& name, uint8_t attr, uint32_t cluster,
                  uint32_t size, std::string& err);
    bool removeEntry(uint32_t dirCluster, const Entry& e, std::string& err);
    std::string makeShortName(const std::vector<Entry>& existing, const std::string& name,
                              bool& needLfn, uint8_t& ntCase) const;

    std::fstream f_;
    bool writable_ = false;
    uint64_t imageSectors_ = 0;
    std::vector<Partition> parts_;
    bool fatOk_ = false;
    uint32_t volStart_ = 0, volSectors_ = 0;
    uint32_t reserved_ = 0, fatSize_ = 0, spc_ = 0, rootCluster_ = 2, fsInfo_ = 1;
    uint8_t nFats_ = 2;
    uint64_t dataStart_ = 0;
    uint32_t clusters_ = 0;     // data clusters (numbered 2..clusters_+1)
    std::vector<uint32_t> fat_;
    std::vector<bool> fatDirty_;    // per FAT sector
    bool fsInfoDirty_ = false;
};
