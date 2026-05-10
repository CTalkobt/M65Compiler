#pragma once
#include <string>
#include <cstdio>
#include "O45Types.hpp"

// Format function attributes for display (used by nm45 and objdump45)
inline std::string formatFuncAttr(const O45FuncAttr& fa, uint32_t zpStart) {
    std::string result = "[";

    // Convention and function flags
    if (fa.flags & FUNC_FLAG_ZP_CONV)  result += "zp_call ";
    else                                result += "stack_call ";
    if (fa.flags & FUNC_FLAG_LEAF)     result += "leaf ";
    if (fa.flags & FUNC_FLAG_REENTRANT) result += "reentrant ";

    // Format ZP mask as address range
    auto fmtZpMask = [&](uint32_t mask) -> std::string {
        if (mask == 0) return "-";
        std::string s;
        int first = -1, last = -1;
        for (int i = 0; i < 32; i++) {
            if (mask & (1u << i)) {
                if (first < 0) first = i;
                last = i;
            }
        }
        char buf[16];
        uint32_t base = zpStart + 1;
        if (first == last) {
            snprintf(buf, sizeof(buf), "%02X", (unsigned)(base + first));
        } else {
            snprintf(buf, sizeof(buf), "%02X-%02X", (unsigned)(base + first), (unsigned)(base + last));
        }
        return buf;
    };

    result += "uses:" + fmtZpMask(fa.zpUses);
    result += " clobbers:" + fmtZpMask(fa.zpClobbers);
    result += " releases:" + fmtZpMask(fa.zpRelease);

    // Registers
    std::string regs;
    if (fa.regClobbers & 0x01) regs += 'A';
    if (fa.regClobbers & 0x02) regs += 'X';
    if (fa.regClobbers & 0x04) regs += 'Y';
    if (fa.regClobbers & 0x08) regs += 'Z';
    result += " regs:" + (regs.empty() ? "-" : regs);

    // Flags
    std::string flags;
    if (fa.flagClobbers & 0x01) flags += 'C';
    if (fa.flagClobbers & 0x02) flags += 'N';
    if (fa.flagClobbers & 0x04) flags += 'Z';
    if (fa.flagClobbers & 0x08) flags += 'V';
    result += " flags:" + (flags.empty() ? "-" : flags);

    result += "]";
    return result;
}
