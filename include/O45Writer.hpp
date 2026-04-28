#pragma once
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include "O45Types.hpp"

// Writes a complete .o45 relocatable object file.
//
// Usage:
//   O45Writer w;
//   w.setTextSegment(0x2000, codeBytes);
//   w.setDataSegment(0x4000, dataBytes);
//   w.setBssSegment(0x6000, bssLen);
//   w.setZpSegment(0x02, zpLen);
//   w.addOption(OPT_ASM, "ca45 1.0");
//   auto blob = w.emit();

// A single relocation entry (high-level, before encoding).
// The encoder converts a sorted list of these into the .o65/.o45 byte stream.
struct O45Reloc {
    uint32_t offset;          // byte offset within the segment being patched
    O45RelocType type;        // R_WORD, R_LOW, R_HIGH, R_LINEAR24, R_LINEAR32, R_SEGADR
    O45Segment segment;       // target segment (SEG_TEXT, SEG_DATA, ... or SEG_EXTERNAL)
    uint32_t symbolIndex = 0; // import table index (only when segment == SEG_EXTERNAL)
    uint8_t extra = 0;        // extra byte for R_HIGH (low byte for underflow correction)
};

// Encodes a list of O45Reloc entries into the .o65/.o45 delta-offset byte stream.
// Entries MUST be sorted by ascending offset before calling.
// The returned bytes do NOT include the $00 terminator (O45Writer appends it).
class O45RelocEncoder {
public:
    static std::vector<uint8_t> encode(const std::vector<O45Reloc>& relocs);
};

struct O45SegmentInfo {
    uint32_t base = 0;
    uint32_t length = 0;
    std::vector<uint8_t> body;   // empty for BSS/ZP (no body emitted)
};

class O45Writer {
public:
    void setMode(uint16_t mode) { mode_ = mode; }
    void setCpuId(uint8_t id) { cpuId_ = id; }

    void setTextSegment(uint32_t base, const std::vector<uint8_t>& body);
    void setDataSegment(uint32_t base, const std::vector<uint8_t>& body);
    void setBssSegment(uint32_t base, uint32_t length);
    void setZpSegment(uint32_t base, uint32_t length);

    void setTextRelocations(const std::vector<uint8_t>& relocs);
    void setDataRelocations(const std::vector<uint8_t>& relocs);

    void addImport(const std::string& name);
    void addExport(const std::string& name, O45Segment seg, uint32_t offset);

    void addOption(uint8_t type, const std::string& value);

    // Emit the complete .o45 file as a byte vector.
    std::vector<uint8_t> emit() const;

    // Accessors for testing
    uint16_t getMode() const { return mode_; }
    uint8_t getCpuId() const { return cpuId_; }
    const O45SegmentInfo& getText() const { return text_; }
    const O45SegmentInfo& getData() const { return data_; }
    const O45SegmentInfo& getBss() const { return bss_; }
    const O45SegmentInfo& getZp() const { return zp_; }

private:
    uint16_t mode_ = O45_MODE_DEFAULT;
    uint8_t cpuId_ = O45_CPU_45GS02;

    O45SegmentInfo text_;
    O45SegmentInfo data_;
    O45SegmentInfo bss_;
    O45SegmentInfo zp_;

    std::vector<uint8_t> textRelocs_;
    std::vector<uint8_t> dataRelocs_;

    std::vector<std::string> imports_;

    struct ExportEntry {
        std::string name;
        O45Segment segment;
        uint32_t offset;
    };
    std::vector<ExportEntry> exports_;

    struct OptionEntry {
        uint8_t type;
        std::string value;
    };
    std::vector<OptionEntry> options_;

    // Write helpers
    void emitHeader(std::vector<uint8_t>& out) const;
    void emitOptions(std::vector<uint8_t>& out) const;
    void emitRelocTable(std::vector<uint8_t>& out, const std::vector<uint8_t>& relocs) const;
    void emitImports(std::vector<uint8_t>& out) const;
    void emitExports(std::vector<uint8_t>& out) const;

    static void writeU16(std::vector<uint8_t>& out, uint16_t val);
    static void writeU32(std::vector<uint8_t>& out, uint32_t val);
    static void writeString(std::vector<uint8_t>& out, const std::string& str);
};

// =============================================================================
// O45SymbolTable — manages imports (.extern) and exports (.global) for an
// object file being assembled. Assigns stable zero-based indices to imports
// (used by relocation entries) and tracks exports with their segment + offset.
//
// Usage:
//   O45SymbolTable syms;
//   uint32_t idx = syms.addImport("_printf");   // returns 0
//   uint32_t idx2 = syms.addImport("_puts");    // returns 1
//   uint32_t idx3 = syms.addImport("_printf");  // returns 0 (dedup)
//   syms.addExport("_main", SEG_TEXT, 0x0010);
//   syms.applyTo(writer);  // populates O45Writer's import/export tables
// =============================================================================

struct O45Export {
    std::string name;
    O45Segment segment;
    uint32_t offset;
};

class O45SymbolTable {
public:
    // Add an import (external reference). Returns its zero-based index.
    // Duplicate names return the existing index.
    uint32_t addImport(const std::string& name);

    // Add an export (global definition). Returns true on success,
    // false if the name was already exported (duplicate).
    bool addExport(const std::string& name, O45Segment segment, uint32_t offset);

    // Look up an import index by name. Returns (uint32_t)-1 if not found.
    uint32_t getImportIndex(const std::string& name) const;

    // Check if a symbol is imported.
    bool isImported(const std::string& name) const;

    // Check if a symbol is exported.
    bool isExported(const std::string& name) const;

    // Apply all imports and exports to an O45Writer.
    void applyTo(O45Writer& writer) const;

    // Accessors
    const std::vector<std::string>& getImports() const { return imports_; }
    const std::vector<O45Export>& getExports() const { return exports_; }
    uint32_t importCount() const { return (uint32_t)imports_.size(); }
    uint32_t exportCount() const { return (uint32_t)exports_.size(); }

    // Validation: returns list of error messages (empty = ok).
    // Checks for imports that are also exported (self-referencing object).
    std::vector<std::string> validate() const;

private:
    std::vector<std::string> imports_;
    std::map<std::string, uint32_t> importIndex_; // name -> index
    std::vector<O45Export> exports_;
    std::map<std::string, uint32_t> exportIndex_; // name -> index in exports_
};
