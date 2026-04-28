#include "O45Emitter.hpp"
#include "O45Writer.hpp"
#include "O45Types.hpp"
#include "AssemblerParser.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include <algorithm>

// Map ca45 segment names to O45 segment IDs.
static O45Segment segIdFromName(const std::string& name) {
    if (name == "code" || name == "text") return SEG_TEXT;
    if (name == "data") return SEG_DATA;
    if (name == "bss") return SEG_BSS;
    if (name == "zp") return SEG_ZP;
    return SEG_TEXT; // default segment → text
}

// Determine if an addressing mode carries a 16-bit address operand.
static bool isAbsoluteMode(AddressingMode m) {
    switch (m) {
        case AddressingMode::ABSOLUTE:
        case AddressingMode::ABSOLUTE_X:
        case AddressingMode::ABSOLUTE_Y:
        case AddressingMode::ABSOLUTE_INDIRECT:
        case AddressingMode::ABSOLUTE_X_INDIRECT:
            return true;
        default:
            return false;
    }
}

std::vector<uint8_t> emitO45(AssemblerParser& parser, const std::string& asmVersion) {
    // Find which segment a symbol's value falls into (lambda to access private Segment).
    auto findSegmentForAddress = [&](uint32_t addr) -> std::string {
        for (const auto& seg : parser.segmentOrder) {
            if (seg->startAddress == 0xFFFFFFFF) continue;
            if (addr >= seg->startAddress && addr < seg->pc) {
                return seg->name;
            }
        }
        return "";
    };
    // --- Step 1: Generate per-segment binary bodies ---
    // We run the generator per-segment, collecting each segment's bytes separately.

    struct SegBody {
        std::string name;
        uint32_t base = 0;
        std::vector<uint8_t> bytes;
    };

    std::vector<std::string> segOrder = parser.requestedSegmentOrder;
    if (segOrder.empty()) segOrder = {"code", "data", "bss"};

    // Build ordered unique segment name list
    std::vector<std::string> allSegNames;
    for (const auto& s : segOrder) {
        if (parser.segments.count(s)) allSegNames.push_back(s);
    }
    for (const auto& seg : parser.segmentOrder) {
        bool found = false;
        for (const auto& s : allSegNames) if (s == seg->name) { found = true; break; }
        if (!found && seg->name != "default") allSegNames.push_back(seg->name);
    }
    // Include default if it has content
    if (parser.segments.count("default") && parser.segments.at("default")->startAddress != 0xFFFFFFFF) {
        bool found = false;
        for (const auto& s : allSegNames) if (s == "default") { found = true; break; }
        if (!found) allSegNames.insert(allSegNames.begin(), "default");
    }

    std::map<std::string, SegBody> segBodies;

    for (const auto& segName : allSegNames) {
        auto seg = parser.segments[segName];
        if (seg->startAddress == 0xFFFFFFFF) continue;
        if (segName == "bss") {
            segBodies[segName] = {segName, seg->startAddress, {}};
            continue;
        }

        std::vector<uint8_t> body;
        M65Emitter e(body, parser.getZPStart());
        e.setAddress(seg->startAddress);

        bool isDeadCode = false;
        std::shared_ptr<AssemblerParser::ProcContext> proc;

        for (auto& [symName, sym] : parser.symbolTable) if (sym.isVariable) sym.value = sym.initialValue;

        for (auto& stmt : parser.statements) {
            if (stmt->segmentName != segName) continue;
            if (stmt->address > e.getAddress()) e.setAddress(stmt->address);
            parser.pc = stmt->address;
            if (stmt->deleted) continue;
            if (!stmt->label.empty()) isDeadCode = false;

            // Use the same generation logic as AssemblerGenerator
            if (stmt->isSimulatedOp() && !isDeadCode) {
                std::vector<uint8_t> d;
                // Emit simulated ops the same way pass2 does for sizing
                // (the actual bytes are already determined)
                // For simplicity, we re-emit via the generator path
            }

            if (stmt->type == AssemblerParser::Statement::INSTRUCTION) {
                if (isDeadCode && stmt->instr.mnemonic != "proc" && stmt->instr.mnemonic != "endproc") continue;
                if (stmt->instr.mnemonic == "rts" || stmt->instr.mnemonic == "rtn" || stmt->instr.mnemonic == "rti") isDeadCode = true;
            }
        }

        segBodies[segName] = {segName, seg->startAddress, body};
    }

    // Actually, the per-segment extraction from the generator is complex because
    // AssemblerGenerator::generate() processes all segments in order into one binary.
    // The simplest correct approach: generate the full binary, then split by segment ranges.

    auto fullBinary = AssemblerGenerator::generate(&parser, false);

    uint32_t globalBase = parser.getFirstOrgAddress();
    if (globalBase == 0xFFFFFFFF) globalBase = 0;

    // Rebuild segBodies from the flat binary using segment address ranges
    segBodies.clear();
    for (const auto& segName : allSegNames) {
        auto seg = parser.segments[segName];
        if (seg->startAddress == 0xFFFFFFFF) continue;

        SegBody sb;
        sb.name = segName;
        sb.base = seg->startAddress;

        if (segName == "bss") {
            sb.bytes.clear(); // BSS has no body
        } else {
            uint32_t segStart = seg->startAddress;
            uint32_t segEnd = seg->pc;
            uint32_t segLen = segEnd - segStart;

            // Offset in the flat binary
            uint32_t binOffset = segStart - globalBase;
            if (binOffset + segLen <= fullBinary.size()) {
                sb.bytes.assign(fullBinary.begin() + binOffset,
                                fullBinary.begin() + binOffset + segLen);
            }
        }
        segBodies[segName] = sb;
    }

    // --- Step 2: Build symbol table ---
    O45SymbolTable syms;

    // Imports from .extern
    for (const auto& name : parser.externSymbols) {
        syms.addImport(name);
    }

    // Exports from .global
    for (const auto& name : parser.globalSymbols) {
        auto* sym = parser.resolveSymbol(name);
        if (!sym) continue;

        // Determine which segment this symbol belongs to
        std::string symSeg = findSegmentForAddress(sym->value);
        O45Segment segId = SEG_TEXT;
        uint32_t offset = sym->value;

        if (!symSeg.empty()) {
            segId = segIdFromName(symSeg);
            offset = sym->value - parser.segments.at(symSeg)->startAddress;
        }

        syms.addExport(name, segId, offset);
    }

    // --- Step 3: Scan statements for relocations ---
    std::map<std::string, std::vector<O45Reloc>> segRelocs; // segment name → reloc list

    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;

        // Only care about instructions with absolute addressing (16-bit address operands)
        if (!isAbsoluteMode(stmt->instr.mode)) continue;

        // Resolve the operand to see if it references a relocatable symbol
        std::string operand = stmt->instr.operand;
        if (operand.empty()) continue;

        Symbol* sym = parser.resolveSymbol(operand, stmt->scopePrefix);
        if (!sym) continue;

        // Is it an extern symbol?
        bool isExtern = parser.isExternSymbol(operand);

        // Is it in a relocatable segment?
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = findSegmentForAddress(sym->value);
            if (targetSeg.empty()) continue; // absolute/unknown — no relocation needed
        }

        // Build relocation entry
        // The operand address is at stmt->address + 1 (after the opcode byte)
        // relative to the segment base
        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = parser.segments.count(srcSeg) ? parser.segments.at(srcSeg)->startAddress : globalBase;
        uint32_t patchOffset = (stmt->address + 1) - srcBase;

        O45Reloc reloc;
        reloc.offset = patchOffset;
        reloc.type = R_WORD; // 16-bit absolute address

        if (isExtern) {
            reloc.segment = SEG_EXTERNAL;
            reloc.symbolIndex = syms.getImportIndex(operand);
        } else {
            reloc.segment = segIdFromName(targetSeg);
        }

        segRelocs[srcSeg].push_back(reloc);
    }

    // Also check .word directives that reference symbols
    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::DIRECTIVE) continue;
        if (stmt->dir.name != "word") continue;

        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = parser.segments.count(srcSeg) ? parser.segments.at(srcSeg)->startAddress : globalBase;
        uint32_t patchAddr = stmt->address;

        for (size_t ai = 0; ai < stmt->dir.arguments.size(); ai++) {
            const std::string& arg = stmt->dir.arguments[ai];
            Symbol* sym = parser.resolveSymbol(arg, stmt->scopePrefix);
            if (!sym) { patchAddr += 2; continue; }

            bool isExtern = parser.isExternSymbol(arg);
            std::string targetSeg;
            if (!isExtern) {
                targetSeg = findSegmentForAddress(sym->value);
                if (targetSeg.empty()) { patchAddr += 2; continue; }
            }

            O45Reloc reloc;
            reloc.offset = patchAddr - srcBase;
            reloc.type = R_WORD;

            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(arg);
            } else {
                reloc.segment = segIdFromName(targetSeg);
            }

            segRelocs[srcSeg].push_back(reloc);
            patchAddr += 2;
        }
    }

    // Sort relocations by offset
    for (auto& [segName, relocs] : segRelocs) {
        std::sort(relocs.begin(), relocs.end(),
                  [](const O45Reloc& a, const O45Reloc& b) { return a.offset < b.offset; });
    }

    // --- Step 4: Package into .o45 ---
    O45Writer writer;
    writer.addDefaultOptions(asmVersion);

    // Set segments
    // Text = "code" or "default"
    std::string textName = segBodies.count("code") ? "code" : "default";
    if (segBodies.count(textName)) {
        const auto& tb = segBodies[textName];
        writer.setTextSegment(tb.base, tb.bytes);
    }

    if (segBodies.count("data")) {
        const auto& db = segBodies["data"];
        writer.setDataSegment(db.base, db.bytes);
    }

    if (segBodies.count("bss")) {
        auto bssSeg = parser.segments.count("bss") ? parser.segments.at("bss") : nullptr;
        if (bssSeg && bssSeg->startAddress != 0xFFFFFFFF) {
            writer.setBssSegment(bssSeg->startAddress, bssSeg->pc - bssSeg->startAddress);
        }
    }

    if (segBodies.count("zp")) {
        auto zpSeg = parser.segments.count("zp") ? parser.segments.at("zp") : nullptr;
        if (zpSeg && zpSeg->startAddress != 0xFFFFFFFF) {
            writer.setZpSegment(zpSeg->startAddress, zpSeg->pc - zpSeg->startAddress);
        }
    }

    // Encode and set relocation tables
    auto encodeRelocs = [&](const std::string& segName) -> std::vector<uint8_t> {
        auto it = segRelocs.find(segName);
        if (it == segRelocs.end()) return {};
        return O45RelocEncoder::encode(it->second);
    };

    writer.setTextRelocations(encodeRelocs(textName));
    writer.setDataRelocations(encodeRelocs("data"));

    // Apply symbols
    syms.applyTo(writer);

    return writer.emit();
}
