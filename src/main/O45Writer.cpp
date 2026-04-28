#include "O45Writer.hpp"

// =============================================================================
// O45RelocEncoder — converts high-level relocation entries into .o65/.o45
// delta-offset byte stream.
//
// Encoding rules (from .o65 spec):
//   - Offsets are encoded as deltas from the previous relocation site
//     (first delta is from segment base, i.e. offset 0).
//   - Delta byte $01-$FE: add that value to the running offset.
//   - Delta byte $FF: add 254 to running offset, then read next delta byte.
//     Multiple $FF bytes chain for large gaps.
//   - Delta byte $00: end of table (not emitted here — O45Writer appends it).
//   - After the delta: one type/seg byte (type in bits 7-5, segment in bits 4-0).
//   - If type == R_HIGH: one extra byte follows (low byte for underflow).
//   - If segment == SEG_EXTERNAL: 4-byte LE symbol index follows (SIZE32 mode).
// =============================================================================

std::vector<uint8_t> O45RelocEncoder::encode(const std::vector<O45Reloc>& relocs) {
    std::vector<uint8_t> out;
    uint32_t pos = 0; // running offset within the segment

    for (const auto& r : relocs) {
        // Encode the delta from current position to this relocation site.
        // Delta must be >= 1 (offset $00 means end-of-table).
        uint32_t delta = r.offset - pos;

        // Emit $FF escape bytes for every 254-byte skip
        while (delta > 254) {
            out.push_back(O45_RELOC_ESCAPE); // $FF = skip 254
            delta -= O45_RELOC_SKIP;
        }

        // The remaining delta must be 1..254.
        // A delta of 0 after escapes means the entry sits exactly at a
        // 254-boundary — emit 1 and adjust (the spec requires delta >= 1).
        // However, this only occurs if the original offset == pos (first entry
        // at offset 0 or two entries at the same offset), which the .o65 spec
        // doesn't support. We handle it defensively with an extra escape.
        if (delta == 0) {
            // Can't emit $00 (that's end-of-table). This shouldn't happen with
            // well-formed input, but handle it: the first reloc should be at
            // offset >= 1 in standard .o65. For offset 0, we use delta = 0
            // which is invalid — caller must ensure offset >= 1.
            // Skip this entry (or the caller should have adjusted).
            continue;
        }

        out.push_back((uint8_t)delta);

        // Type/segment byte
        uint8_t typeSeg = (uint8_t)r.type | (uint8_t)r.segment;
        out.push_back(typeSeg);

        // R_HIGH extra byte (low byte for borrow/underflow correction)
        if (r.type == R_HIGH) {
            out.push_back(r.extra);
        }

        // External symbol index (4 bytes LE in SIZE32 mode)
        if (r.segment == SEG_EXTERNAL) {
            out.push_back((uint8_t)(r.symbolIndex & 0xFF));
            out.push_back((uint8_t)((r.symbolIndex >> 8) & 0xFF));
            out.push_back((uint8_t)((r.symbolIndex >> 16) & 0xFF));
            out.push_back((uint8_t)((r.symbolIndex >> 24) & 0xFF));
        }

        // Advance running position to this relocation site
        pos = r.offset;
    }

    return out;
}

// --- Segment setters ---

void O45Writer::setTextSegment(uint32_t base, const std::vector<uint8_t>& body) {
    text_.base = base;
    text_.length = (uint32_t)body.size();
    text_.body = body;
}

void O45Writer::setDataSegment(uint32_t base, const std::vector<uint8_t>& body) {
    data_.base = base;
    data_.length = (uint32_t)body.size();
    data_.body = body;
}

void O45Writer::setBssSegment(uint32_t base, uint32_t length) {
    bss_.base = base;
    bss_.length = length;
}

void O45Writer::setZpSegment(uint32_t base, uint32_t length) {
    zp_.base = base;
    zp_.length = length;
}

// --- Relocation setters ---

void O45Writer::setTextRelocations(const std::vector<uint8_t>& relocs) {
    textRelocs_ = relocs;
}

void O45Writer::setDataRelocations(const std::vector<uint8_t>& relocs) {
    dataRelocs_ = relocs;
}

// --- Symbol table ---

void O45Writer::addImport(const std::string& name) {
    imports_.push_back(name);
}

void O45Writer::addExport(const std::string& name, O45Segment seg, uint32_t offset, bool weak) {
    exports_.push_back({name, seg, offset, weak});
}

// --- Options ---

void O45Writer::addOption(uint8_t type, const std::string& value) {
    std::vector<uint8_t> data;
    for (char c : value) data.push_back((uint8_t)c);
    data.push_back(0x00); // NUL terminator for string options
    options_.push_back({type, data});
}

void O45Writer::addOptionRaw(uint8_t type, const std::vector<uint8_t>& data) {
    options_.push_back({type, data});
}

void O45Writer::addDefaultOptions(const std::string& asmVersion) {
    addOptionRaw(OPT_OS, {OPT_OS_MEGA65});
    addOption(OPT_ASM, asmVersion);
}

// --- Emit complete .o45 file ---

std::vector<uint8_t> O45Writer::emit() const {
    std::vector<uint8_t> out;
    out.reserve(O45_HEADER_SIZE + text_.length + data_.length + 256);

    emitHeader(out);
    emitOptions(out);

    // Text segment body
    out.insert(out.end(), text_.body.begin(), text_.body.end());
    // Text relocation table (terminated by $00)
    emitRelocTable(out, textRelocs_);

    // Data segment body
    out.insert(out.end(), data_.body.begin(), data_.body.end());
    // Data relocation table (terminated by $00)
    emitRelocTable(out, dataRelocs_);

    // Symbol tables
    emitImports(out);
    emitExports(out);

    return out;
}

// --- Header: 41 bytes ---
//
// Offset  Field       Size
// 0-1     marker      2     $01 $00
// 2-4     magic       3     "o65"
// 5       version     1     $00
// 6-7     mode        2     LE
// 8-11    tbase       4
// 12-15   tlen        4
// 16-19   dbase       4
// 20-23   dlen        4
// 24-27   bbase       4
// 28-31   blen        4
// 32-35   zbase       4
// 36-39   zlen        4
// 40      cpu_id      1     $45

void O45Writer::emitHeader(std::vector<uint8_t>& out) const {
    // Marker (2 bytes)
    out.push_back(O45_MARKER1);
    out.push_back(O45_MARKER2);

    // Magic "o65" (3 bytes)
    out.push_back(O45_MAGIC[0]);
    out.push_back(O45_MAGIC[1]);
    out.push_back(O45_MAGIC[2]);

    // Version (1 byte)
    out.push_back(O45_VERSION);

    // Mode word (2 bytes LE)
    writeU16(out, mode_);

    // Text segment: base + length (4+4)
    writeU32(out, text_.base);
    writeU32(out, text_.length);

    // Data segment: base + length (4+4)
    writeU32(out, data_.base);
    writeU32(out, data_.length);

    // BSS segment: base + length (4+4)
    writeU32(out, bss_.base);
    writeU32(out, bss_.length);

    // ZP segment: base + length (4+4)
    writeU32(out, zp_.base);
    writeU32(out, zp_.length);

    // Extended CPU ID byte (1 byte)
    out.push_back(cpuId_);
}

// --- Option headers ---
// Each: length (1) + type (1) + data (N), terminated by $00

void O45Writer::emitOptions(std::vector<uint8_t>& out) const {
    for (const auto& opt : options_) {
        uint8_t len = (uint8_t)(2 + opt.data.size()); // length byte + type byte + payload
        out.push_back(len);
        out.push_back(opt.type);
        out.insert(out.end(), opt.data.begin(), opt.data.end());
    }
    out.push_back(OPT_END); // end of options
}

// --- Relocation table ---
// Raw pre-encoded bytes, plus the $00 terminator

void O45Writer::emitRelocTable(std::vector<uint8_t>& out, const std::vector<uint8_t>& relocs) const {
    out.insert(out.end(), relocs.begin(), relocs.end());
    out.push_back(O45_RELOC_END);
}

// --- Import table ---
// 4-byte count (LE) + NUL-terminated names

void O45Writer::emitImports(std::vector<uint8_t>& out) const {
    writeU32(out, (uint32_t)imports_.size());
    for (const auto& name : imports_) {
        writeString(out, name);
    }
}

// --- Export table ---
// 4-byte count (LE) + entries: name (NUL-term) + segment (1) + offset (4 LE)

void O45Writer::emitExports(std::vector<uint8_t>& out) const {
    writeU32(out, (uint32_t)exports_.size());
    for (const auto& exp : exports_) {
        writeString(out, exp.name);
        uint8_t segByte = (uint8_t)exp.segment;
        if (exp.weak) segByte |= O45_EXPORT_WEAK;
        out.push_back(segByte);
        writeU32(out, exp.offset);
    }
}

// --- Write helpers ---

void O45Writer::writeU16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)(val >> 8));
}

void O45Writer::writeU32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)((val >> 8) & 0xFF));
    out.push_back((uint8_t)((val >> 16) & 0xFF));
    out.push_back((uint8_t)((val >> 24) & 0xFF));
}

void O45Writer::writeString(std::vector<uint8_t>& out, const std::string& str) {
    for (char c : str) out.push_back((uint8_t)c);
    out.push_back(0x00);
}

// =============================================================================
// O45SymbolTable
// =============================================================================

uint32_t O45SymbolTable::addImport(const std::string& name) {
    auto it = importIndex_.find(name);
    if (it != importIndex_.end()) return it->second;
    uint32_t idx = (uint32_t)imports_.size();
    imports_.push_back(name);
    importIndex_[name] = idx;
    return idx;
}

bool O45SymbolTable::addExport(const std::string& name, O45Segment segment, uint32_t offset, bool weak) {
    if (exportIndex_.count(name)) return false;
    uint32_t idx = (uint32_t)exports_.size();
    exports_.push_back({name, segment, offset, weak});
    exportIndex_[name] = idx;
    return true;
}

uint32_t O45SymbolTable::getImportIndex(const std::string& name) const {
    auto it = importIndex_.find(name);
    if (it != importIndex_.end()) return it->second;
    return (uint32_t)-1;
}

bool O45SymbolTable::isImported(const std::string& name) const {
    return importIndex_.count(name) > 0;
}

bool O45SymbolTable::isExported(const std::string& name) const {
    return exportIndex_.count(name) > 0;
}

void O45SymbolTable::applyTo(O45Writer& writer) const {
    for (const auto& name : imports_) {
        writer.addImport(name);
    }
    for (const auto& exp : exports_) {
        writer.addExport(exp.name, exp.segment, exp.offset, exp.weak);
    }
}

std::vector<std::string> O45SymbolTable::validate() const {
    std::vector<std::string> errors;
    for (const auto& name : imports_) {
        if (exportIndex_.count(name)) {
            errors.push_back("symbol '" + name + "' is both imported and exported");
        }
    }
    return errors;
}
