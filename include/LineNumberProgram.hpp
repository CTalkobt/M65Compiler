#pragma once

#include "DebugInfo.hpp"
#include "SourceLocationTracker.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <map>

// DWARF Line Number Program standard opcodes
namespace dwarf {

// Line number program header constants
struct LineNumberProgramHeader {
    uint32_t unit_length = 0;      // Length of .debug_line contribution
    uint16_t version = 4;           // DWARF version
    uint32_t header_length = 0;     // Offset of first LNP instruction
    uint8_t min_instruction_length = 1;
    uint8_t maximum_operations_per_instruction = 1;
    uint8_t default_is_stmt = 1;    // Default is_stmt value
    int8_t line_base = -5;          // Line range base
    uint8_t line_range = 14;        // Line range
    uint8_t opcode_base = 13;       // First user-defined opcode
};

// Standard line number program opcodes (DWARF 4)
enum class LineNumberOp : uint8_t {
    DW_LNS_EXTENDED_OP = 0x00,      // Extended opcode follows
    DW_LNS_COPY = 0x01,              // Emit current line entry
    DW_LNS_ADVANCE_PC = 0x02,        // Advance program counter
    DW_LNS_ADVANCE_LINE = 0x03,      // Advance current line
    DW_LNS_SET_FILE = 0x04,          // Set current file
    DW_LNS_SET_COLUMN = 0x05,        // Set current column
    DW_LNS_NEGATE_STMT = 0x06,       // Negate is_stmt
    DW_LNS_SET_BASIC_BLOCK = 0x07,   // Mark basic block
    DW_LNS_CONST_ADD_PC = 0x08,      // Add to PC (special opcode substitute)
    DW_LNS_FIXED_ADVANCE_PC = 0x09,  // Advance PC by fixed amount
    DW_LNS_SET_PROLOGUE_END = 0x0A,  // Mark prologue end
    DW_LNS_SET_EPILOGUE_BEGIN = 0x0B, // Mark epilogue begin
    DW_LNS_SET_ISA = 0x0C,           // Set instruction set architecture
};

// Extended line number program opcodes
enum class LineNumberExtOp : uint8_t {
    DW_LNE_END_SEQUENCE = 0x01,      // End of sequence
    DW_LNE_SET_ADDRESS = 0x02,       // Set absolute address
    DW_LNE_DEFINE_FILE = 0x03,       // Define new file
    DW_LNE_SET_DISCRIMINATOR = 0x04, // Set discriminator
};

// Note: LineState is defined in DebugInfo.hpp and used directly

// Line number program builder
class LineNumberProgramBuilder {
public:
    LineNumberProgramBuilder();

    // Add a file entry to the line table
    uint32_t addFile(const std::string& filename,
                    const std::string& directory,
                    uint32_t mod_time = 0,
                    uint32_t file_size = 0);

    // Emit line entries for source locations
    void emitLineEntry(uint64_t address, uint32_t line, uint32_t column = 0);
    void emitLineEntry(uint64_t address, const SourceLocationTracker::SourceLocation& loc);

    // Emit address range
    void emitAddressRange(uint64_t start, uint64_t end, uint32_t line);

    // Get line number program bytes
    const std::vector<uint8_t>& getLineProgram() const { return lineProgram_; }

    // Get file entries
    const std::vector<FileEntry>& getFileEntries() const { return files_; }

    // Get line number program header
    const LineNumberProgramHeader& getHeader() const { return header_; }

    // Finalize and emit end sequence
    void finalizeProgram();

private:
    // Helper methods for emitting opcodes
    void emitByte(uint8_t value);
    void emitWord(uint16_t value);
    void emitDWord(uint32_t value);
    void emitQWord(uint64_t value);
    void emitULEB128(uint64_t value);
    void emitSLEB128(int64_t value);

    // Emit extended opcode
    void emitExtendedOp(LineNumberExtOp op, const std::vector<uint8_t>& data);

    // Emit opcode with operand
    void emitOp(LineNumberOp op, uint64_t operand = 0);

    // Emit special opcode
    void emitSpecialOpcode(const LineState& prev_state, const LineState& new_state);

    LineNumberProgramHeader header_;
    std::vector<uint8_t> lineProgram_;
    std::vector<FileEntry> files_;
    std::map<std::string, uint32_t> fileMap_;  // filename -> index
    LineState currentState_;
    LineState previousState_;
    uint32_t nextFileIndex_ = 1;  // 1-based file index
};

}  // namespace dwarf
