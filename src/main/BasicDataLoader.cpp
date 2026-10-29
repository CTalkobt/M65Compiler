#include "BasicDataLoader.hpp"
#include <cstdio>
#include <sstream>

namespace {
std::string hex2(uint8_t b) {
    char buf[3];
    std::snprintf(buf, sizeof buf, "%02x", b);
    return buf;
}
}  // namespace

DataLoaderResult makeDataLoader(const std::vector<uint8_t>& input, const DataLoaderOptions& o) {
    DataLoaderResult r;
    size_t skip = 0;
    uint32_t addr;
    if (o.isPrg) {
        if (input.size() < 2) { r.error = "the program has no load address (2 bytes)"; return r; }
        addr = input[0] | (input[1] << 8);
        skip = 2;
    } else {
        addr = 0;
    }
    if (o.loadAt >= 0) addr = (uint32_t)o.loadAt;
    else if (!o.isPrg) { r.error = "a raw binary needs --load-at ADDR"; return r; }
    std::vector<uint8_t> code(input.begin() + skip, input.end());
    if (code.empty()) { r.error = "there is no code to load"; return r; }
    if (addr + code.size() > 0x10000) { r.error = "the code does not fit below $10000"; return r; }
    if (o.perLine < 1 || o.perLine > 30) { r.error = "--per-line takes 1 to 30"; return r; }
    r.address = (uint16_t)addr;
    r.count = (uint32_t)code.size();

    // the lines: the loader, the REM, then the DATA lines. BANK 0 first:
    // BASIC 65's POKE and SYS use the BANK, and its default (128, the
    // system mapping) does not have RAM at most addresses
    std::ostringstream src;
    int line = o.start;
    auto next = [&]() { int l = line; line += o.increment; return l; };
    int loaderLines = o.checksum ? 5 : 2;
    int remLine = o.start + (loaderLines + (o.sys && !o.checksum ? 1 : 0)) * o.increment;
    int firstData = remLine + o.increment;
    std::string readV = o.hex ? "read v$:v=dec(v$)" : "read v";
    char buf[160];
    if (!o.checksum) {
        std::snprintf(buf, sizeof buf, "%d bank 0:a=%u:n=%u\n", next(), addr, r.count);
        src << buf;
        src << next() << " for i=0 to n-1:" << readV << ":poke a+i,v:next i\n";
        if (o.sys) src << next() << " sys a\n";
    } else {
        // each DATA line ends with the sum of its bytes AND 255; a mismatch
        // names the line and stops before the code runs
        std::snprintf(buf, sizeof buf, "%d bank 0:a=%u:n=%u:r=%d:l=%d:k=%d\n", next(), addr, r.count, o.perLine,
                      firstData, o.increment);
        src << buf;
        src << next() << " for i=0 to n-1 step r:s=0\n";
        src << next() << " for j=0 to r-1:if i+j<n then " << readV << ":poke a+i+j,v:s=s+v\n";
        src << next() << " next j:" << (o.hex ? "read c$:c=dec(c$)" : "read c")
            << ":if c<>(s and 255) then print \"data error in line\";l+k*int(i/r):end\n";
        src << next() << " next i" << (o.sys ? ":sys a" : "") << "\n";
    }
    std::snprintf(buf, sizeof buf, "%d rem %s - $%04x, %u bytes\n", next(), o.name.empty() ? "code" : o.name.c_str(),
                  addr, r.count);
    src << buf;
    for (size_t i = 0; i < code.size(); i += o.perLine) {
        src << next() << " data ";
        unsigned sum = 0;
        for (size_t k = i; k < code.size() && k < i + (size_t)o.perLine; k++) {
            if (k > i) src << ",";
            src << (o.hex ? hex2(code[k]) : std::to_string(code[k]));
            sum += code[k];
        }
        if (o.checksum) src << "," << (o.hex ? hex2((uint8_t)sum) : std::to_string(sum & 255));
        src << "\n";
        if (line > 63999 + o.increment) { r.error = "too many DATA lines for line numbers up to 63999"; return r; }
    }
    r.source = src.str();
    r.ok = true;
    return r;
}
