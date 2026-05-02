#include "O45Reader.hpp"
#include <cstring>

static uint16_t readU16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t readU32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

bool O45Reader::read(const std::vector<uint8_t>& data, O45File& out, std::string& errorMsg) {
    // Minimum header: marker(2) + magic(3) + version(1) + mode(2) = 8 bytes
    if (data.size() < 8) {
        errorMsg = "file too small for header";
        return false;
    }

    // Detect cc65 object format (magic: "uz\0\0") — incompatible with .o65/.o45
    if (data.size() >= 4 && data[0] == 0x75 && data[1] == 0x7A && data[2] == 0x00 && data[3] == 0x00) {
        errorMsg = "incompatible format: cc65 object file (ca65/ld65). "
                   "Supported formats: Andre Fachat .o65 (xa65) and .o45 (ca45/cc45)";
        return false;
    }

    // Validate Andre Fachat .o65/.o45 markers and magic
    if (data[0] != O45_MARKER1 || data[1] != O45_MARKER2) {
        errorMsg = "unrecognized file format (expected .o65 or .o45). "
                   "Supported formats: Andre Fachat .o65 (xa65) and .o45 (ca45/cc45)";
        return false;
    }
    if (data[2] != O45_MAGIC[0] || data[3] != O45_MAGIC[1] || data[4] != O45_MAGIC[2]) {
        errorMsg = "invalid magic (expected 'o65'). "
                   "Supported formats: Andre Fachat .o65 (xa65) and .o45 (ca45/cc45)";
        return false;
    }
    if (data[5] != O45_VERSION) {
        errorMsg = "unsupported version";
        return false;
    }

    out.mode = readU16(&data[6]);
    bool is32 = (out.mode & O45_MODE_SIZE32) != 0;
    bool hasCpuExt = (out.mode & O45_MODE_CPUEXT) != 0;

    // Field width: 2 bytes for .o65, 4 bytes for .o45
    int fw = is32 ? 4 : 2;

    // Header after mode: 4 segments * 2 fields (base+len) * fw, plus optional cpu ext byte
    size_t segFieldsSize = 4 * 2 * fw; // tbase,tlen,dbase,dlen,bbase,blen,zbase,zlen
    size_t headerEnd = 8 + segFieldsSize + (hasCpuExt ? 1 : 0);

    if (data.size() < headerEnd) {
        errorMsg = "file too small for header fields";
        return false;
    }

    // Read segment base/length fields
    size_t off = 8;
    auto readField = [&]() -> uint32_t {
        uint32_t val;
        if (is32) { val = readU32(&data[off]); off += 4; }
        else      { val = readU16(&data[off]); off += 2; }
        return val;
    };

    out.tbase = readField(); out.tlen = readField();
    out.dbase = readField(); out.dlen = readField();
    out.bbase = readField(); out.blen = readField();
    out.zbase = readField(); out.zlen = readField();

    // CPU ext byte
    if (hasCpuExt) {
        out.cpuId = data[off++];
    } else {
        // Derive CPU from mode bits 8-10
        uint8_t cpuBits = (out.mode >> 8) & 0x07;
        out.cpuId = cpuBits; // 0=6502, 1=65C02, etc.
    }

    // External symbol index width: 2 bytes for .o65, 4 bytes for .o45
    int symIdxWidth = is32 ? 4 : 2;

    // Parse relocation table (shared logic for text and data)
    auto parseRelocTable = [&](std::vector<uint8_t>& relocOut) -> bool {
        while (off < data.size() && data[off] != 0) {
            uint8_t delta = data[off++];
            relocOut.push_back(delta);
            // $FF = escape byte (advance 254, read next delta). No type/seg follows.
            if (delta == 0xFF) continue;
            if (off >= data.size()) { errorMsg = "truncated reloc table"; return false; }
            uint8_t typeSeg = data[off++];
            relocOut.push_back(typeSeg);
            // R_HIGH has an extra byte
            if ((typeSeg & O45_RTYPE_MASK) == R_HIGH) {
                if (off >= data.size()) { errorMsg = "truncated R_HIGH extra byte"; return false; }
                relocOut.push_back(data[off++]);
            }
            // External references have a symbol index
            if ((typeSeg & O45_RSEG_MASK) == SEG_EXTERNAL) {
                if (off + symIdxWidth > data.size()) { errorMsg = "truncated external symbol index"; return false; }
                if (is32) {
                    for (int i = 0; i < 4; i++) relocOut.push_back(data[off++]);
                } else {
                    // Read 16-bit index, store as 4 bytes (widened for uniform O45File format)
                    uint16_t idx16 = readU16(&data[off]); off += 2;
                    relocOut.push_back((uint8_t)(idx16 & 0xFF));
                    relocOut.push_back((uint8_t)(idx16 >> 8));
                    relocOut.push_back(0);
                    relocOut.push_back(0);
                }
            }
        }
        if (off >= data.size()) { errorMsg = "missing reloc table terminator"; return false; }
        off++; // skip $00
        return true;
    };

    // Parse options
    while (off < data.size() && data[off] != 0) {
        uint8_t len = data[off];
        if (off + len > data.size()) { errorMsg = "option overflows file"; return false; }
        O45File::Option opt;
        opt.type = data[off + 1];
        opt.data.assign(data.begin() + off + 2, data.begin() + off + len);
        out.options.push_back(opt);
        off += len;
    }
    if (off >= data.size()) { errorMsg = "missing option terminator"; return false; }
    off++; // skip $00

    // Text body
    if (off + out.tlen > data.size()) { errorMsg = "text body overflows file"; return false; }
    out.textBody.assign(data.begin() + off, data.begin() + off + out.tlen);
    off += out.tlen;

    // Text relocation table
    if (!parseRelocTable(out.textRelocs)) return false;

    // Data body
    if (off + out.dlen > data.size()) { errorMsg = "data body overflows file"; return false; }
    out.dataBody.assign(data.begin() + off, data.begin() + off + out.dlen);
    off += out.dlen;

    // Data relocation table
    if (!parseRelocTable(out.dataRelocs)) return false;

    // Import table
    int countWidth = is32 ? 4 : 2;
    if (off + countWidth > data.size()) { errorMsg = "truncated import count"; return false; }
    uint32_t importCount = is32 ? readU32(&data[off]) : readU16(&data[off]);
    off += countWidth;
    for (uint32_t i = 0; i < importCount; i++) {
        O45File::Import imp;
        size_t end = off;
        while (end < data.size() && data[end] != 0) end++;
        if (end >= data.size()) { errorMsg = "truncated import name"; return false; }
        imp.name.assign(data.begin() + off, data.begin() + end);
        off = end + 1;
        out.imports.push_back(imp);
    }

    // Export table
    if (off + countWidth > data.size()) { errorMsg = "truncated export count"; return false; }
    uint32_t exportCount = is32 ? readU32(&data[off]) : readU16(&data[off]);
    off += countWidth;
    for (uint32_t i = 0; i < exportCount; i++) {
        O45File::Export exp;
        size_t end = off;
        while (end < data.size() && data[end] != 0) end++;
        if (end >= data.size()) { errorMsg = "truncated export name"; return false; }
        exp.name.assign(data.begin() + off, data.begin() + end);
        off = end + 1;
        // segment byte + offset (2 or 4 bytes)
        if (off + 1 + fw > data.size()) { errorMsg = "truncated export entry"; return false; }
        exp.segment = data[off++];
        exp.offset = is32 ? readU32(&data[off]) : readU16(&data[off]);
        off += fw;
        out.exports.push_back(exp);
    }

    return true;
}
