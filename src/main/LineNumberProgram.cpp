#include "LineNumberProgram.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>

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
    // FIX #5: File index overflow check
    // Use (filename, directory) pair as key to avoid collision (FIX #3.3)
    auto key = std::make_pair(filename, directory);
    auto it = fileMap_.find(key);
    if (it != fileMap_.end()) {
        return it->second;
    }

    // FIX #2.3: Check for file index overflow before incrementing
    static constexpr uint32_t MAX_FILES = 0xFFFFFF;  // Reserve headroom below uint32_t max
    if (nextFileIndex_ >= MAX_FILES) {
        throw std::runtime_error(
            "File table overflow: too many files (max " + std::to_string(MAX_FILES) + ")"
        );
    }

    uint32_t index = nextFileIndex_++;
    fileMap_[key] = index;

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
    // DWARF 4 special opcodes: valid range [13, 255] (243 valid opcodes)
    // Special opcode calculation: (line_delta - line_base) * opcode_base + (pc_delta % opcode_base) + opcode_base
    // For 6502: line_base=-5, line_range=14, opcode_base=13

    int adjusted_line_delta = line_delta - header_.line_base;

    if (adjusted_line_delta >= 0 && adjusted_line_delta < header_.line_range && pc_delta < 256) {
        // FIX #1.1: Calculate special opcode value and validate it's in valid range [13, 255]
        uint32_t opcode_value = (adjusted_line_delta * header_.opcode_base) +
                                (pc_delta % header_.opcode_base) +
                                header_.opcode_base;

        if (opcode_value >= 13 && opcode_value <= 255) {
            emitByte(static_cast<uint8_t>(opcode_value));
        } else {
            // Fallback: use standard opcodes instead when special opcode calculation overflows
            if (line_delta != 0) {
                emitOp(LineNumberOp::DW_LNS_ADVANCE_LINE, static_cast<uint64_t>(line_delta));
            }
            if (pc_delta != 0) {
                emitOp(LineNumberOp::DW_LNS_ADVANCE_PC, pc_delta);
            }
            emitOp(LineNumberOp::DW_LNS_COPY);
        }
    } else {
        // Use standard opcodes for this entry
        if (line_delta != 0) {
            emitOp(LineNumberOp::DW_LNS_ADVANCE_LINE, static_cast<uint64_t>(line_delta));
        }
        if (pc_delta != 0) {
            emitOp(LineNumberOp::DW_LNS_ADVANCE_PC, pc_delta);
        }
        emitOp(LineNumberOp::DW_LNS_COPY);
    }
}

void LineNumberProgramBuilder::emitLineEntry(uint64_t address, uint32_t line, uint32_t column) {
    // FIX #2.1: Validate line number bounds
    // Source line numbers typically < 1 million; clamp excessive values
    const uint32_t MAX_LINE_NUMBER = 1000000;
    if (line > MAX_LINE_NUMBER) {
        std::cerr << "Warning: Line number " << line << " exceeds 1M, clamping to range\n";
        line = line % 100000;  // Fold to reasonable range
    }

    // FIX #2.1: Validate column bounds
    const uint32_t MAX_COLUMN = 10000;
    if (column > MAX_COLUMN) {
        std::cerr << "Warning: Column number " << column << " exceeds 10K, resetting to 0\n";
        column = 0;
    }

    // FIX #2.2: Validate address range and detect backward jumps
    const uint64_t MEGA65_MAX_ADDR = 0x100000;  // 1MB extended memory limit
    if (address >= MEGA65_MAX_ADDR) {
        std::cerr << "Warning: Line entry address $" << std::hex << address
                  << " exceeds MEGA65 address space (>1MB), skipping entry\n" << std::dec;
        return;
    }

    // FIX #2.2: Detect backward jumps (indicate potential problem in compilation)
    if (address < currentState_.address && currentState_.address != 0) {
        // Emit end-of-sequence and reset for new sequence
        std::cerr << "Warning: Backward jump detected (from $" << std::hex << currentState_.address
                  << " to $" << address << "), emitting end-of-sequence\n" << std::dec;
        emitExtendedOp(LineNumberExtOp::DW_LNE_END_SEQUENCE, {});
        currentState_.reset();
        currentState_.is_stmt = header_.default_is_stmt;
        previousState_.reset();
        previousState_.is_stmt = header_.default_is_stmt;
    }

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
    // FIX #3.4: Add double-finalization guard
    if (finalized_) {
        throw std::runtime_error("Line program already finalized");
    }

    // Emit end sequence
    emitExtendedOp(LineNumberExtOp::DW_LNE_END_SEQUENCE, std::vector<uint8_t>());

    // Update unit length in header
    header_.unit_length = static_cast<uint32_t>(lineProgram_.size()) + 4;  // +4 for length field itself

    finalized_ = true;
}

}  // namespace dwarf
