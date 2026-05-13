#include "O45Emitter.hpp"
#include "O45Writer.hpp"
#include "O45Types.hpp"
#include "AssemblerParser.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Map ca45 segment names to O45 segment IDs.
static O45Segment segIdFromName(const std::string& name) {
    if (name == "code" || name == "text" || name == "init") return SEG_TEXT;
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
    // --- Step 1: Generate per-segment binary bodies ---
    // We run the generator per-segment, collecting each segment's bytes separately.

    struct SegBody {
        std::string name;
        uint32_t base = 0;
        std::vector<uint8_t> bytes;
    };

    std::vector<std::string> segOrder = parser.requestedSegmentOrder;
    if (segOrder.empty()) segOrder = {"init", "code", "data", "bss"};

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
    if (parser.segments.count("default") && (parser.segments.at("default")->startAddress != 0xFFFFFFFF || parser.segments.at("default")->pc > 0)) {
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
        M65Emitter e(body, parser.getZPStart()); e.setSpBase(parser.getSpBase());
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

    std::vector<uint8_t> fullBinary;
    M65Emitter genEmitter(fullBinary, parser.getZPStart()); genEmitter.setSpBase(parser.getSpBase());
    uint32_t genStart = parser.getFirstOrgAddress();
    if (genStart != 0xFFFFFFFF) genEmitter.setAddress(genStart);
    AssemblerGenerator::generate(&parser, genEmitter);

    uint32_t globalBase = parser.getFirstOrgAddress();
    if (globalBase == 0xFFFFFFFF) globalBase = 0;

    // Rebuild segBodies from the flat binary using segment address ranges
    segBodies.clear();
    for (const auto& segName : allSegNames) {
        auto seg = parser.segments[segName];
        // Skip segments that have no content (no statements were placed in them).
        // A segment has content if its startAddress was set (via .org or by emitting
        // code into it) OR if it has a nonzero size in the original object.
        bool hasContent = (seg->startAddress != 0xFFFFFFFF);
        if (!hasContent) {
            // Check if any statements were emitted into this segment
            for (const auto& stmt : parser.statements) {
                if (stmt->segmentName == segName && !stmt->deleted && stmt->size > 0) {
                    hasContent = true;
                    break;
                }
            }
        }
        if (!hasContent) continue;

        SegBody sb;
        sb.name = segName;
        sb.base = (seg->startAddress != 0xFFFFFFFF) ? seg->startAddress : 0;

        if (segName == "bss") {
            sb.bytes.clear(); // BSS has no body
        } else {
            uint32_t segStart = sb.base;
            uint32_t segEnd = seg->pc;
            uint32_t segLen = (segEnd > segStart) ? (segEnd - segStart) : 0;

            // Offset in the flat binary
            uint32_t binOffset = segStart - globalBase;
            // Clamp to actual binary size (PC may overshoot due to dead code)
            if (binOffset < fullBinary.size()) {
                uint32_t avail = (uint32_t)fullBinary.size() - binOffset;
                uint32_t copyLen = (segLen <= avail) ? segLen : avail;
                sb.bytes.assign(fullBinary.begin() + binOffset,
                                fullBinary.begin() + binOffset + copyLen);
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

        // Equates (isAddress=false) are absolute values — not segment-relative
        if (!sym->isAddress) {
            bool isWeak = parser.isWeakSymbol(name);
            syms.addExport(name, SEG_ABSOLUTE, sym->value, isWeak);
            continue;
        }

        // Use the authoritative segment field set at parse time
        std::string symSeg = sym->segment;
        O45Segment segId = SEG_TEXT;
        uint32_t offset = sym->value;

        if (!symSeg.empty() && parser.segments.count(symSeg)) {
            segId = segIdFromName(symSeg);
            uint32_t segBase = parser.segments.at(symSeg)->startAddress;
            if (segBase == 0xFFFFFFFF) segBase = 0;
            offset = sym->value - segBase;
        }

        bool isWeak = parser.isWeakSymbol(name);
        syms.addExport(name, segId, offset, isWeak);
    }

    // --- Step 2b: Attach function attributes from proc contexts ---
    for (const auto& [addr, proc] : parser.getProcedures()) {
        if (!proc->hasFuncAttrs) continue;
        if (!syms.isExported(proc->name)) continue;
        O45FuncAttr attr;
        attr.flags = proc->funcFlags;
        attr.regClobbers = proc->regClobbersMask;
        attr.flagClobbers = proc->flagClobbersMask;
        attr.zpUses = proc->zpUsesMask;
        attr.zpClobbers = proc->zpClobbersMask;
        attr.zpRelease = proc->zpReleaseMask;
        attr.paramSize = (uint8_t)proc->totalParamSize;
        syms.setFuncAttr(proc->name, attr);
    }

    // --- Step 3: Scan statements for relocations ---
    std::map<std::string, std::vector<O45Reloc>> segRelocs; // segment name → reloc list

    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;

        // Restore @ label scope context for this statement
        parser.currentLocalScope_ = stmt->localLabelScope;

        // Resolve the actual addressing mode (stmt->instr.mode may be stale from
        // initial parsing — e.g., equate operands like `ptr = $20` default to ABSOLUTE
        // during parsing because parseNumericLiteral can't resolve identifiers, but
        // calculateInstructionSize correctly resolves them to BASE_PAGE in pass2
        // without writing the resolved mode back).
        AddressingMode resolvedMode = stmt->instr.mode;
        if (!stmt->instr.forceMode && stmt->instr.operandTokenIndex != -1 &&
            (resolvedMode == AddressingMode::BASE_PAGE || resolvedMode == AddressingMode::ABSOLUTE ||
             resolvedMode == AddressingMode::BASE_PAGE_X || resolvedMode == AddressingMode::ABSOLUTE_X ||
             resolvedMode == AddressingMode::BASE_PAGE_Y || resolvedMode == AddressingMode::ABSOLUTE_Y)) {
            try {
                uint32_t val = parser.evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                bool fitsIn8 = (val <= 0xFF);
                bool forceAbs = (stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp");
                if (fitsIn8 && !forceAbs && stmt->instr.operandTokenIndex < (int)parser.tokens.size() &&
                    parser.tokens[stmt->instr.operandTokenIndex].type == AssemblerTokenType::IDENTIFIER) {
                    std::string symName = stmt->scopePrefix + parser.tokens[stmt->instr.operandTokenIndex].value;
                    if (!parser.symbolTable.count(symName)) symName = parser.tokens[stmt->instr.operandTokenIndex].value;
                    if (parser.isRelocatableSymbol(symName)) forceAbs = true;
                }
                if (resolvedMode == AddressingMode::BASE_PAGE || resolvedMode == AddressingMode::ABSOLUTE)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE : AddressingMode::ABSOLUTE;
                else if (resolvedMode == AddressingMode::BASE_PAGE_X || resolvedMode == AddressingMode::ABSOLUTE_X)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_X : AddressingMode::ABSOLUTE_X;
                else if (resolvedMode == AddressingMode::BASE_PAGE_Y || resolvedMode == AddressingMode::ABSOLUTE_Y)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_Y : AddressingMode::ABSOLUTE_Y;
            } catch(...) {
                // Can't resolve — keep original mode (safe: ABSOLUTE)
            }
        }

        // Only care about instructions with absolute addressing (16-bit address operands)
        if (!isAbsoluteMode(resolvedMode)) continue;

        // Skip branch instructions — they use relative offsets, not absolute addresses
        static const std::set<std::string> branches = {
            "beq", "bne", "bra", "bcc", "bcs", "bpl", "bmi", "bvc", "bvs", "bsr",
            "bbr0","bbr1","bbr2","bbr3","bbr4","bbr5","bbr6","bbr7",
            "bbs0","bbs1","bbs2","bbs3","bbs4","bbs5","bbs6","bbs7"
        };
        if (branches.count(stmt->instr.mnemonic)) continue;

        // Resolve the operand to see if it references a relocatable symbol
        std::string operand = stmt->instr.operand;
        if (operand.empty()) continue;

        // Skip numeric literals — they're absolute addresses, not relocatable
        if (operand[0] == '$' || operand[0] == '%' || (operand[0] >= '0' && operand[0] <= '9')) continue;

        // Try resolving the full operand as a symbol first (simple case: bare label)
        std::string symName = operand;
        Symbol* sym = parser.resolveSymbol(symName, stmt->scopePrefix);
        if (!sym) {
            // Compound expression (e.g. "table+4"): extract base symbol from token stream
            int tokIdx = stmt->instr.operandTokenIndex;
            if (tokIdx >= 0 && tokIdx < (int)parser.tokens.size() &&
                parser.tokens[tokIdx].type == AssemblerTokenType::IDENTIFIER) {
                symName = parser.tokens[tokIdx].value;
                sym = parser.resolveSymbol(symName, stmt->scopePrefix);
            }
        }
        if (!sym) continue;

        // Is it an extern symbol?
        bool isExtern = parser.isExternSymbol(symName);

        // Is it in a relocatable segment?
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue; // absolute/unknown — no relocation needed
        }

        // Build relocation entry
        // The operand address is at stmt->address + 1 (after the opcode byte)
        // relative to the segment base
        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = globalBase;
        if (parser.segments.count(srcSeg)) {
            srcBase = parser.segments.at(srcSeg)->startAddress;
            if (srcBase == 0xFFFFFFFF) srcBase = 0;
        }
        uint32_t patchOffset = (stmt->address + 1) - srcBase;

        O45Reloc reloc;
        reloc.offset = patchOffset;
        reloc.type = R_WORD; // 16-bit absolute address

        if (isExtern) {
            reloc.segment = SEG_EXTERNAL;
            reloc.symbolIndex = syms.getImportIndex(symName);
        } else {
            reloc.segment = segIdFromName(targetSeg);
        }

        segRelocs[srcSeg].push_back(reloc);
    }

    // Scan simulated LDW (ldax #<label) for immediate address loads
    // These emit LDA #lo; LDX #hi (4 bytes) — need R_LOW at +1, R_HIGH at +3
    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::LDW) continue;
        parser.currentLocalScope_ = stmt->localLabelScope;

        // Check if operand is immediate (#) by looking at the token stream
        int idx = stmt->exprTokenIndex;
        if (idx < 0 || idx >= (int)parser.tokens.size()) continue;
        if (parser.tokens[idx].type != AssemblerTokenType::HASH) continue;
        idx++; // skip #

        // Check for < prefix (address-of operator from compiler)
        bool hasLowPrefix = false;
        if (idx < (int)parser.tokens.size() && parser.tokens[idx].value == "<") {
            hasLowPrefix = true;
            idx++;
        }

        if (idx >= (int)parser.tokens.size()) continue;
        std::string symName = parser.tokens[idx].value;

        // Try to resolve as a symbol
        Symbol* sym = parser.resolveSymbol(symName, stmt->scopePrefix);
        if (!sym || !sym->isAddress) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
        }

        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = globalBase;
        if (parser.segments.count(srcSeg)) {
            srcBase = parser.segments.at(srcSeg)->startAddress;
            if (srcBase == 0xFFFFFFFF) srcBase = 0;
        }

        // LDA #lo is at stmt->address + 0, operand byte at +1
        // LDX #hi is at stmt->address + 2, operand byte at +3
        uint32_t baseOff = stmt->address - srcBase;

        // The R_HIGH extra byte carries the low byte for carry correction
        uint8_t lowByte = (uint8_t)(sym->value & 0xFF);

        if (isExtern) {
            uint32_t symIdx = syms.getImportIndex(symName);
            segRelocs[srcSeg].push_back({baseOff + 1, R_LOW, SEG_EXTERNAL, symIdx, 0});
            segRelocs[srcSeg].push_back({baseOff + 3, R_HIGH, SEG_EXTERNAL, symIdx, lowByte});
        } else {
            O45Segment segId = segIdFromName(targetSeg);
            segRelocs[srcSeg].push_back({baseOff + 1, R_LOW, segId, 0, 0});
            segRelocs[srcSeg].push_back({baseOff + 3, R_HIGH, segId, 0, lowByte});
        }
    }

    // Scan immediate instructions with <symbol or >symbol for lo/hi byte relocs,
    // and PHW #symbol (IMMEDIATE16) for full word relocs.
    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;
        if (stmt->instr.mode != AddressingMode::IMMEDIATE &&
            stmt->instr.mode != AddressingMode::IMMEDIATE16) continue;
        parser.currentLocalScope_ = stmt->localLabelScope;

        int idx = stmt->instr.operandTokenIndex;
        if (idx < 0 || idx >= (int)parser.tokens.size()) continue;

        // Determine if this is a lo/hi byte reference or a full word (PHW)
        bool isLow = false, isHigh = false, isWord = false;
        if (parser.tokens[idx].value == "<") {
            isLow = true;
            idx++;
        } else if (parser.tokens[idx].value == ">") {
            isHigh = true;
            idx++;
        } else if (stmt->instr.mode == AddressingMode::IMMEDIATE16) {
            // PHW #symbol — full 16-bit immediate
            isWord = true;
        }

        if (!isLow && !isHigh && !isWord) continue;
        if (idx >= (int)parser.tokens.size()) continue;

        std::string symName = parser.tokens[idx].value;
        Symbol* sym = parser.resolveSymbol(symName, stmt->scopePrefix);
        if (!sym || !sym->isAddress) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
        }

        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = globalBase;
        if (parser.segments.count(srcSeg)) {
            srcBase = parser.segments.at(srcSeg)->startAddress;
            if (srcBase == 0xFFFFFFFF) srcBase = 0;
        }

        // Operand byte is at stmt->address + 1 (after the opcode)
        uint32_t patchOff = (stmt->address + 1) - srcBase;

        if (isWord) {
            O45Reloc reloc;
            reloc.offset = patchOff;
            // Use R_LINEAR32 for 32-bit symbols, R_WORD for 16-bit
            reloc.type = (sym->value > 0xFFFF) ? R_LINEAR32 : R_WORD;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg);
            }
            segRelocs[srcSeg].push_back(reloc);
        } else if (isLow) {
            O45Reloc reloc;
            reloc.offset = patchOff;
            reloc.type = R_LOW;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg);
            }
            segRelocs[srcSeg].push_back(reloc);
        } else { // isHigh
            uint8_t lowByte = (uint8_t)(sym->value & 0xFF);
            O45Reloc reloc;
            reloc.offset = patchOff;
            reloc.type = R_HIGH;
            reloc.extra = lowByte;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg);
            }
            segRelocs[srcSeg].push_back(reloc);
        }
    }

    // Also check .word directives that reference symbols
    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->size == 0) continue;
        if (stmt->type != AssemblerParser::Statement::DIRECTIVE) continue;
        if (stmt->dir.name != "word") continue;
        parser.currentLocalScope_ = stmt->localLabelScope;

        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = globalBase;
        if (parser.segments.count(srcSeg)) {
            srcBase = parser.segments.at(srcSeg)->startAddress;
            if (srcBase == 0xFFFFFFFF) srcBase = 0;
        }
        uint32_t patchAddr = stmt->address;

        for (size_t ai = 0; ai < stmt->dir.arguments.size(); ai++) {
            const std::string& arg = stmt->dir.arguments[ai];
            Symbol* sym = parser.resolveSymbol(arg, stmt->scopePrefix);
            // For 32-bit symbols in .word directives, skip (they should use .dword)
            if (!sym) { patchAddr += 2; continue; }
            if (sym->value > 0xFFFF) {
                // .word is 16-bit; 32-bit symbols require .dword
                patchAddr += 2;
                continue;
            }

            bool isExtern = parser.isExternSymbol(arg);
            std::string targetSeg;
            if (!isExtern) {
                targetSeg = sym->segment;
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

    // Scan __sp_base relocation sites from simulated stack ops.
    // If __sp_base is declared .extern, each site needs an R_WORD reloc.
    // The assembler writes spBase+addend at each site; we patch it to just
    // the addend so the linker can add the resolved __sp_base value.
    if (parser.isExternSymbol("__sp_base")) {
        uint32_t spBaseIdx = syms.getImportIndex("__sp_base");
        uint16_t asmSpBase = genEmitter.spBase(); // built-in default (e.g., $0101)
        for (const auto& sr : genEmitter.spBaseRelocs()) {
            // Determine which segment this address falls into
            for (const auto& segName : allSegNames) {
                auto seg = parser.segments[segName];
                uint32_t segStart = (seg->startAddress != 0xFFFFFFFF) ? seg->startAddress : 0;
                if (sr.address >= segStart && sr.address < seg->pc) {
                    // Patch binary: replace spBase+addend with just addend
                    uint32_t patchOff = sr.address - segStart;
                    if (patchOff + 1 < segBodies[segName].bytes.size()) {
                        segBodies[segName].bytes[patchOff]     = (uint8_t)(sr.addend & 0xFF);
                        segBodies[segName].bytes[patchOff + 1] = (uint8_t)((sr.addend >> 8) & 0xFF);
                    }
                    O45Reloc reloc;
                    reloc.offset = patchOff;
                    reloc.type = R_WORD;
                    reloc.segment = SEG_EXTERNAL;
                    reloc.symbolIndex = spBaseIdx;
                    segRelocs[segName].push_back(reloc);
                    break;
                }
            }
        }
    } else if (!genEmitter.spBaseRelocs().empty()) {
        std::cerr << "warning: stack-relative access uses built-in __sp_base ($"
                  << std::hex << std::uppercase << std::setfill('0') << std::setw(4)
                  << genEmitter.spBase()
                  << "); add '.extern __sp_base' for relocatable code" << std::endl;
    }

    // Scan symbol relocation sites from simulated ops (ldax/stax of global variables).
    for (const auto& sr : genEmitter.symbolRelocs()) {
        std::string symName = sr.symbolName;
        Symbol* sym = parser.resolveSymbol(symName, "");
        if (!sym) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
        }

        // Find which source segment this reloc site falls into
        for (const auto& segName : allSegNames) {
            auto seg = parser.segments[segName];
            uint32_t segStart = (seg->startAddress != 0xFFFFFFFF) ? seg->startAddress : 0;
            if (sr.address >= segStart && sr.address < seg->pc) {
                O45Reloc reloc;
                reloc.offset = sr.address - segStart;
                reloc.type = (O45RelocType)sr.relocType;
                reloc.extra = sr.extra;
                if (isExtern) {
                    reloc.segment = SEG_EXTERNAL;
                    reloc.symbolIndex = syms.getImportIndex(symName);
                } else {
                    reloc.segment = segIdFromName(targetSeg);
                }
                segRelocs[segName].push_back(reloc);
                break;
            }
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

    // Collect all assembler segments that map to SEG_TEXT, in order.
    // Concatenate their bodies and emit OPT_SEGATTR records.
    std::vector<std::string> textSegNames;
    for (const auto& name : allSegNames) {
        if (segBodies.count(name) && segIdFromName(name) == SEG_TEXT) {
            textSegNames.push_back(name);
        }
    }
    // Fallback: if no named text segments, try "default"
    if (textSegNames.empty() && segBodies.count("default")) {
        textSegNames.push_back("default");
    }

    // Build merged text body from all text sub-segments, tracking per-sub-segment
    // offsets for symbol adjustment.
    std::map<std::string, uint32_t> textSubSegOffset; // segName -> offset in merged body
    {
        std::vector<uint8_t> mergedTextBody;
        std::vector<O45Reloc> mergedTextRelocs;
        uint32_t textBase = 0;
        bool baseSet = false;

        for (const auto& name : textSegNames) {
            const auto& sb = segBodies[name];
            uint32_t offset = (uint32_t)mergedTextBody.size();
            textSubSegOffset[name] = offset;

            if (!baseSet && !sb.bytes.empty()) {
                textBase = sb.base;
                baseSet = true;
            }

            // Emit OPT_SEGATTR when there are multiple text sub-segments,
            // or when a single sub-segment has a non-default name (e.g., "init")
            bool needAttr = (textSegNames.size() > 1) ||
                            (name != "code" && name != "default" && name != "text");
            if (needAttr && !sb.bytes.empty()) {
                std::vector<uint8_t> payload;
                payload.push_back((uint8_t)SEG_TEXT);
                // offset (4 LE)
                payload.push_back((uint8_t)(offset & 0xFF));
                payload.push_back((uint8_t)((offset >> 8) & 0xFF));
                payload.push_back((uint8_t)((offset >> 16) & 0xFF));
                payload.push_back((uint8_t)((offset >> 24) & 0xFF));
                // length (4 LE)
                uint32_t len = (uint32_t)sb.bytes.size();
                payload.push_back((uint8_t)(len & 0xFF));
                payload.push_back((uint8_t)((len >> 8) & 0xFF));
                payload.push_back((uint8_t)((len >> 16) & 0xFF));
                payload.push_back((uint8_t)((len >> 24) & 0xFF));
                // name (NUL-terminated)
                for (char c : name) payload.push_back((uint8_t)c);
                payload.push_back(0x00);
                writer.addOptionRaw(OPT_SEGATTR, payload);
            }

            // Adjust relocation offsets for this sub-segment
            auto it = segRelocs.find(name);
            if (it != segRelocs.end()) {
                for (auto r : it->second) {
                    r.offset += offset;
                    mergedTextRelocs.push_back(r);
                }
            }

            mergedTextBody.insert(mergedTextBody.end(), sb.bytes.begin(), sb.bytes.end());
        }

        // Sort merged relocs by offset
        std::sort(mergedTextRelocs.begin(), mergedTextRelocs.end(),
                  [](const O45Reloc& a, const O45Reloc& b) { return a.offset < b.offset; });

        if (!mergedTextBody.empty()) {
            writer.setTextSegment(textBase, mergedTextBody);
        }
        writer.setTextRelocations(O45RelocEncoder::encode(mergedTextRelocs));

        // Adjust exported symbol offsets for merged text sub-segments.
        // Symbols were computed relative to their individual segment base;
        // add the sub-segment's position within the merged text body.
        if (textSegNames.size() > 1) {
            for (auto& exp : syms.getExportsMut()) {
                if (exp.segment != SEG_TEXT) continue;
                // Find which sub-segment this symbol belongs to by checking
                // its original assembler segment name
                auto* sym = parser.resolveSymbol(exp.name);
                if (!sym || sym->segment.empty()) continue;
                auto adjIt = textSubSegOffset.find(sym->segment);
                if (adjIt != textSubSegOffset.end() && adjIt->second > 0) {
                    exp.offset += adjIt->second;
                }
            }
        }
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

    // Data relocation table
    {
        auto it = segRelocs.find("data");
        writer.setDataRelocations(it != segRelocs.end() ? O45RelocEncoder::encode(it->second) : std::vector<uint8_t>{});
    }

    // Apply symbols
    syms.applyTo(writer);

    return writer.emit();
}
