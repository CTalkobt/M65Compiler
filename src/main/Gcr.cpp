#include "Gcr.hpp"
#include <cstring>

namespace gcr {

namespace {

// nybble -> 5-bit GCR code, and back (0xFF: not a GCR code)
const uint8_t toGcr[16] = {0x0A, 0x0B, 0x12, 0x13, 0x0E, 0x0F, 0x16, 0x17,
                           0x09, 0x19, 0x1A, 0x1B, 0x0D, 0x1D, 0x1E, 0x15};
uint8_t fromGcr[32];
bool tablesReady = false;

void initTables() {
    if (tablesReady) return;
    std::memset(fromGcr, 0xFF, sizeof fromGcr);
    for (int n = 0; n < 16; n++) fromGcr[toGcr[n]] = (uint8_t)n;
    tablesReady = true;
}

// A track as a bit stream
struct Bits {
    const uint8_t* bytes;
    size_t len;          // bytes
    size_t total;        // bits
    bool circular;
    int at(size_t i) const {
        if (i >= total) {
            if (!circular) return -1;
            i %= total;
        }
        return (bytes[i / 8] >> (7 - i % 8)) & 1;
    }
    // n GCR bytes from bit position p, decoded (n a multiple of 5); false
    // on a bit past the end or a code that is not GCR
    bool decode(size_t p, int n, uint8_t* out) const {
        for (int g = 0; g < n * 8 / 5; g++) {
            int code = 0;
            for (int b = 0; b < 5; b++) {
                int bit = at(p + g * 5 + b);
                if (bit < 0) return false;
                code = (code << 1) | bit;
            }
            uint8_t nyb = fromGcr[code];
            if (nyb == 0xFF) return false;
            if (g % 2 == 0) out[g / 2] = (uint8_t)(nyb << 4);
            else out[g / 2] |= nyb;
        }
        return true;
    }
    // The bit after the next sync (10+ 1 bits) at or after p, before limit
    bool nextSync(size_t p, size_t limit, size_t& after) const {
        int ones = 0;
        for (size_t i = p; i < limit; i++) {
            int bit = at(i);
            if (bit < 0) return false;
            if (bit) ones++;
            else {
                if (ones >= 10) { after = i; return true; }
                ones = 0;
            }
        }
        return false;
    }
};

}  // namespace

std::vector<Sector> decodeTrack(const uint8_t* bytes, size_t len, bool circular) {
    initTables();
    std::vector<Sector> out;
    if (len < 16) return out;
    Bits bits{bytes, len, len * 8, circular};
    std::vector<bool> seen(256, false);
    // every sync in one pass over the track (a sector may wrap past the end)
    size_t p = 0;
    size_t after;
    while (p < bits.total && bits.nextSync(p, bits.total, after)) {
        p = after;
        uint8_t hdr[8];
        if (!bits.decode(after, 10, hdr) || hdr[0] != 0x08 || hdr[1] != (hdr[2] ^ hdr[3] ^ hdr[4] ^ hdr[5])) continue;
        // the data block's sync follows the header gap (within ~60 bytes)
        size_t dataStart;
        if (!bits.nextSync(after + 80, after + 80 + 60 * 8, dataStart)) continue;
        uint8_t block[260];
        if (!bits.decode(dataStart, 325, block) || block[0] != 0x07) continue;
        uint8_t sum = 0;
        for (int i = 1; i <= 256; i++) sum ^= block[i];
        if (sum != block[257]) continue;
        int sec = hdr[2];
        if (!seen[sec]) {
            seen[sec] = true;
            Sector s;
            s.track = hdr[3];
            s.sector = sec;
            s.id2 = hdr[4];
            s.id1 = hdr[5];
            std::memcpy(s.data, block + 1, 256);
            out.push_back(s);
        }
        p = dataStart + 325 * 8;
    }
    return out;
}

int speedZone(int track) {
    return track <= 17 ? 3 : track <= 24 ? 2 : track <= 30 ? 1 : 0;
}

int sectorsOnTrack(int track) {
    return track <= 17 ? 21 : track <= 24 ? 19 : track <= 30 ? 18 : 17;
}

size_t trackLength(int zone) {
    static const size_t len[4] = {6250, 6666, 7142, 7692};
    return len[zone & 3];
}

namespace {

// 4 bytes -> 5 GCR bytes
void put4(std::vector<uint8_t>& out, const uint8_t* in) {
    uint64_t v = 0;
    for (int i = 0; i < 4; i++) {
        v = (v << 5) | toGcr[in[i] >> 4];
        v = (v << 5) | toGcr[in[i] & 15];
    }
    for (int i = 4; i >= 0; i--) out.push_back((uint8_t)(v >> (8 * i)));
}

}  // namespace

std::vector<uint8_t> encodeTrack(int track, const std::vector<const uint8_t*>& sectors, uint8_t id1, uint8_t id2,
                                 int zone) {
    initTables();
    std::vector<uint8_t> t;
    size_t n = sectors.size();
    size_t length = trackLength(zone);
    const size_t perSector = 5 + 10 + 9 + 5 + 325;
    size_t gap = n ? (length > n * perSector ? (length - n * perSector) / n : 0) : length;
    for (size_t s = 0; s < n; s++) {
        for (int i = 0; i < 5; i++) t.push_back(0xFF);
        uint8_t hdr[8] = {0x08, 0, (uint8_t)s, (uint8_t)track, id2, id1, 0x0F, 0x0F};
        hdr[1] = hdr[2] ^ hdr[3] ^ hdr[4] ^ hdr[5];
        put4(t, hdr);
        put4(t, hdr + 4);
        for (int i = 0; i < 9; i++) t.push_back(0x55);
        for (int i = 0; i < 5; i++) t.push_back(0xFF);
        uint8_t block[260];
        block[0] = 0x07;
        std::memcpy(block + 1, sectors[s], 256);
        uint8_t sum = 0;
        for (int i = 1; i <= 256; i++) sum ^= block[i];
        block[257] = sum;
        block[258] = block[259] = 0x00;
        for (int i = 0; i < 260; i += 4) put4(t, block + i);
        for (size_t i = 0; i < gap; i++) t.push_back(0x55);
    }
    while (t.size() < length) t.push_back(0x55);
    return t;
}

}  // namespace gcr
