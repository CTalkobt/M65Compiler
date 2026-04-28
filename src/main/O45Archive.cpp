#include "O45Archive.hpp"
#include <cstring>

static uint16_t readU16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t readU32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

static void writeU16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)(val >> 8));
}

static void writeU32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)((val >> 8) & 0xFF));
    out.push_back((uint8_t)((val >> 16) & 0xFF));
    out.push_back((uint8_t)((val >> 24) & 0xFF));
}

static void writeString(std::vector<uint8_t>& out, const std::string& str) {
    for (char c : str) out.push_back((uint8_t)c);
    out.push_back(0x00);
}

// =============================================================================
// Ar45Writer
// =============================================================================

std::vector<uint8_t> Ar45Writer::write(const Ar45Archive& archive) {
    std::vector<uint8_t> out;

    // Magic
    out.insert(out.end(), AR45_MAGIC, AR45_MAGIC + 4);

    // Version
    out.push_back(AR45_VERSION);

    // Member count
    writeU32(out, (uint32_t)archive.members.size());

    // Symbol count
    writeU32(out, (uint32_t)archive.symbols.size());

    // Symbol index
    for (const auto& sym : archive.symbols) {
        writeString(out, sym.name);
        writeU32(out, sym.memberIndex);
    }

    // Members
    for (const auto& member : archive.members) {
        writeU16(out, (uint16_t)member.name.size());
        out.insert(out.end(), member.name.begin(), member.name.end());
        writeU32(out, (uint32_t)member.data.size());
        out.insert(out.end(), member.data.begin(), member.data.end());
    }

    return out;
}

// =============================================================================
// Ar45Reader
// =============================================================================

bool Ar45Reader::read(const std::vector<uint8_t>& data, Ar45Archive& out, std::string& errorMsg) {
    if (data.size() < 13) {
        errorMsg = "file too small for archive header";
        return false;
    }

    // Magic
    if (memcmp(data.data(), AR45_MAGIC, 4) != 0) {
        errorMsg = "not an ar45 archive (invalid magic)";
        return false;
    }

    // Version
    if (data[4] != AR45_VERSION) {
        errorMsg = "unsupported archive version";
        return false;
    }

    size_t off = 5;
    uint32_t memberCount = readU32(&data[off]); off += 4;
    uint32_t symbolCount = readU32(&data[off]); off += 4;

    // Symbol index
    out.symbols.clear();
    for (uint32_t i = 0; i < symbolCount; i++) {
        Ar45SymbolEntry sym;
        size_t end = off;
        while (end < data.size() && data[end] != 0) end++;
        if (end >= data.size()) { errorMsg = "truncated symbol name"; return false; }
        sym.name.assign(data.begin() + off, data.begin() + end);
        off = end + 1;
        if (off + 4 > data.size()) { errorMsg = "truncated symbol index"; return false; }
        sym.memberIndex = readU32(&data[off]); off += 4;
        out.symbols.push_back(sym);
    }

    // Members
    out.members.clear();
    for (uint32_t i = 0; i < memberCount; i++) {
        if (off + 2 > data.size()) { errorMsg = "truncated member name length"; return false; }
        uint16_t nameLen = readU16(&data[off]); off += 2;
        if (off + nameLen > data.size()) { errorMsg = "truncated member name"; return false; }
        Ar45Member member;
        member.name.assign(data.begin() + off, data.begin() + off + nameLen);
        off += nameLen;
        if (off + 4 > data.size()) { errorMsg = "truncated member data length"; return false; }
        uint32_t dataLen = readU32(&data[off]); off += 4;
        if (off + dataLen > data.size()) { errorMsg = "truncated member data"; return false; }
        member.data.assign(data.begin() + off, data.begin() + off + dataLen);
        off += dataLen;
        out.members.push_back(member);
    }

    return true;
}
