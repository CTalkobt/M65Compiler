#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>

// Semantic intermediate representation for assembler code
// Format-agnostic representation that can be read from and written to different assemblers

namespace AsmIR {

enum class AddressingMode {
    IMPLIED,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    INDIRECT,
    INDIRECT_X,
    INDIRECT_Y,
    STACK_RELATIVE,
    FRAME_RELATIVE,
    INDEXED_PAIR  // 45GS02 specific: $addr, XY
};

enum class DirectiveType {
    BYTE_DATA,
    WORD_DATA,
    LONG_DATA,
    FILL,
    ALIGN,
    ORG,
    SEGMENT,
    GLOBAL,
    EXTERN,
    WEAK,
    CONST,
    CPU,
    FUNCTION,
    ZP_USES,
    ZP_CLOBBERS,
    REG_CLOBBERS,
    FLAG_CLOBBERS,
    LOC,  // source location
    OTHER
};

struct Operand {
    std::string text;
    int64_t value = 0;
    bool is_symbolic = false;
    bool is_expression = false;
};

struct Instruction {
    std::string mnemonic;
    AddressingMode mode = AddressingMode::IMPLIED;
    Operand operand;
    std::string label;

    Instruction() = default;
    Instruction(const std::string& mnemonic_, AddressingMode mode_ = AddressingMode::IMPLIED)
        : mnemonic(mnemonic_), mode(mode_) {}
};

struct Directive {
    DirectiveType type = DirectiveType::OTHER;
    std::string name;
    std::vector<std::string> arguments;
    std::map<std::string, std::string> attributes;

    Directive() = default;
    Directive(DirectiveType type_, const std::string& name_)
        : type(type_), name(name_) {}
};

struct Statement {
    enum Type {
        INSTRUCTION,
        DIRECTIVE,
        LABEL,
        COMMENT,
        BLANK
    } type = BLANK;

    Instruction instr;
    Directive dir;
    std::string label;
    std::string comment;

    int source_line = 0;
    std::string source_file;
};

struct Symbol {
    std::string name;
    uint32_t value = 0;
    bool is_global = false;
    bool is_extern = false;
    bool is_weak = false;
    bool is_constant = false;
    std::string segment;
};

struct Macro {
    std::string name;
    std::vector<std::string> parameters;  // parameter names
    std::vector<std::string> body;        // macro body lines (may contain \1, \2, etc. for params)
    int definitionLine = 0;
    std::string sourceFile;

    Macro() = default;
    Macro(const std::string& name_) : name(name_) {}
};

struct Module {
    std::vector<Statement> statements;
    std::map<std::string, Symbol> symbols;
    std::map<std::string, Macro> macros;    // macro name -> macro definition
    std::map<std::string, uint32_t> segments;  // segment name -> start address

    std::string cpu = "45GS02";
    bool is_o45 = false;  // indicates if .o45 format metadata was present
};

}  // namespace AsmIR
