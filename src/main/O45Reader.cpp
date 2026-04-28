#include "O45Reader.hpp"
#include <cstring>

static uint16_t readU16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t readU32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

bool O45Reader::read(const std::vector<uint8_t>& data, O45File& out, std::string& errorMsg) {
    if (data.size() < (size_t)O45_HEADER_SIZE) {
        errorMsg = "file too small for .o45 header";
        return false;
    }

    // Validate markers and magic
    if (data[0] != O45_MARKER1 || data[1] != O45_MARKER2) {
        errorMsg = "invalid marker bytes";
        return false;
    }
    if (data[2] != O45_MAGIC[0] || data[3] != O45_MAGIC[1] || data[4] != O45_MAGIC[2]) {
        errorMsg = "invalid magic (expected 'o65')";
        return false;
    }
    if (data[5] != O45_VERSION) {
        errorMsg = "unsupported version";
        return false;
    }

    out.mode = readU16(&data[6]);
    if (!(out.mode & O45_MODE_SIZE32)) {
        errorMsg = "SIZE32 not set (not a 32-bit .o45 file)";
        return false;
    }

    out.tbase = readU32(&data[8]);
    out.tlen  = readU32(&data[12]);
    out.dbase = readU32(&data[16]);
    out.dlen  = readU32(&data[20]);
    out.bbase = readU32(&data[24]);
    out.blen  = readU32(&data[28]);
    out.zbase = readU32(&data[32]);
    out.zlen  = readU32(&data[36]);
    out.cpuId = data[40];

    size_t off = O45_HEADER_SIZE;

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
    off++; // skip $00 terminator

    // Text body
    if (off + out.tlen > data.size()) { errorMsg = "text body overflows file"; return false; }
    out.textBody.assign(data.begin() + off, data.begin() + off + out.tlen);
    off += out.tlen;

    // Text relocation table
    while (off < data.size() && data[off] != 0) {
        uint8_t delta = data[off++];
        out.textRelocs.push_back(delta);
        if (off >= data.size()) { errorMsg = "truncated text reloc"; return false; }
        uint8_t typeSeg = data[off++];
        out.textRelocs.push_back(typeSeg);
        if ((typeSeg & O45_RTYPE_MASK) == R_HIGH) {
            if (off >= data.size()) { errorMsg = "truncated R_HIGH extra"; return false; }
            out.textRelocs.push_back(data[off++]);
        }
        if ((typeSeg & O45_RSEG_MASK) == SEG_EXTERNAL) {
            if (off + 4 > data.size()) { errorMsg = "truncated external index"; return false; }
            for (int i = 0; i < 4; i++) out.textRelocs.push_back(data[off++]);
        }
    }
    if (off >= data.size()) { errorMsg = "missing text reloc terminator"; return false; }
    off++; // skip $00

    // Data body
    if (off + out.dlen > data.size()) { errorMsg = "data body overflows file"; return false; }
    out.dataBody.assign(data.begin() + off, data.begin() + off + out.dlen);
    off += out.dlen;

    // Data relocation table
    while (off < data.size() && data[off] != 0) {
        uint8_t delta = data[off++];
        out.dataRelocs.push_back(delta);
        if (off >= data.size()) { errorMsg = "truncated data reloc"; return false; }
        uint8_t typeSeg = data[off++];
        out.dataRelocs.push_back(typeSeg);
        if ((typeSeg & O45_RTYPE_MASK) == R_HIGH) {
            if (off >= data.size()) { errorMsg = "truncated R_HIGH extra"; return false; }
            out.dataRelocs.push_back(data[off++]);
        }
        if ((typeSeg & O45_RSEG_MASK) == SEG_EXTERNAL) {
            if (off + 4 > data.size()) { errorMsg = "truncated external index"; return false; }
            for (int i = 0; i < 4; i++) out.dataRelocs.push_back(data[off++]);
        }
    }
    if (off >= data.size()) { errorMsg = "missing data reloc terminator"; return false; }
    off++; // skip $00

    // Import table
    if (off + 4 > data.size()) { errorMsg = "truncated import count"; return false; }
    uint32_t importCount = readU32(&data[off]); off += 4;
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
    if (off + 4 > data.size()) { errorMsg = "truncated export count"; return false; }
    uint32_t exportCount = readU32(&data[off]); off += 4;
    for (uint32_t i = 0; i < exportCount; i++) {
        O45File::Export exp;
        size_t end = off;
        while (end < data.size() && data[end] != 0) end++;
        if (end >= data.size()) { errorMsg = "truncated export name"; return false; }
        exp.name.assign(data.begin() + off, data.begin() + end);
        off = end + 1;
        if (off + 5 > data.size()) { errorMsg = "truncated export entry"; return false; }
        exp.segment = data[off++];
        exp.offset = readU32(&data[off]); off += 4;
        out.exports.push_back(exp);
    }

    return true;
}
