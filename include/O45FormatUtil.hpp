#pragma once
#include <string>
#include <cstdio>
#include "O45Types.hpp"

// Format function attributes for display (used by nm45 and objdump45)
inline std::string formatFuncAttr(const O45FuncAttr& fa) {
    std::string result = "[";

    // Convention and function flags
    if (fa.flags & FUNC_FLAG_ZP_CONV)  result += "zp_call ";
    else                                result += "stack_call ";
    if (fa.flags & FUNC_FLAG_LEAF)     result += "leaf ";
    if (fa.flags & FUNC_FLAG_REENTRANT) result += "reentrant ";
    if (fa.flags & FUNC_FLAG_STATIC_ALLOC) result += "static_alloc ";
    if (fa.flags & FUNC_FLAG_ZERO_ALLOC) result += "zeroalloc ";
    if (fa.flags & FUNC_FLAG_ISR)      result += "isr ";

    // the base-page addresses (#360)
    result += "uses:" + fa.zpUses.str();
    result += " clobbers:" + fa.zpClobbers.str();
    result += " releases:" + fa.zpRelease.str();

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
