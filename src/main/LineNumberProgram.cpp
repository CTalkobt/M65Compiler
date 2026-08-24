#include "LineNumberProgram.hpp"
#include <algorithm>

namespace dwarf {

LineNumberProgramBuilder::LineNumberProgramBuilder() {
    currentState_.reset();
    previousState_.reset();
    currentState_.is_stmt = header_.default_is_stmt;
    previousState_.is_stmt = header_.default_is_stmt;
}

uint32_t LineNumberProgramBuilder::addFile(const std::string& filename,
                                           const std::string& directory,
                                           uint32_t mod_time,
                                           uint32_t file_size) {
    auto it = fileMap_.find(filename);
    if (it != fileMap_.end()) {
        return it->second;
    }

    uint32_t index = nextFileIndex_++;
    fileMap_[filename] = index;

    FileEntry entry;
    entry.filename = filename;
    entry.directory = directory;
    entry.mod_time = mod_time;
    entry.file_size = file_size;
    files_.push_back(entry);

    return index;
}

void LineNumberProgramBuilder::emitByte(uint8_t value) {
    lineProgram_.push_back(value);
}

void LineNumberProgramBuilder::emitWord(uint16_t value) {
    lineProgram_.push_back(value & 0xFF);
    lineProgram_.push_back((value >> 8) & 0xFF);
}

void LineNumberProgramBuilder::emitDWord(uint32_t value) {
    emitWord(value & 0xFFFF);
    emitWord((value >> 16) & 0xFFFF);
}

void LineNumberProgramBuilder::emitQWord(uint64_t value) {
    emitDWord(value & 0xFFFFFFFF);
    emitDWord((value >> 32) & 0xFFFFFFFF);
}

void LineNumberProgramBuilder::emitULEB128(uint64_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        emitByte(byte);
    } while (value != 0);
}

void LineNumberProgramBuilder::emitSLEB128(int64_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        more = !((value == 0 && (byte & 0x40) == 0) ||
                 (value == -1 && (byte & 0x40) != 0));
        if (more) {
            byte |= 0x80;
        }
        emitByte(byte);
    }
}

void LineNumberProgramBuilder::emitExtendedOp(LineNumberExtOp op,
                                              const std::vector<uint8_t>& data) {
    emitByte(static_cast<uint8_t>(LineNumberOp::DW_LNS_EXTENDED_OP));
    emitULEB128(data.size() + 1);  // +1 for opcode byte
    emitByte(static_cast<uint8_t>(op));
    for (uint8_t byte : data) {
        emitByte(byte);
    }
}

void LineNumberProgramBuilder::emitOp(LineNumberOp op, uint64_t operand) {
    emitByte(static_cast<uint8_t>(op));
    if (operand > 0) {
        emitULEB128(operand);
    }
}

void LineNumberProgramBuilder::emitSpecialOpcode(const LineState& prev_state,
                                                 const LineState& new_state) {
    // Calculate deltas
    int32_t line_delta = static_cast<int32_t>(new_state.line) - static_cast<int32_t>(prev_state.line);
    uint64_t pc_delta = new_state.address - prev_state.address;

    // Try to encode as special opcode
    // Special opcodes combine address and line deltas
    int adjusted_line_delta = line_delta - header_.line_base;

    if (adjusted_line_delta >= 0 && adjusted_line_delta < header_.line_range &&
        pc_delta < 256) {
        // Can use special opcode
        uint8_t opcode = static_cast<uint8_t>(adjusted_line_delta * header_.opcode_base + pc_delta + header_.opcode_base);
        emitByte(opcode);
    } else {
        // Need multiple opcodes
        if (line_delta != 0) {
            emitOp(LineNumberOp::DW_LNS_ADVANCE_LINE, line_delta);
        }
        if (pc_delta != 0) {
            emitOp(LineNumberOp::DW_LNS_ADVANCE_PC, pc_delta);
        }
        emitOp(LineNumberOp::DW_LNS_COPY);
    }
}

void LineNumberProgramBuilder::emitLineEntry(uint64_t address, uint32_t line, uint32_t column) {
    currentState_.address = address;
    currentState_.line = line;
    currentState_.column = column;

    // Emit necessary state changes
    if (currentState_.address != previousState_.address ||
        currentState_.line != previousState_.line ||
        currentState_.column != previousState_.column) {

        // Set address if changed
        if (currentState_.address != previousState_.address) {
            std::vector<uint8_t> addr_data;
            for (int i = 0; i < 8; i++) {
                addr_data.push_back((currentState_.address >> (i * 8)) & 0xFF);
            }
            emitExtendedOp(LineNumberExtOp::DW_LNE_SET_ADDRESS, addr_data);
        }

        // Set file if changed
        if (currentState_.file != previousState_.file) {
            emitOp(LineNumberOp::DW_LNS_SET_FILE, currentState_.file);
        }

        // Set column if changed
        if (currentState_.column != previousState_.column) {
            emitOp(LineNumberOp::DW_LNS_SET_COLUMN, currentState_.column);
        }

        // Use special opcode for line/address combo or explicit copy
        if (currentState_.line != previousState_.line) {
            emitOp(LineNumberOp::DW_LNS_ADVANCE_LINE, currentState_.line - previousState_.line);
        }

        emitOp(LineNumberOp::DW_LNS_COPY);

        previousState_ = currentState_;
    }
}

void LineNumberProgramBuilder::emitLineEntry(uint64_t address,
                                             const SourceLocationTracker::SourceLocation& loc) {
    if (!loc.isValid()) {
        return;
    }

    // Find or add file
    uint32_t file_idx = addFile(loc.filename, "");  // Empty directory for now
    currentState_.file = file_idx;

    emitLineEntry(address, loc.line, loc.column);
}

void LineNumberProgramBuilder::emitAddressRange(uint64_t start, uint64_t end, uint32_t line) {
    // Emit start address with line
    emitLineEntry(start, line, 0);

    // Emit end address (marks range end)
    currentState_.address = end;
    emitExtendedOp(LineNumberExtOp::DW_LNE_SET_ADDRESS,
        std::vector<uint8_t>{
            static_cast<uint8_t>(end & 0xFF),
            static_cast<uint8_t>((end >> 8) & 0xFF),
            static_cast<uint8_t>((end >> 16) & 0xFF),
            static_cast<uint8_t>((end >> 24) & 0xFF),
            static_cast<uint8_t>((end >> 32) & 0xFF),
            static_cast<uint8_t>((end >> 40) & 0xFF),
            static_cast<uint8_t>((end >> 48) & 0xFF),
            static_cast<uint8_t>((end >> 56) & 0xFF),
        });
    emitOp(LineNumberOp::DW_LNS_COPY);
}

void LineNumberProgramBuilder::finalizeProgram() {
    // Emit end sequence
    emitExtendedOp(LineNumberExtOp::DW_LNE_END_SEQUENCE, std::vector<uint8_t>());

    // Update unit length in header
    header_.unit_length = static_cast<uint32_t>(lineProgram_.size()) + 4;  // +4 for length field itself
}

}  // namespace dwarf
