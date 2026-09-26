#include "O45Writer.hpp"
#include <cstring>

// Phase 113: DWARF debug section serialization for O45Writer

namespace {
    // Helper: write a little-endian u32 to output
    inline void writeU32LE(std::vector<uint8_t>& out, uint32_t val) {
        out.push_back(val & 0xFF);
        out.push_back((val >> 8) & 0xFF);
        out.push_back((val >> 16) & 0xFF);
        out.push_back((val >> 24) & 0xFF);
    }

    // Helper: write a little-endian u16 to output
    inline void writeU16LE(std::vector<uint8_t>& out, uint16_t val) {
        out.push_back(val & 0xFF);
        out.push_back((val >> 8) & 0xFF);
    }

    // Helper: write a byte to output
    inline void writeU8(std::vector<uint8_t>& out, uint8_t val) {
        out.push_back(val);
    }
}

void O45Writer::emitDebugSections(std::vector<uint8_t>& out) const {
    // Write debug sections in order: .debug_info, .debug_line, .debug_str
    // Each section is prefixed with a section header:
    // - 4 bytes: section size (including this 4-byte header)
    // - section data

    if (hasDebugInfo_) {
        uint32_t sectionSize = 4 + debugInfo_.size();
        writeU32LE(out, sectionSize);
        out.insert(out.end(), debugInfo_.begin(), debugInfo_.end());
    }

    if (hasDebugLine_) {
        uint32_t sectionSize = 4 + debugLine_.size();
        writeU32LE(out, sectionSize);
        out.insert(out.end(), debugLine_.begin(), debugLine_.end());
    }

    if (hasDebugStr_) {
        uint32_t sectionSize = 4 + debugStr_.size();
        writeU32LE(out, sectionSize);
        out.insert(out.end(), debugStr_.begin(), debugStr_.end());
    }
}
