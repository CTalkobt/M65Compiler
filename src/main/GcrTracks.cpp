#include "GcrTracks.hpp"
#include "Gcr.hpp"
#include <algorithm>
#include <cstring>

namespace {

uint32_t get32(const std::vector<uint8_t>& f, size_t o) {
    return f[o] | (f[o + 1] << 8) | (f[o + 2] << 16) | ((uint32_t)f[o + 3] << 24);
}

void put32(std::vector<uint8_t>& f, size_t o, uint32_t v) {
    for (int i = 0; i < 4; i++) f[o + i] = (uint8_t)(v >> (8 * i));
}

}  // namespace

bool GcrTracks::isGcrFile(const std::vector<uint8_t>& data, bool* doubleSided) {
    if (data.size() < 12) return false;
    bool g41 = std::memcmp(data.data(), "GCR-1541", 8) == 0;
    bool g71 = std::memcmp(data.data(), "GCR-1571", 8) == 0;
    if (doubleSided) *doubleSided = g71;
    return g41 || g71;
}

int GcrTracks::logicalTrack(int entry) const {
    int side = entry / 84, idx = entry % 84;
    if (idx % 2) return 0;  // half track
    int t = idx / 2 + 1;
    if (side == 0) return t;
    return t <= 35 ? t + 35 : 0;  // the 1571's second side: tracks 36-70
}

void GcrTracks::reset(int sides) {
    sides_ = sides;
    entries_ = 84 * sides;
    maxTrack_ = 7928;
    raw_.assign(entries_, {});
    speed_.assign(entries_, 0);
    speedBlocks_.clear();
    loadedSectors_.clear();
    haveId_ = false;
    decoded_ = 0;
}

bool GcrTracks::load(const std::vector<uint8_t>& file, DiskImage& img, int sides) {
    if (!isGcrFile(file)) return false;
    reset(sides);
    int n = std::min<int>(file[9], 84 * sides);
    uint16_t maxSize = file[10] | (file[11] << 8);
    if (maxSize > maxTrack_) maxTrack_ = maxSize;
    size_t speedTable = 12 + (size_t)file[9] * 4;
    if (speedTable + (size_t)file[9] * 4 > file.size()) return false;
    for (int e = 0; e < n; e++) {
        uint32_t off = get32(file, 12 + e * 4);
        speed_[e] = get32(file, speedTable + e * 4);
        if (off == 0 || (size_t)off + 2 > file.size()) continue;
        size_t len = file[off] | (file[off + 1] << 8);
        if (off + 2 + len > file.size()) len = file.size() - off - 2;
        raw_[e].assign(file.begin() + off + 2, file.begin() + off + 2 + len);
        if (len > maxTrack_) maxTrack_ = (uint16_t)std::min<size_t>(len, 0xFFFF);
        if (speed_[e] > 3) {
            // a per-byte speed block: copied with the track (1 byte per 4)
            size_t blen = (len + 3) / 4, so = speed_[e];
            if (so + blen <= file.size()) {
                speed_[e] = 0x80000000u | (uint32_t)speedBlocks_.size();
                speedBlocks_.insert(speedBlocks_.end(), file.begin() + so, file.begin() + so + blen);
            } else {
                speed_[e] = gcr::speedZone(((e % 84) / 2) + 1);
            }
        }
        int lt = logicalTrack(e);
        if (lt == 0 || lt > img.totalTracks()) continue;
        for (const auto& s : gcr::decodeTrack(raw_[e].data(), raw_[e].size(), true)) {
            if (s.track != lt || s.sector >= img.sectorsOnTrack(lt)) continue;
            uint8_t* dst = img.sectorData(lt, s.sector);
            if (!dst) continue;
            std::memcpy(dst, s.data, 256);
            decoded_++;
            if (!haveId_) { id1_ = s.id1; id2_ = s.id2; haveId_ = true; }
        }
    }
    // the sectors as loaded, to find the tracks a change touched
    loadedSectors_.assign(img.totalTracks() + 1, {});
    for (int t = 1; t <= img.totalTracks(); t++)
        for (int s = 0; s < img.sectorsOnTrack(t); s++) {
            const uint8_t* d = img.sectorData(t, s);
            if (d) loadedSectors_[t].insert(loadedSectors_[t].end(), d, d + 256);
        }
    return true;
}

std::vector<uint8_t> GcrTracks::save(const DiskImage& img) const {
    int entries = 84 * sides_;
    // the format ID: from the headers read, else the BAM's disk ID
    uint8_t id1 = id1_, id2 = id2_;
    if (!haveId_) {
        const uint8_t* bam = img.sectorData(18, 0);
        id1 = bam ? bam[0xA2] : '0';
        id2 = bam ? bam[0xA3] : '0';
    }
    std::vector<std::vector<uint8_t>> tracks(entries);
    std::vector<uint32_t> speeds(entries, 0);
    std::vector<std::vector<uint8_t>> speedBlock(entries);
    for (int e = 0; e < entries; e++) {
        const std::vector<uint8_t> none;
        const auto& raw = e < (int)raw_.size() ? raw_[e] : none;
        uint32_t sp = e < (int)speed_.size() ? speed_[e] : 0;
        auto keepRaw = [&]() {
            tracks[e] = raw;
            if (sp & 0x80000000u) {
                size_t at = sp & 0x7FFFFFFF, blen = (raw.size() + 3) / 4;
                speedBlock[e].assign(speedBlocks_.begin() + at, speedBlocks_.begin() + at + blen);
            } else {
                speeds[e] = sp;
            }
        };
        int lt = logicalTrack(e);
        if (lt == 0 || lt > img.totalTracks()) {
            if (!raw.empty()) keepRaw();
            continue;
        }
        std::vector<uint8_t> now;
        std::vector<const uint8_t*> secs;
        for (int s = 0; s < img.sectorsOnTrack(lt); s++) {
            const uint8_t* d = img.sectorData(lt, s);
            now.insert(now.end(), d, d + 256);
            secs.push_back(d);
        }
        if (!raw.empty() && lt < (int)loadedSectors_.size() && loadedSectors_[lt] == now) {
            keepRaw();
            continue;
        }
        int physical = lt > 35 ? lt - 35 : lt;
        int zone = gcr::speedZone(physical);
        tracks[e] = gcr::encodeTrack(lt, secs, id1, id2, zone);
        speeds[e] = (uint32_t)zone;
    }
    size_t maxLen = maxTrack_;
    for (const auto& t : tracks) maxLen = std::max(maxLen, t.size());
    if (maxLen > 0xFFFF) maxLen = 0xFFFF;

    size_t headerLen = 12 + (size_t)entries * 8;
    std::vector<uint8_t> out(headerLen, 0);
    std::memcpy(out.data(), sides_ == 2 ? "GCR-1571" : "GCR-1541", 8);
    out[8] = 0;
    out[9] = (uint8_t)entries;
    out[10] = (uint8_t)maxLen;
    out[11] = (uint8_t)(maxLen >> 8);
    for (int e = 0; e < entries; e++) {
        if (tracks[e].empty()) continue;
        put32(out, 12 + e * 4, (uint32_t)out.size());
        size_t len = std::min(tracks[e].size(), maxLen);
        out.push_back((uint8_t)len);
        out.push_back((uint8_t)(len >> 8));
        out.insert(out.end(), tracks[e].begin(), tracks[e].begin() + len);
        out.insert(out.end(), maxLen - len, 0xFF);
    }
    for (int e = 0; e < entries; e++) {
        if (!speedBlock[e].empty()) {
            put32(out, 12 + entries * 4 + e * 4, (uint32_t)out.size());
            out.insert(out.end(), speedBlock[e].begin(), speedBlock[e].end());
        } else {
            put32(out, 12 + entries * 4 + e * 4, speeds[e]);
        }
    }
    return out;
}
