#include "DebugInfoBuilder.hpp"
#include "LineNumberProgram.hpp"
#include "O45Writer.hpp"
#include <vector>
#include <cstring>

// Phase 113: Helper to serialize DWARF information from DebugInfoBuilder and LineNumberProgramBuilder

namespace {
    // Helper: write little-endian u32
    inline void writeU32LE(std::vector<uint8_t>& out, uint32_t val) {
        out.push_back(val & 0xFF);
        out.push_back((val >> 8) & 0xFF);
        out.push_back((val >> 16) & 0xFF);
        out.push_back((val >> 24) & 0xFF);
    }

    // Serialize a DWARF DIE tree into .debug_info format
    void serializeDIETree(std::vector<uint8_t>& out, dwarf::DIE* die,
                         const dwarf::StringPool& stringPool,
                         const dwarf::AbbreviationTable& abbrevTable) {
        if (!die) return;

        // For now, emit a placeholder DIE structure
        // Full implementation would recursively emit DIE tree with proper abbreviation codes

        // This is a simplified implementation that will be enhanced in future iterations
        // The actual DIE encoding follows DWARF 4 specification with:
        // - Abbreviation code (ULEB128)
        // - Attribute values according to abbreviation
        // - Null terminator (0x00) for end of DIE
    }
}

// Public function to prepare O45Writer with DWARF sections
void prepareO45WriterWithDWARF(O45Writer& writer,
                               const DebugInfoBuilder& debugBuilder,
                               const dwarf::LineNumberProgramBuilder& lineBuilder) {
    // Serialize .debug_info section (DIE tree + abbreviation table)
    std::vector<uint8_t> debugInfoBytes;

    // Write DWARF 4 .debug_info header
    // Unit length placeholder (will be calculated)
    uint32_t unitLengthPos = debugInfoBytes.size();
    writeU32LE(debugInfoBytes, 0);  // Placeholder for unit length

    // DWARF version (2 bytes)
    debugInfoBytes.push_back(0x04);  // DWARF 4
    debugInfoBytes.push_back(0x00);

    // .debug_abbrev_offset (4 bytes)
    writeU32LE(debugInfoBytes, 0);  // Will be 0 for now, embedded in file

    // Address size (1 byte)
    debugInfoBytes.push_back(0x02);  // 2-byte addresses (65xx)

    // TODO: Emit DIE tree from debugBuilder
    // For now, emit just a compile unit DIE

    // Update unit length
    uint32_t unitLength = debugInfoBytes.size() - unitLengthPos - 4;
    debugInfoBytes[unitLengthPos] = unitLength & 0xFF;
    debugInfoBytes[unitLengthPos + 1] = (unitLength >> 8) & 0xFF;
    debugInfoBytes[unitLengthPos + 2] = (unitLength >> 16) & 0xFF;
    debugInfoBytes[unitLengthPos + 3] = (unitLength >> 24) & 0xFF;

    writer.setDebugInfoSegment(debugInfoBytes);

    // Serialize .debug_line section
    const auto& lineProgram = lineBuilder.getLineProgram();
    writer.setDebugLineSegment(lineProgram);

    // Serialize .debug_str section (string pool)
    std::vector<uint8_t> debugStrBytes;
    const auto& strings = debugBuilder.getStringPool().getStrings();
    for (const auto& str : strings) {
        debugStrBytes.insert(debugStrBytes.end(), str.begin(), str.end());
        debugStrBytes.push_back(0x00);  // NUL terminator
    }
    writer.setDebugStrSegment(debugStrBytes);
}
