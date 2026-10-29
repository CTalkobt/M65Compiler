#pragma once
#include "AsmIR.hpp"
#include <string>
#include <memory>

// Abstract base class for assembler format writers
class AsmWriter {
public:
    virtual ~AsmWriter() = default;

    virtual std::string write(const AsmIR::Module& module) = 0;

    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }
    bool hasErrors() const { return !errors_.empty(); }

protected:
    // ca45's strings (.text PETSCII, .screencode, .ascii) as .byte lists of
    // the bytes ca45 makes of them, the string kept in the comment, and its
    // character constants ('A' is PETSCII $C1) as numbers: the other
    // assemblers encode both differently
    static AsmIR::Module textAsBytes(const AsmIR::Module& module);
    // A data directive's kind, whatever ca45 spelling it has: "dword" (.dword,
    // .long), "fill" (.res, .fill, .space, .ds), "align" (.align, .balign),
    // "byte" (.byte, .db), "word" (.word, .dw); any other its own name
    static std::string dataKind(const std::string& name);
    // ca45's @local labels as globals, <enclosing label>__<name> (in their
    // definitions and in operands and arguments), for the dialects whose own
    // local labels scope differently or are not known here
    static AsmIR::Module localsAsGlobals(const AsmIR::Module& module);
    // the 45GS02's 32-bit instructions (ldq, adcq, ...: the Q register)
    static bool isQInstruction(const std::string& mnemonic);
    // a 45GS02 flat-memory operand, [zp],z: its zp part (else "")
    static std::string flatOperand(const std::string& operand);
    // a .dword / .fill whose arguments are numbers, as the bytes ca45 makes
    // of it ("$78, $56, $34, $12"); "" when they are not numbers (or a fill
    // over 256 bytes)
    static std::string literalBytes(const AsmIR::Directive& dir);
    // a .cpu argument's family, whatever the dialect called it: "45gs02"
    // (45GS02, m65, _45gs02, 4510, 65CE02), "6502" (6502, _6502), "65c02",
    // else "" (#224)
    static std::string cpuFamily(const std::string& arg);
    // a directive the target has no equivalent of: written as given, warned
    std::string noEquivalent(const AsmIR::Directive& dir, const std::string& target, const std::string& written) {
        addWarning("." + dir.name + " has no " + target + " equivalent here: written as '" + written + "'");
        return written;
    }

    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    void addError(const std::string& msg) {
        errors_.push_back(msg);
    }

    void addWarning(const std::string& msg) {
        warnings_.push_back(msg);
    }
};

// Factory function signature
using WriterFactory = AsmWriter*(*)();

// Registry for available writers
class WriterRegistry {
public:
    static WriterRegistry& getInstance();

    void registerWriter(const std::string& format, WriterFactory factory);
    std::unique_ptr<AsmWriter> createWriter(const std::string& format);
    std::vector<std::string> getAvailableWriters() const;

private:
    std::map<std::string, WriterFactory> writers_;

    WriterRegistry() = default;
    WriterRegistry(const WriterRegistry&) = delete;
    WriterRegistry& operator=(const WriterRegistry&) = delete;
};
