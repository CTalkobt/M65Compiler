#include "ZipArchive.hpp"
#include <zlib.h>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iterator>

namespace {

uint16_t get16(const std::vector<uint8_t>& b, size_t o) { return b[o] | (b[o + 1] << 8); }
uint32_t get32(const std::vector<uint8_t>& b, size_t o) {
    return b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | ((uint32_t)b[o + 3] << 24);
}
void put16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
void put32(std::vector<uint8_t>& b, uint32_t v) { for (int i = 0; i < 4; i++) b.push_back((v >> (8 * i)) & 0xFF); }

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

}  // namespace

bool ZipArchive::isZip(const std::vector<uint8_t>& data) {
    return data.size() >= 22 && ((data[0] == 'P' && data[1] == 'K' && data[2] == 3 && data[3] == 4) ||
                                 (data[0] == 'P' && data[1] == 'K' && data[2] == 5 && data[3] == 6));
}

bool ZipArchive::load(const std::string& path, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { err = "cannot open " + path; return false; }
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    members_.clear();
    // the end of central directory record (the last one; a comment may follow)
    if (b.size() < 22) { err = path + " is not a zip file"; return false; }
    size_t eocd = std::string::npos;
    for (size_t i = b.size() - 22 + 1; i-- > 0 && b.size() - i <= 22 + 65535;)
        if (get32(b, i) == 0x06054b50) { eocd = i; break; }
    if (eocd == std::string::npos) { err = path + " is not a zip file (no end record)"; return false; }
    uint16_t count = get16(b, eocd + 10);
    uint32_t cdSize = get32(b, eocd + 12), cdOff = get32(b, eocd + 16);
    if (cdOff == 0xFFFFFFFF || count == 0xFFFF) { err = "ZIP64 archives are not supported"; return false; }
    if ((uint64_t)cdOff + cdSize > b.size()) { err = path + ": damaged central directory"; return false; }
    size_t p = cdOff;
    for (int i = 0; i < count; i++) {
        if (p + 46 > b.size() || get32(b, p) != 0x02014b50) { err = path + ": damaged central directory"; return false; }
        Member m;
        uint16_t flags = get16(b, p + 8);
        m.method = get16(b, p + 10);
        m.time = get16(b, p + 12);
        m.date = get16(b, p + 14);
        m.crc = get32(b, p + 16);
        m.compSize = get32(b, p + 20);
        m.size = get32(b, p + 24);
        uint16_t nameLen = get16(b, p + 28), extraLen = get16(b, p + 30), commentLen = get16(b, p + 32);
        m.extAttr = get32(b, p + 38);
        uint32_t local = get32(b, p + 42);
        if (p + 46 + nameLen > b.size()) { err = path + ": damaged central directory"; return false; }
        m.name.assign((const char*)b.data() + p + 46, nameLen);
        p += 46 + nameLen + extraLen + commentLen;
        if (flags & 1) { err = m.name + " is encrypted"; return false; }
        if (local + 30 > b.size() || get32(b, local) != 0x04034b50) { err = path + ": damaged member " + m.name; return false; }
        size_t data = local + 30 + get16(b, local + 26) + get16(b, local + 28);
        if (data + m.compSize > b.size()) { err = path + ": truncated member " + m.name; return false; }
        m.compressed.assign(b.begin() + data, b.begin() + data + m.compSize);
        members_.push_back(std::move(m));
    }
    return true;
}

int ZipArchive::find(const std::string& name) const {
    for (size_t i = 0; i < members_.size(); i++)
        if (lower(members_[i].name) == lower(name)) return (int)i;
    return -1;
}

bool ZipArchive::extract(int index, std::vector<uint8_t>& data, std::string& err) const {
    const Member& m = members_[index];
    if (m.method == 0) {
        data = m.compressed;
    } else if (m.method == 8) {
        data.assign(m.size, 0);
        z_stream s = {};
        if (inflateInit2(&s, -15) != Z_OK) { err = "zlib error"; return false; }
        s.next_in = const_cast<Bytef*>(m.compressed.data());
        s.avail_in = (uInt)m.compressed.size();
        s.next_out = data.data();
        s.avail_out = (uInt)data.size();
        int r = inflate(&s, Z_FINISH);
        inflateEnd(&s);
        if (r != Z_STREAM_END || s.total_out != m.size) { err = m.name + ": bad deflate data"; return false; }
    } else {
        err = m.name + ": compression method " + std::to_string(m.method) + " is not supported";
        return false;
    }
    if (crc32(0, data.data(), (uInt)data.size()) != m.crc) { err = m.name + ": CRC mismatch"; return false; }
    return true;
}

void ZipArchive::put(const std::string& name, const std::vector<uint8_t>& data) {
    Member m;
    m.name = name;
    m.method = 8;
    m.size = (uint32_t)data.size();
    m.crc = crc32(0, data.data(), (uInt)data.size());
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    m.time = (uint16_t)((tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2));
    m.date = (uint16_t)(((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday);
    uLongf bound = compressBound((uLong)data.size()) + 64;
    m.compressed.resize(bound);
    z_stream s = {};
    deflateInit2(&s, Z_BEST_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    s.next_in = const_cast<Bytef*>(data.data());
    s.avail_in = (uInt)data.size();
    s.next_out = m.compressed.data();
    s.avail_out = (uInt)m.compressed.size();
    deflate(&s, Z_FINISH);
    m.compressed.resize(s.total_out);
    deflateEnd(&s);
    m.compSize = (uint32_t)m.compressed.size();
    int i = find(name);
    if (i >= 0) {
        m.name = members_[i].name;
        m.extAttr = members_[i].extAttr;
        members_[i] = std::move(m);
    } else {
        members_.push_back(std::move(m));
    }
}

bool ZipArchive::save(const std::string& path, std::string& err) const {
    std::vector<uint8_t> out, cd;
    for (const auto& m : members_) {
        uint32_t local = (uint32_t)out.size();
        put32(out, 0x04034b50);
        put16(out, 20);           // version needed
        put16(out, 0);            // flags
        put16(out, m.method);
        put16(out, m.time);
        put16(out, m.date);
        put32(out, m.crc);
        put32(out, m.compSize);
        put32(out, m.size);
        put16(out, (uint16_t)m.name.size());
        put16(out, 0);
        out.insert(out.end(), m.name.begin(), m.name.end());
        out.insert(out.end(), m.compressed.begin(), m.compressed.end());

        put32(cd, 0x02014b50);
        put16(cd, 0x031E);        // made by: Unix, 3.0
        put16(cd, 20);
        put16(cd, 0);
        put16(cd, m.method);
        put16(cd, m.time);
        put16(cd, m.date);
        put32(cd, m.crc);
        put32(cd, m.compSize);
        put32(cd, m.size);
        put16(cd, (uint16_t)m.name.size());
        put16(cd, 0);
        put16(cd, 0);
        put16(cd, 0);
        put16(cd, 0);
        put32(cd, m.extAttr ? m.extAttr : 0x81A40000u);  // -rw-r--r--
        put32(cd, local);
        cd.insert(cd.end(), m.name.begin(), m.name.end());
    }
    uint32_t cdOff = (uint32_t)out.size();
    out.insert(out.end(), cd.begin(), cd.end());
    put32(out, 0x06054b50);
    put16(out, 0);
    put16(out, 0);
    put16(out, (uint16_t)members_.size());
    put16(out, (uint16_t)members_.size());
    put32(out, (uint32_t)cd.size());
    put32(out, cdOff);
    put16(out, 0);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) { err = "cannot write " + path; return false; }
    f.write((const char*)out.data(), out.size());
    if (!f) { err = "write error on " + path; return false; }
    return true;
}
