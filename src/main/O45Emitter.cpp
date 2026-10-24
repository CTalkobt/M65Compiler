#include "O45Emitter.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "Diagnostic.hpp"
#include "O45Writer.hpp"
#include "O45Types.hpp"
#include "AssemblerParser.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include "StringUtil.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <set>

// Map ca45 segment names to O45 segment IDs.
// Offset of an instruction's 16-bit operand: after the opcode, or after the
// $42 $42 prefix and opcode of a 32-bit (Q) instruction (5 bytes)
static uint32_t operandOffset(const AssemblerParser::Statement* stmt) {
    return stmt->size >= 5 ? (uint32_t)stmt->size - 2 : 1;
}

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

    std::vector<M65Emitter::SpBaseReloc> allSpBaseRelocs;
    std::vector<M65Emitter::SymbolReloc> allSymbolRelocs;
    std::vector<M65Emitter::ImmediateReloc> allImmediateRelocs; // Phase 78: SMC immediate relocations
    // Segment each recorded reloc was emitted in (segments overlap in address
    // space in relocatable mode, so an address alone can't identify it)
    std::vector<std::string> spBaseRelocSeg, symbolRelocSeg, immediateRelocSeg;
    int bpPoolTop = 0x30;   // (cc45.bptop: the register pool's bytes the code names)

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
        e.setObjectMode(true);
        {
            std::vector<M65Emitter::ZpRange> zr;
            for (const auto& z : parser.zpSyms) zr.push_back({z.start, z.len, z.name});
            e.setZpSyms(std::move(zr));
        }
        AssemblerGenerator::generate(&parser, e, segName);

        auto spRelocs = e.spBaseRelocs();
        allSpBaseRelocs.insert(allSpBaseRelocs.end(), spRelocs.begin(), spRelocs.end());
        spBaseRelocSeg.insert(spBaseRelocSeg.end(), spRelocs.size(), segName);

        auto symRelocs = e.symbolRelocs();
        allSymbolRelocs.insert(allSymbolRelocs.end(), symRelocs.begin(), symRelocs.end());
        symbolRelocSeg.insert(symbolRelocSeg.end(), symRelocs.size(), segName);

        // Phase 78: Collect immediate relocations for SMC parameters
        auto immRelocs = e.immediateRelocs();
        allImmediateRelocs.insert(allImmediateRelocs.end(), immRelocs.begin(), immRelocs.end());
        immediateRelocSeg.insert(immediateRelocSeg.end(), immRelocs.size(), segName);

        bpPoolTop = std::max(bpPoolTop, e.bpPoolTop());
        segBodies[segName] = {segName, seg->startAddress, body};
    }

    uint32_t globalBase = parser.getFirstOrgAddress();
    if (globalBase == 0xFFFFFFFF) globalBase = 0;
    // For line map offsets, use the code segment's start address (not global base).
    // In .o45 mode, data may precede code in the assembler's address space,
    // but line offsets in the .o45 must be relative to the text segment start.
    uint32_t genStart = globalBase;
    if (segBodies.count("code")) {
        genStart = segBodies["code"].base;
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

    // The end of the register pool ($30-$7F) this object's instructions name
    // directly: ln45 places the functions' base-page blocks above the
    // largest (an object without it counts as using the whole pool); .bptop
    // states it instead
    if (parser.bpTop >= 0) bpPoolTop = parser.bpTop;
    if (!syms.isExported("cc45.bptop")) syms.addExport("cc45.bptop", SEG_ABSOLUTE, bpPoolTop, true);

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
        attr.frameSize = proc->frameSize;  // Phase 2: frame size for overlay coloring

        // Phase 3: Collect SAC parameter metadata for SAC functions
        if ((attr.flags & FUNC_FLAG_STATIC_ALLOC) != 0) {
            attr.sacMetadata.functionName = proc->name;

            // Look for parameter symbols matching pattern: funcname__param_*
            std::string paramPrefix = proc->name + "__param_";
            int paramIndex = 0;
            for (const auto& [symName, sym] : parser.symbolTable) {
                if (symName.find(paramPrefix) == 0) {
                    O45SACParam param;
                    param.symbolName = symName;
                    param.offset = sym.value;  // Will be relative to AR base after linking
                    param.size = (paramIndex < (int)proc->paramSizes.size())
                        ? proc->paramSizes[paramIndex] : 2;  // Default to 2 if no size info

                    // Check if this parameter has a constant value from .param_const directive
                    if (proc->paramConstants.find(paramIndex) != proc->paramConstants.end()) {
                        param.isConstant = true;
                        param.constantValue = proc->paramConstants.at(paramIndex);
                    }

                    attr.sacMetadata.parameters.push_back(param);
                    paramIndex++;
                }
            }
        }

        syms.setFuncAttr(proc->name, attr);
    }

    // --- Step 3: Scan statements for relocations ---
    std::map<std::string, std::vector<O45Reloc>> segRelocs; // segment name → reloc list

    // An operand naming several labels (blk_end-blk, buf+len): relocated
    // against its first label's segment only if its value holds that
    // segment's base once and no other's; a constant (a difference of two
    // labels of one segment) gets no relocation; anything else is an error
    auto relocatedAs = [&](const AssemblerParser::Statement* stmt, int idx, const std::string& seg) {
        auto m = parser.segmentMultiples(idx, stmt->scopePrefix);
        if (m.empty()) return true;
        bool any = false;
        for (const auto& [s, k] : m) {
            if (k == 0) continue;
            if (s != seg || k != 1)
                throw std::runtime_error(stmt->sourceFile + ":" + std::to_string(stmt->line) +
                    ": error: the operand's value can't be relocated: it holds the address of a label of segment '" +
                    s + "' " + (k == 0x7FFF ? std::string("in part") : std::to_string(k) + " times") + " (a difference of two labels of one segment is a constant)");
            any = true;
        }
        return any;
    };

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

        // Check if operand is an AR reference (e.g., _main:__ar+2) to force ABSOLUTE mode
        std::string operand = stmt->instr.operand;
        bool isArRef = operand.find(":__ar") != std::string::npos;

        if (!stmt->instr.forceMode && stmt->instr.operandTokenIndex != -1 &&
            (resolvedMode == AddressingMode::BASE_PAGE || resolvedMode == AddressingMode::ABSOLUTE ||
             resolvedMode == AddressingMode::BASE_PAGE_X || resolvedMode == AddressingMode::ABSOLUTE_X ||
             resolvedMode == AddressingMode::BASE_PAGE_Y || resolvedMode == AddressingMode::ABSOLUTE_Y)) {
            try {
                uint32_t val = parser.evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                bool fitsIn8 = (val <= 0xFF);
                bool forceAbs = (stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp") || isArRef;
                if (fitsIn8 && !forceAbs && stmt->instr.operandTokenIndex < (int)parser.tokens.size() &&
                    parser.tokens[stmt->instr.operandTokenIndex].type == AssemblerTokenType::IDENTIFIER) {
                    std::string symName = stmt->scopePrefix + parser.tokens[stmt->instr.operandTokenIndex].value;
                    if (!parser.symbolTable.count(symName)) symName = parser.tokens[stmt->instr.operandTokenIndex].value;
                    if (parser.isRelocatableSymbol(symName) &&
                        !parser.isZpSegmentSymbolIn(parser.tokens[stmt->instr.operandTokenIndex].value, stmt->scopePrefix))
                        forceAbs = true;
                }
                if (resolvedMode == AddressingMode::BASE_PAGE || resolvedMode == AddressingMode::ABSOLUTE)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE : AddressingMode::ABSOLUTE;
                else if (resolvedMode == AddressingMode::BASE_PAGE_X || resolvedMode == AddressingMode::ABSOLUTE_X)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_X : AddressingMode::ABSOLUTE_X;
                else if (resolvedMode == AddressingMode::BASE_PAGE_Y || resolvedMode == AddressingMode::ABSOLUTE_Y)
                    resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_Y : AddressingMode::ABSOLUTE_Y;
                // an instruction without the base-page mode (LDZ, LDA bp,Y) is
                // assembled absolute, as the generator does
                if (resolvedMode == AddressingMode::BASE_PAGE && !AssemblerOpcodeDatabase::isValidMode(stmt->instr.mnemonic, AddressingMode::BASE_PAGE))
                    resolvedMode = AddressingMode::ABSOLUTE;
                else if (resolvedMode == AddressingMode::BASE_PAGE_X && !AssemblerOpcodeDatabase::isValidMode(stmt->instr.mnemonic, AddressingMode::BASE_PAGE_X))
                    resolvedMode = AddressingMode::ABSOLUTE_X;
                else if (resolvedMode == AddressingMode::BASE_PAGE_Y && !AssemblerOpcodeDatabase::isValidMode(stmt->instr.mnemonic, AddressingMode::BASE_PAGE_Y))
                    resolvedMode = AddressingMode::ABSOLUTE_Y;
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
        if (operand.empty()) continue;

        // Skip numeric literals — they're absolute addresses, not relocatable
        if (operand[0] == '$' || operand[0] == '%' || (operand[0] >= '0' && operand[0] <= '9')) continue;

        // Check for SAC AR symbol relocation first (e.g., _main:__ar+2)
        AssemblerParser::ArRelocation arReloc = parser.tryParseArRelocation(operand);
        if (arReloc.isArReloc) {
            // This is an AR relocation — create an external reference
            // To avoid addend encoding complexity, use the offset as part of the symbol name
            std::string arSymbolWithOffset = arReloc.arSymbol;
            if (arReloc.addend != 0) {
                arSymbolWithOffset += "_" + std::to_string(arReloc.addend);
            }

            std::string srcSeg = stmt->segmentName;
            uint32_t srcBase = globalBase;
            if (parser.segments.count(srcSeg)) {
                srcBase = parser.segments.at(srcSeg)->startAddress;
                if (srcBase == 0xFFFFFFFF) srcBase = 0;
            }
            uint32_t patchOffset = (stmt->address + operandOffset(stmt.get())) - srcBase;

            // Add as import and create external relocation
            syms.addImport(arSymbolWithOffset);

            O45Reloc reloc;
            reloc.offset = patchOffset;
            reloc.type = R_WORD;
            reloc.segment = SEG_EXTERNAL;
            reloc.symbolIndex = syms.getImportIndex(arSymbolWithOffset);

            segRelocs[srcSeg].push_back(reloc);
            continue;  // Skip further processing for this operand
        }

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
            if (!relocatedAs(stmt.get(), stmt->instr.operandTokenIndex, targetSeg)) continue;
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
        uint32_t patchOffset = (stmt->address + operandOffset(stmt.get())) - srcBase;

        O45Reloc reloc;
        reloc.offset = patchOffset;
        reloc.type = R_WORD; // 16-bit absolute address

        // A .weak symbol is referenced by name (an import), even when this
        // object defines it: the linker binds it to a strong definition
        // from another object if there is one (crt0's jsr _init_features,
        // jsr ___mainargs)
        bool byName = isExtern || parser.isWeakSymbol(symName);
        if (byName) {
            if (!isExtern) {
                syms.addImport(symName);
                // An import's operand bytes hold only the addend (the linker
                // adds the symbol): take this object's value of it out
                auto& body = segBodies[srcSeg].bytes;
                if (patchOffset + 1 < body.size()) {
                    uint16_t w = (uint16_t)(body[patchOffset] | (body[patchOffset + 1] << 8));
                    w = (uint16_t)(w - sym->value);
                    body[patchOffset] = (uint8_t)(w & 0xFF);
                    body[patchOffset + 1] = (uint8_t)(w >> 8);
                }
            }
            reloc.segment = SEG_EXTERNAL;
            reloc.symbolIndex = syms.getImportIndex(symName);
        } else {
            reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
        }

        segRelocs[srcSeg].push_back(reloc);
    }

    // (ldax/lday/ldaz #sym[+n]: the emitter records their R_LOW/R_HIGH,
    // with the operand's low byte, in allSymbolRelocs below)

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
        } else if (parser.tokens[idx].type == AssemblerTokenType::IDENTIFIER &&
                   parser.isExternSymbol(parser.tokens[idx].value)) {
            // #sym of an external symbol (an absolute value the linker
            // defines, say): its low byte, as #<sym (it assembled as 0)
            isLow = true;
        }

        if (!isLow && !isHigh && !isWord) continue;
        if (idx >= (int)parser.tokens.size()) continue;
        // #<(sym+2): the symbol inside the parentheses; the whole expression
        // gives the carry byte
        int exprIdx = idx;
        while (idx + 1 < (int)parser.tokens.size() && parser.tokens[idx].value == "(") idx++;

        std::string symName = parser.tokens[idx].value;
        Symbol* sym = parser.resolveSymbol(symName, stmt->scopePrefix);
        if (!sym || !sym->isAddress) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
            if (!relocatedAs(stmt.get(), exprIdx, targetSeg)) continue;
        }

        std::string srcSeg = stmt->segmentName;
        uint32_t srcBase = globalBase;
        if (parser.segments.count(srcSeg)) {
            srcBase = parser.segments.at(srcSeg)->startAddress;
            if (srcBase == 0xFFFFFFFF) srcBase = 0;
        }

        // Operand byte is at stmt->address + 1 (after the opcode)
        uint32_t patchOff = (stmt->address + operandOffset(stmt.get())) - srcBase;

        if (isWord) {
            O45Reloc reloc;
            reloc.offset = patchOff;
            // Use R_LINEAR32 for 32-bit symbols, R_WORD for 16-bit
            reloc.type = (sym->value > 0xFFFF) ? R_LINEAR32 : R_WORD;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
            }
            segRelocs[srcSeg].push_back(reloc);
        } else if (isLow) {
            // R_LOWX: the extra byte is the high byte of the whole value the
            // low byte is taken from, so the linker can rebuild it
            uint8_t highByte = (uint8_t)((sym->value >> 8) & 0xFF);
            try { highByte = (uint8_t)((parser.evaluateExpressionAt(exprIdx, stmt->scopePrefix) >> 8) & 0xFF); } catch (...) {}
            O45Reloc reloc;
            reloc.offset = patchOff;
            reloc.type = R_LOWX;
            reloc.extra = highByte;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
            }
            segRelocs[srcSeg].push_back(reloc);
        } else { // isHigh
            // The extra byte is the low byte of the whole value the high
            // byte is taken from ('>' applies to everything after it:
            // #>buf+2 is the high byte of buf+2), for the linker's carry
            uint8_t lowByte = (uint8_t)(sym->value & 0xFF);
            try { lowByte = (uint8_t)(parser.evaluateExpressionAt(exprIdx, stmt->scopePrefix) & 0xFF); } catch (...) {}
            O45Reloc reloc;
            reloc.offset = patchOff;
            reloc.type = R_HIGH;
            reloc.extra = lowByte;
            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(symName);
            } else {
                reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
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
            // The symbol of an expression argument: its first name (lbl+1, 2*lbl)
            const std::string& full = stmt->dir.arguments[ai];
            int ti = ai < stmt->dir.argTokenIndices.size() ? stmt->dir.argTokenIndices[ai] : -1;
            std::string arg = full.substr(0, full.find_first_of("+-", 1));
            for (int t = ti; ti >= 0 && t < (int)parser.tokens.size(); ++t) {
                auto ty = parser.tokens[t].type;
                if (ty == AssemblerTokenType::COMMA || ty == AssemblerTokenType::NEWLINE ||
                    ty == AssemblerTokenType::END_OF_FILE) break;
                if (ty == AssemblerTokenType::IDENTIFIER) { arg = parser.tokens[t].value; break; }
            }
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
                // (blk_end-blk: a constant; 2*blk: an error)
                if (ti >= 0 && parser.isRelocatableSymbolIn(arg, stmt->scopePrefix) &&
                    !relocatedAs(stmt.get(), ti, targetSeg)) { patchAddr += 2; continue; }
            }

            O45Reloc reloc;
            reloc.offset = patchAddr - srcBase;
            reloc.type = R_WORD;

            if (isExtern) {
                reloc.segment = SEG_EXTERNAL;
                reloc.symbolIndex = syms.getImportIndex(arg);
            } else {
                reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
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
        for (size_t spi = 0; spi < allSpBaseRelocs.size(); spi++) {
            const auto& sr = allSpBaseRelocs[spi];
            // The segment this site was emitted in
            for (const auto& segName : {spBaseRelocSeg[spi]}) {
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
    } else if (!allSpBaseRelocs.empty()) {
        std::cerr << "warning: stack-relative access uses built-in __sp_base ($"
                  << std::hex << std::uppercase << std::setfill('0') << std::setw(4)
                  << parser.getSpBase()
                  << "); add '.extern __sp_base' for relocatable code" << std::endl;
    }

    // Scan symbol relocation sites from simulated ops (ldax/stax of global variables).
    for (size_t sri = 0; sri < allSymbolRelocs.size(); sri++) {
        const auto& sr = allSymbolRelocs[sri];
        std::string symName = sr.symbolName;
        // resolved in the emitting statement's scope (an @label in a proc)
        parser.currentLocalScope_ = sr.localScope;
        Symbol* sym = parser.resolveSymbol(symName, sr.scope);
        if (!sym) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
        }

        // The source segment it was emitted in
        for (const auto& segName : {symbolRelocSeg[sri]}) {
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
                    reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
                }
                segRelocs[segName].push_back(reloc);
                break;
            }
        }
    }

    // Phase 78: Process immediate relocations for SMC parameters
    for (size_t iri = 0; iri < allImmediateRelocs.size(); iri++) {
        const auto& ir = allImmediateRelocs[iri];
        std::string symName = ir.symbolName;
        Symbol* sym = parser.resolveSymbol(symName, "");
        if (!sym) continue;

        bool isExtern = parser.isExternSymbol(symName);
        std::string targetSeg;
        if (!isExtern) {
            targetSeg = sym->segment;
            if (targetSeg.empty()) continue;
        }

        // The source segment it was emitted in
        for (const auto& segName : {immediateRelocSeg[iri]}) {
            auto seg = parser.segments[segName];
            uint32_t segStart = (seg->startAddress != 0xFFFFFFFF) ? seg->startAddress : 0;
            if (ir.address >= segStart && ir.address < seg->pc) {
                O45Reloc reloc;
                reloc.offset = ir.address - segStart;
                reloc.type = (O45RelocType)ir.relocType;  // R_IMM8 or R_IMM16
                reloc.extra = 0;
                if (isExtern) {
                    reloc.segment = SEG_EXTERNAL;
                    reloc.symbolIndex = syms.getImportIndex(symName);
                } else {
                    reloc.segment = segIdFromName(targetSeg); reloc.baseSeg = targetSeg; reloc.baseAddr = sym->value;
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
        // The same site can be found by more than one scan (e.g. a .word
        // directive and the emitter's symbol reloc): keep one entry per offset
        relocs.erase(std::unique(relocs.begin(), relocs.end(),
                                 [](const O45Reloc& a, const O45Reloc& b) { return a.offset == b.offset; }),
                     relocs.end());
    }

    // --- Step 3b: link-time symbols ---
    // A symbol resolved at link time (.extern) can only be used in a form a
    // relocation holds (sym, sym +/- n, <sym, >sym), and not where its value
    // is needed while assembling (sizes, alignment, assignments): either is an
    // error, as is such a use that got no relocation. A weak absolute symbol
    // defined here (.weak x / x = value) is used at its value here: those the
    // code uses are recorded (OPT_WEAKUSE), and ln45 refuses a program that
    // gives one another value.
    std::map<std::string, uint32_t> weakUsedValue;
    {
        std::vector<std::string> errs;
        auto relocAt = [&](const std::string& seg, uint32_t off) {
            auto it = segRelocs.find(seg);
            if (it == segRelocs.end()) return false;
            for (const auto& r : it->second) if (r.offset == off) return true;
            return false;
        };
        auto segBase = [&](const std::string& seg) {
            uint32_t b = globalBase;
            if (parser.segments.count(seg)) {
                b = parser.segments.at(seg)->startAddress;
                if (b == 0xFFFFFFFF) b = 0;
            }
            return b;
        };
        std::set<std::string> weakUsed;
        for (const auto& stmt : parser.statements) {
            if (stmt->deleted) continue;
            auto err = [&](const std::string& m) {
                errs.push_back(formatDiagnostic(stmt->sourceFile, stmt->line, 1, Severity::Error, m));
            };
            auto badForm = [&](const std::string& n) {
                err("'" + n + "' is resolved at link time: only " + n + ", " + n + " + n, " + n + " - n, <" + n +
                    " and >" + n + " can be relocated");
            };
            std::string n;
            if (stmt->type == AssemblerParser::Statement::INSTRUCTION) {
                if (stmt->instr.operandTokenIndex < 0 || stmt->size == 0) continue;
                int k = parser.linkTimeUse(stmt->instr.operandTokenIndex, stmt->scopePrefix, n, &weakUsed);
                if (k == 2) badForm(n);
                else if (k == 1 && stmt->instr.mode != AddressingMode::RELATIVE &&
                         stmt->instr.mode != AddressingMode::RELATIVE16 &&
                         !relocAt(stmt->segmentName, stmt->address + operandOffset(stmt.get()) - segBase(stmt->segmentName)))
                    err("no relocation could be made for '" + n + "' in this operand (write it as " + n + ", " + n +
                        " + n, <" + n + " or >" + n + ")");
            } else if (stmt->type == AssemblerParser::Statement::DIRECTIVE) {
                const auto& d = stmt->dir;
                int width = d.name == "byte" ? 1 : d.name == "word" ? 2 : (d.name == "long" || d.name == "dword") ? 4 : 0;
                if (width) {
                    for (size_t i = 0; i < d.argTokenIndices.size(); i++) {
                        n.clear();
                        int k = parser.linkTimeUse(d.argTokenIndices[i], stmt->scopePrefix, n, &weakUsed);
                        if (k == 2) badForm(n);
                        else if (k == 1 && !relocAt(stmt->segmentName, stmt->address + (uint32_t)(i * width) - segBase(stmt->segmentName)))
                            err("no relocation could be made for '" + n + "' in this ." + d.name + " (write it as " + n +
                                ", " + n + " + n, <" + n + " or >" + n + ")");
                    }
                } else if (d.name == "res" || d.name == "fill" || d.name == "space" || d.name == "align" || d.name == "org") {
                    std::vector<int> at;
                    if (d.tokenIndex >= 0) at.push_back(d.tokenIndex);
                    if (d.valueTokenIndex >= 0) at.push_back(d.valueTokenIndex);
                    for (int t : at) {
                        n.clear();
                        if (parser.linkTimeUse(t, stmt->scopePrefix, n, &weakUsed)) {
                            err("." + d.name + " uses '" + n + "', which is resolved at link time: its value is not known here");
                            break;
                        }
                    }
                }
            }
        }
        if (!errs.empty()) {
            std::string all;
            for (const auto& e : errs) all += (all.empty() ? "" : "\n") + e;
            throw std::runtime_error(all);
        }
        for (const auto& w : weakUsed) {
            auto* s = parser.resolveSymbol(w);
            if (s) weakUsedValue[w] = s->value;
        }
    }

    // --- Step 4: Package into .o45 ---
    O45Writer writer;
    writer.addDefaultOptions(asmVersion);

    // OPT_WEAKUSE: name (NUL-terminated) and value (4 bytes LE) of each weak
    // absolute symbol the code was assembled with
    {
        std::vector<uint8_t> payload;
        auto flush = [&]() { if (!payload.empty()) { writer.addOptionRaw(OPT_WEAKUSE, payload); payload.clear(); } };
        for (const auto& [w, v] : weakUsedValue) {
            if (payload.size() + w.size() + 5 > O45_OPT_PAYLOAD_MAX) flush();
            for (char c : w) payload.push_back((uint8_t)c);
            payload.push_back(0);
            for (int i = 0; i < 4; i++) payload.push_back((uint8_t)(v >> (8 * i)));
        }
        flush();
    }

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

        // An internal relocation into a later text sub-segment ("far": the
        // __far functions, "init") holds the target's address in that
        // segment: make it the address in the merged text, which the linker
        // reads as an offset from the text's base (stored value - tbase)
        if (textSegNames.size() > 1) {
            auto shift = [&](std::vector<uint8_t>& body, O45Reloc& r) {
                if (r.segment != SEG_TEXT || r.baseSeg.empty()) return;
                auto it = textSubSegOffset.find(r.baseSeg);
                if (it == textSubSegOffset.end() || !parser.segments.count(r.baseSeg)) return;
                uint32_t sbase = parser.segments.at(r.baseSeg)->startAddress;
                if (sbase == 0xFFFFFFFF) sbase = 0;
                int32_t delta = (int32_t)(it->second + textBase) - (int32_t)sbase;
                if (!delta || r.offset >= body.size()) return;
                switch (r.type) {
                    case R_WORD:
                        if (r.offset + 1 < body.size()) {
                            uint16_t w = (uint16_t)((body[r.offset] | (body[r.offset + 1] << 8)) + delta);
                            body[r.offset] = (uint8_t)w; body[r.offset + 1] = (uint8_t)(w >> 8);
                        }
                        break;
                    case R_HIGH: {   // the high byte; extra: the low byte
                        uint16_t w = (uint16_t)(((body[r.offset] << 8) | r.extra) + delta);
                        body[r.offset] = (uint8_t)(w >> 8); r.extra = (uint8_t)w;
                        break;
                    }
                    case R_LOWX: {   // the low byte; extra: the high byte
                        uint16_t w = (uint16_t)(((r.extra << 8) | body[r.offset]) + delta);
                        body[r.offset] = (uint8_t)w; r.extra = (uint8_t)(w >> 8);
                        break;
                    }
                    case R_LOW:
                        body[r.offset] = (uint8_t)(body[r.offset] + delta);
                        break;
                    default: break;
                }
            };
            for (auto& r : mergedTextRelocs) shift(mergedTextBody, r);
            auto dr = segRelocs.find("data");
            if (dr != segRelocs.end() && segBodies.count("data"))
                for (auto& r : dr->second) shift(segBodies["data"].bytes, r);
        }

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

    // --- Step 3b: section table (.function_sections / ca45 -ffunction-sections) ---
    // A section per proc ... endproc in a text segment, and per .split
    // (ended by the next .split/.nosplit of its segment or the segment end):
    // ln45 keeps or drops each as a whole. Sections tied together without a
    // relocation -- a relative branch between them, code falling through from
    // one into the next, a label difference -- are merged; a section tied to
    // bytes outside any section (the residual) becomes residual, and so does
    // a group spanning segments.
    if (parser.functionSections) {
        struct Sec { std::string seg; uint32_t start, end; std::string name; bool keep; uint8_t align; };
        std::vector<Sec> secs;
        std::map<std::string, int> open;           // segment -> its open section
        std::map<std::string, uint32_t> pendingAlign;  // .align right before a proc
        auto closeAt = [&](const std::string& seg, uint32_t addr) {
            auto it = open.find(seg);
            if (it == open.end()) return;
            secs[it->second].end = std::max(addr, secs[it->second].start);
            open.erase(it);
        };
        auto openSec = [&](const std::string& seg, uint32_t addr, const std::string& name, uint8_t align) {
            closeAt(seg, addr);
            secs.push_back({seg, addr, addr, name, false, align});
            open[seg] = (int)secs.size() - 1;
        };
        std::set<int> procSecs;
        for (const auto& stmt : parser.statements) {
            if (stmt->deleted) continue;
            const std::string& seg = stmt->segmentName;
            if (!parser.segments.count(seg) || parser.segments.at(seg)->startAddress == 0xFFFFFFFF) continue;
            if (stmt->type == AssemblerParser::Statement::INSTRUCTION && stmt->instr.mnemonic == "proc" &&
                segIdFromName(seg) == SEG_TEXT) {
                // the proc's name (the parser keeps it as the statement's label)
                std::string name = stmt->label;
                if (!name.empty() && (unsigned char)name[0] < ' ') name.clear();   // an unnamed proc
                uint8_t al = 0;
                auto pa = pendingAlign.find(seg);
                if (pa != pendingAlign.end()) { while ((1u << al) < pa->second && al < 15) al++; }
                openSec(seg, stmt->address, name, al);
                procSecs.insert(open[seg]);
                continue;
            }
            if (stmt->type == AssemblerParser::Statement::INSTRUCTION && stmt->instr.mnemonic == "endproc") {
                auto it = open.find(seg);
                if (it != open.end() && procSecs.count(it->second)) closeAt(seg, stmt->address + stmt->size);
                pendingAlign.erase(seg);
                continue;
            }
            if (stmt->type == AssemblerParser::Statement::DIRECTIVE) {
                const std::string& d = stmt->dir.name;
                if (d == "split") {
                    uint8_t al = 0;
                    auto pa = pendingAlign.find(seg);
                    if (pa != pendingAlign.end()) { while ((1u << al) < pa->second && al < 15) al++; }
                    pendingAlign.erase(seg);
                    openSec(seg, stmt->address, stmt->dir.arguments.empty() ? std::string() : stmt->dir.arguments[0], al);
                    continue;
                }
                if (d == "nosplit") { closeAt(seg, stmt->address); continue; }
                if (d == "keep") { auto it = open.find(seg); if (it != open.end()) secs[it->second].keep = true; continue; }
                if ((d == "align" || d == "balign") && !stmt->dir.arguments.empty()) {
                    try { pendingAlign[seg] = (uint32_t)parseNumericLiteral(stmt->dir.arguments[0]); } catch (...) {}
                    continue;
                }
            }
            if (stmt->size > 0) pendingAlign.erase(seg);
        }
        for (auto& [seg, idx] : std::map<std::string, int>(open))
            closeAt(seg, parser.segments.at(seg)->pc);
        // drop empty sections
        secs.erase(std::remove_if(secs.begin(), secs.end(), [](const Sec& s) { return s.end <= s.start; }), secs.end());

        // the section holding an address of a segment (-1: residual)
        auto find = [&](const std::string& seg, int64_t addr) -> int {
            for (size_t i = 0; i < secs.size(); i++)
                if (secs[i].seg == seg && addr >= secs[i].start && addr < secs[i].end) return (int)i;
            return -1;
        };
        // union-find over the sections plus one residual node per segment
        std::vector<int> parent(secs.size());
        for (size_t i = 0; i < parent.size(); i++) parent[i] = (int)i;
        std::map<std::string, int> residualNode;
        auto node = [&](const std::string& seg, int sec) -> int {
            if (sec >= 0) return sec;
            auto it = residualNode.find(seg);
            if (it != residualNode.end()) return it->second;
            parent.push_back((int)parent.size());
            return residualNode[seg] = (int)parent.size() - 1;
        };
        std::function<int(int)> root = [&](int x) { return parent[x] == x ? x : parent[x] = root(parent[x]); };
        auto unite = [&](int a, int b) { a = root(a); b = root(b); if (a != b) parent[a] = b; };

        // endproc assembles to rts
        static const std::set<std::string> terminal = {"rts", "rti", "jmp", "bra", "rtn", "endproc"};
        static const std::set<std::string> branchOps = {
            "beq", "bne", "bra", "bcc", "bcs", "bpl", "bmi", "bvc", "bvs", "bsr",
            "bbr0", "bbr1", "bbr2", "bbr3", "bbr4", "bbr5", "bbr6", "bbr7",
            "bbs0", "bbs1", "bbs2", "bbs3", "bbs4", "bbs5", "bbs6", "bbs7"};
        for (const auto& stmt : parser.statements) {
            if (stmt->deleted || stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;
            const std::string& seg = stmt->segmentName;
            if (!parser.segments.count(seg)) continue;
            // a relative branch (8 or 16 bits): its target's section stays with its own
            if (branchOps.count(stmt->instr.mnemonic)) {
                bool bitBranch = stmt->instr.mnemonic[1] == 'b';
                parser.currentLocalScope_ = stmt->localLabelScope;
                parser.pc = stmt->address;   // for * in the operand
                int64_t target = -1;
                try {
                    if (bitBranch) {
                        Symbol* t = parser.resolveSymbol(stmt->instr.bitBranchTarget, stmt->scopePrefix);
                        if (t) target = t->value;
                    } else if (stmt->instr.operandTokenIndex >= 0) {
                        target = parser.evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                    } else if (!stmt->instr.operand.empty()) {
                        // a branch the optimizer inserted (a BSR to a SeqExtract
                        // routine): no source tokens, the operand is a label
                        Symbol* t = parser.resolveSymbol(stmt->instr.operand, stmt->scopePrefix);
                        if (t) target = t->value;
                    }
                } catch (...) {}
                int from = find(seg, stmt->address);
                if (target < 0) {   // unknown: keep with the object
                    if (getenv("CA45_DEBUG_SECTIONS")) std::cerr << "branch unknown " << stmt->instr.operand << " at " << stmt->address << std::endl;
                    unite(node(seg, from), node(seg, -1)); continue; }
                int to = find(seg, target);
                if (from != to && getenv("CA45_DEBUG_SECTIONS"))
                    std::cerr << "branch " << stmt->instr.mnemonic << " " << stmt->instr.operand << " at " << stmt->address
                              << " sec " << from << " -> " << target << " sec " << to << std::endl;
                if (from != to) unite(node(seg, from), node(seg, to));
            }
        }
        // falling through: a text section whose last instruction can continue
        for (size_t i = 0; i < secs.size(); i++) {
            if (segIdFromName(secs[i].seg) != SEG_TEXT) continue;
            const AssemblerParser::Statement* last = nullptr;
            for (const auto& stmt : parser.statements)
                if (!stmt->deleted && stmt->size > 0 && stmt->segmentName == secs[i].seg &&
                    stmt->address >= secs[i].start && stmt->address < secs[i].end)
                    last = stmt.get();
            bool ends = last && last->type == AssemblerParser::Statement::INSTRUCTION &&
                        terminal.count(last->instr.mnemonic);
            uint32_t segEnd = parser.segments.at(secs[i].seg)->pc;
            if (!ends && secs[i].end < segEnd) unite(node(secs[i].seg, (int)i), node(secs[i].seg, find(secs[i].seg, secs[i].end)));
            // residual code right before the section that can run on into it
            const AssemblerParser::Statement* prev = nullptr;
            for (const auto& stmt : parser.statements)
                if (!stmt->deleted && stmt->size > 0 && stmt->segmentName == secs[i].seg &&
                    stmt->address < secs[i].start && (!prev || stmt->address >= prev->address))
                    prev = stmt.get();
            // (an instruction: alignment padding and data do not run)
            if (prev && find(secs[i].seg, prev->address) < 0 &&
                prev->type == AssemblerParser::Statement::INSTRUCTION && !terminal.count(prev->instr.mnemonic))
                unite(node(secs[i].seg, (int)i), node(secs[i].seg, -1));
        }
        // label differences (a - b): both labels' sections stay together
        auto unitePair = [&](const std::vector<std::string>& names, const std::string& scope) {
            std::vector<int> ns;
            for (const auto& n : names) {
                if (!parser.isRelocatableSymbolIn(n, scope)) continue;
                Symbol* s = parser.resolveSymbol(n, scope);
                if (!s || s->segment.empty() || s->segment == "__extern__") continue;
                ns.push_back(node(s->segment, find(s->segment, s->value)));
            }
            for (size_t k = 1; k < ns.size(); k++) unite(ns[0], ns[k]);
        };
        auto identifiers = [](const std::string& e) {
            std::vector<std::string> out;
            for (size_t i = 0; i < e.size();) {
                if (std::isalpha((unsigned char)e[i]) || e[i] == '_' || e[i] == '@' || e[i] == '.') {
                    size_t j = i;
                    while (j < e.size() && (std::isalnum((unsigned char)e[j]) || e[j] == '_' || e[j] == '@' || e[j] == '.' || e[j] == ':')) j++;
                    out.push_back(e.substr(i, j - i));
                    i = j;
                } else if (e[i] == '$' || std::isdigit((unsigned char)e[i])) {
                    i++;   // (past the '$' of a hex number too)
                    while (i < e.size() && std::isalnum((unsigned char)e[i])) i++;
                } else i++;
            }
            return out;
        };
        for (const auto& stmt : parser.statements) {
            if (stmt->deleted) continue;
            parser.currentLocalScope_ = stmt->localLabelScope;
            parser.pc = stmt->address;
            if (stmt->type == AssemblerParser::Statement::DIRECTIVE) {
                for (const auto& a : stmt->dir.arguments)
                    if (a.find('-') != std::string::npos) unitePair(identifiers(a), stmt->scopePrefix);
            } else if (stmt->type == AssemblerParser::Statement::INSTRUCTION && stmt->instr.operand.find('-') != std::string::npos) {
                unitePair(identifiers(stmt->instr.operand), stmt->scopePrefix);
            }
        }

        // groups -> final sections: a group with a residual node or more than
        // one segment becomes residual; otherwise its span (absorbing any
        // section inside it) is one section
        std::map<int, std::vector<int>> groups;
        for (size_t i = 0; i < secs.size(); i++) groups[root((int)i)].push_back((int)i);
        std::set<int> residualRoots;
        for (const auto& [seg, n] : residualNode) residualRoots.insert(root(n));
        std::vector<Sec> merged;
        std::vector<bool> isResidual(secs.size(), false);
        for (const auto& [r, members] : groups) {
            bool resid = residualRoots.count(r) > 0;
            for (int i : members) if (secs[i].seg != secs[members[0]].seg) resid = true;
            if (resid) { for (int i : members) isResidual[i] = true; continue; }
            Sec g = secs[members[0]];
            for (int i : members) {
                g.start = std::min(g.start, secs[i].start);
                g.end = std::max(g.end, secs[i].end);
                g.keep = g.keep || secs[i].keep;
                if (secs[i].start <= g.start) { g.name = secs[i].name; g.align = secs[i].align; }
            }
            merged.push_back(g);
        }
        // spans that overlap (a group around another one's section) merge too
        std::sort(merged.begin(), merged.end(), [](const Sec& a, const Sec& b) {
            return a.seg != b.seg ? a.seg < b.seg : a.start < b.start; });
        std::vector<Sec> finalSecs;
        for (const auto& g : merged) {
            if (!finalSecs.empty() && finalSecs.back().seg == g.seg && g.start < finalSecs.back().end) {
                finalSecs.back().end = std::max(finalSecs.back().end, g.end);
                finalSecs.back().keep = finalSecs.back().keep || g.keep;
            } else finalSecs.push_back(g);
        }
        // a span may now cover a section that became residual: such a span is residual too
        finalSecs.erase(std::remove_if(finalSecs.begin(), finalSecs.end(), [&](const Sec& g) {
            for (size_t i = 0; i < secs.size(); i++)
                if (isResidual[i] && secs[i].seg == g.seg && secs[i].start < g.end && secs[i].end > g.start) return true;
            return false; }), finalSecs.end());

        // table entries: offsets within the .o45 segment bodies
        auto segOffset = [&](const std::string& seg) -> int64_t {
            uint32_t base = parser.segments.at(seg)->startAddress;
            if (segIdFromName(seg) == SEG_TEXT) {
                auto it = textSubSegOffset.find(seg);
                if (it == textSubSegOffset.end()) return -1;
                return (int64_t)it->second - base;
            }
            return -(int64_t)base;
        };
        std::vector<O45Section> table;
        std::vector<std::pair<std::string, std::pair<uint32_t, uint32_t>>> tableSpan;   // seg, [start, end) addresses
        for (int pass = 0; pass < 4; pass++) {
            O45Segment want = pass == 0 ? SEG_TEXT : pass == 1 ? SEG_DATA : pass == 2 ? SEG_BSS : SEG_ZP;
            std::vector<std::pair<int64_t, size_t>> order;
            for (size_t i = 0; i < finalSecs.size(); i++) {
                if (segIdFromName(finalSecs[i].seg) != want) continue;
                int64_t so = segOffset(finalSecs[i].seg);
                if (so == -1 && want == SEG_TEXT) continue;
                order.push_back({so + finalSecs[i].start, i});
            }
            std::sort(order.begin(), order.end());
            for (const auto& [off, i] : order) {
                O45Section sc;
                sc.segId = (uint8_t)want;
                sc.keep = finalSecs[i].keep;
                sc.offset = (uint32_t)off;
                sc.length = finalSecs[i].end - finalSecs[i].start;
                sc.alignLog2 = finalSecs[i].align;
                sc.name = finalSecs[i].name;
                table.push_back(sc);
                tableSpan.push_back({finalSecs[i].seg, {finalSecs[i].start, finalSecs[i].end}});
            }
        }
        auto tableIndex = [&](const std::string& seg, int64_t addr) -> int {
            for (size_t i = 0; i < tableSpan.size(); i++)
                if (tableSpan[i].first == seg && addr >= tableSpan[i].second.first && addr < tableSpan[i].second.second)
                    return (int)i;
            return -1;
        };
        // references: an internal relocation whose stored value does not
        // name its target's section (a low byte, one past the end, ...)
        std::vector<O45SectRef> refs;
        for (const auto& [srcSeg, relocs] : segRelocs) {
            bool text = segIdFromName(srcSeg) == SEG_TEXT;
            if (!text && srcSeg != "data") continue;
            int64_t srcOff = text ? segOffset(srcSeg) + (int64_t)parser.segments.at(srcSeg)->startAddress : 0;
            if (text && srcOff < 0) continue;
            const auto& body = segBodies[srcSeg].bytes;
            for (const auto& r : relocs) {
                if (r.segment == SEG_EXTERNAL || r.baseSeg.empty() || r.baseAddr < 0) continue;
                bool targetSectioned = false;
                for (const auto& sp : tableSpan) if (sp.first == r.baseSeg) { targetSectioned = true; break; }
                if (!targetSectioned) continue;   // all residual: nothing to tell apart
                int want = tableIndex(r.baseSeg, r.baseAddr);
                int64_t value = -1;
                if (r.type == R_WORD && r.offset + 1 < body.size())
                    value = body[r.offset] | (body[r.offset + 1] << 8);
                else if (r.type == R_HIGH && r.offset < body.size())
                    value = (body[r.offset] << 8) | r.extra;
                else if (r.type == R_LOWX && r.offset < body.size())
                    value = body[r.offset] | (r.extra << 8);
                int byValue = value >= 0 ? tableIndex(r.baseSeg, value) : -2;
                if (byValue == want) continue;
                O45SectRef ref;
                ref.kind = text ? O45_SECTREF_TEXT_RELOC : O45_SECTREF_DATA_RELOC;
                ref.key = (uint32_t)(r.offset + (text ? srcOff : 0));
                ref.section = want < 0 ? O45_SECT_RESIDUAL : (uint16_t)want;
                refs.push_back(ref);
            }
        }
        // .ref: the section needs an import (no relocation)
        for (const auto& stmt : parser.statements) {
            if (stmt->deleted || stmt->type != AssemblerParser::Statement::DIRECTIVE || stmt->dir.name != "ref") continue;
            int sec = tableIndex(stmt->segmentName, stmt->address);
            for (const auto& name : stmt->dir.arguments) {
                if (!syms.isImported(name)) continue;   // defined here: nothing to pull
                O45SectRef ref;
                ref.kind = O45_SECTREF_IMPORT;
                ref.key = syms.getImportIndex(name);
                ref.section = sec < 0 ? O45_SECT_RESIDUAL : (uint16_t)sec;
                refs.push_back(ref);
            }
        }
        if (!table.empty()) {
            writer.addSections(table);
            writer.addSectRefs(refs);
        }
    }

    // Apply symbols
    syms.applyTo(writer);

    // Emit debug line info option (OPT_LINEINFO)
    // (only lines of code: a .loc before data or BSS -- a function's static
    // storage -- has no place in the text)
    std::vector<AssemblerParser::LineEntry> lineMap;
    for (const auto& e : parser.getLineMap())
        if (e.segment.empty() || segIdFromName(e.segment) == SEG_TEXT) lineMap.push_back(e);
    // a line's offset in the merged text: its segment's place there (a later
    // text sub-segment: "far", __far functions)
    auto lineOff = [&](const AssemblerParser::LineEntry& e) -> uint16_t {
        auto it = textSubSegOffset.find(e.segment);
        if (it != textSubSegOffset.end() && parser.segments.count(e.segment)) {
            uint32_t sbase = parser.segments.at(e.segment)->startAddress;
            if (sbase == 0xFFFFFFFF) sbase = 0;
            return (uint16_t)(e.address - sbase + it->second);
        }
        return (uint16_t)(e.address - genStart);
    };
    if (!lineMap.empty()) {
        // Build file string table (dedup filenames) with safe path shortening and length limits
        std::vector<std::string> fileTable;
        std::map<std::string, uint16_t> fileIndex;
        size_t currentFileTableBytes = 0;
        for (const auto& entry : lineMap) {
            std::string shortFile = entry.file;
            // Shorten path
            std::string prefix = "/home/duck/m65/in_devel/ccomp/";
            if (shortFile.rfind(prefix, 0) == 0) {
                shortFile = shortFile.substr(prefix.length());
            }
            size_t idx;
            while ((idx = shortFile.find("bin/../")) != std::string::npos) {
                shortFile.replace(idx, 7, "");
            }
            
            if (!fileIndex.count(entry.file)) {
                // Limit file table size to prevent option payload overflow (>253 bytes)
                if (currentFileTableBytes + shortFile.length() + 1 > 220) {
                    // Fall back to basename
                    size_t lastSlash = shortFile.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        shortFile = shortFile.substr(lastSlash + 1);
                    }
                }
                
                if (currentFileTableBytes + shortFile.length() + 1 > 220 && !fileTable.empty()) {
                    // Drop/map to first file to prevent overflow
                    fileIndex[entry.file] = 0;
                } else {
                    fileIndex[entry.file] = (uint16_t)fileTable.size();
                    fileTable.push_back(shortFile);
                    currentFileTableBytes += shortFile.length() + 1;
                }
            }
        }
        // First option: file string table
        // Payload: fileCount(2) + NUL-terminated strings
        {
            std::vector<uint8_t> payload;
            uint16_t fc = (uint16_t)fileTable.size();
            payload.push_back((uint8_t)(fc & 0xFF));
            payload.push_back((uint8_t)(fc >> 8));
            for (const auto& f : fileTable) {
                for (char c : f) payload.push_back((uint8_t)c);
                payload.push_back(0x00);
            }
            // Followed by entryCount(2) and as many 6-byte entries as fit
            // Max payload per option: 253 bytes (255 - len - type)
            uint16_t totalEntries = (uint16_t)lineMap.size();
            payload.push_back((uint8_t)(totalEntries & 0xFF));
            payload.push_back((uint8_t)(totalEntries >> 8));
            size_t ei = 0;
            while (payload.size() + 6 <= 253 && ei < lineMap.size()) {
                uint16_t off = lineOff(lineMap[ei]);
                uint16_t fi = fileIndex[lineMap[ei].file];
                uint16_t ln = (uint16_t)lineMap[ei].line;
                payload.push_back((uint8_t)(off & 0xFF)); payload.push_back((uint8_t)(off >> 8));
                payload.push_back((uint8_t)(fi & 0xFF)); payload.push_back((uint8_t)(fi >> 8));
                payload.push_back((uint8_t)(ln & 0xFF)); payload.push_back((uint8_t)(ln >> 8));
                ei++;
            }
            writer.addOptionRaw(OPT_LINEINFO, payload);

            // Continuation options: just 6-byte entries, no header
            // Use OPT_LINEINFO + 1 (0x12) as continuation marker
            while (ei < lineMap.size()) {
                std::vector<uint8_t> cont;
                while (cont.size() + 6 <= 253 && ei < lineMap.size()) {
                    uint16_t off = lineOff(lineMap[ei]);
                    uint16_t fi = fileIndex[lineMap[ei].file];
                    uint16_t ln = (uint16_t)lineMap[ei].line;
                    cont.push_back((uint8_t)(off & 0xFF)); cont.push_back((uint8_t)(off >> 8));
                    cont.push_back((uint8_t)(fi & 0xFF)); cont.push_back((uint8_t)(fi >> 8));
                    cont.push_back((uint8_t)(ln & 0xFF)); cont.push_back((uint8_t)(ln >> 8));
                    ei++;
                }
                writer.addOptionRaw(OPT_LINEINFO, cont);
            }
        }
    }

    // Emit debug symbols option (OPT_DEBUG_SYMBOLS)
    const auto& procedures = parser.getProcedures();
    if (!procedures.empty()) {
        std::vector<uint8_t> payload;

        // Count total procedures (as header)
        uint16_t procCount = (uint16_t)procedures.size();
        payload.push_back((uint8_t)(procCount & 0xFF));
        payload.push_back((uint8_t)(procCount >> 8));

        // Emit each procedure's debug info: name, parameter count, then
        // (name, 16-bit offset) per parameter, ended by $FF. Records are
        // kept under the 253-byte option limit; a procedure with more
        // parameters than fit is split into several records of the same name
        auto flush = [&]() {
            if (payload.size() > 2) writer.addOptionRaw(OPT_DEBUG_SYMBOLS, payload);
            payload.clear();
            payload.push_back((uint8_t)(procCount & 0xFF));
            payload.push_back((uint8_t)(procCount >> 8));
        };
        for (const auto& [addr, proc] : procedures) {
            std::vector<std::pair<std::string, int>> args(proc->localArgs.begin(), proc->localArgs.end());
            size_t next = 0;
            do {
                // How many parameters fit in one record
                size_t room = 240 - std::min<size_t>(240, proc->name.length() + 3);
                size_t take = 0, used = 0;
                while (next + take < args.size() && used + args[next + take].first.length() + 3 <= room) {
                    used += args[next + take].first.length() + 3;
                    take++;
                }
                if (take == 0 && next < args.size()) take = 1;  // a name too long to share a record
                size_t recSize = proc->name.length() + 3 + used;
                if (payload.size() > 2 && payload.size() + recSize > 240) flush();

                for (char c : proc->name) payload.push_back((uint8_t)c);
                payload.push_back(0x00);
                payload.push_back((uint8_t)take);
                for (size_t k = next; k < next + take; k++) {
                    for (char c : args[k].first) payload.push_back((uint8_t)c);
                    payload.push_back(0x00);
                    int16_t signedOffset = (int16_t)args[k].second;
                    payload.push_back((uint8_t)(signedOffset & 0xFF));
                    payload.push_back((uint8_t)((signedOffset >> 8) & 0xFF));
                }
                payload.push_back(0xFF);
                next += take;
            } while (next < args.size());
        }

        // Emit final payload
        if (payload.size() > 2) {  // Only emit if we have more than just the count header
            writer.addOptionRaw(OPT_DEBUG_SYMBOLS, payload);
        }
    }

    // Debug information (#202): cc45's .dbg_type / .dbg_var as NUL-terminated
    // text entries, packed into records under the option size limit. A
    // variable's sym:NAME[+n] location becomes seg:SEGMENT+offset (in this
    // object's segment body), imp:NAME+n (an import) or abs:$XXXX (an
    // equate); a symbol that is not there (an optimised-out local's
    // storage) leaves it none
    {
        auto pack = [&](uint8_t opt, const std::vector<std::string>& entries) {
            std::vector<uint8_t> payload;
            for (const auto& e : entries) {
                if (!payload.empty() && payload.size() + e.size() + 1 > 240) {
                    writer.addOptionRaw(opt, payload);
                    payload.clear();
                }
                payload.insert(payload.end(), e.begin(), e.end());
                payload.push_back(0x00);
            }
            if (!payload.empty()) writer.addOptionRaw(opt, payload);
        };
        auto location = [&](const std::string& loc) -> std::string {
            if (loc.rfind("sym:", 0) != 0) return loc;
            std::string name = loc.substr(4);
            long add = 0;
            size_t plus = name.find('+');
            if (plus != std::string::npos) { add = std::stol(name.substr(plus + 1)); name.resize(plus); }
            std::string tail = add ? "+" + std::to_string(add) : "";
            if (parser.isExternSymbol(name)) return "imp:" + name + tail;
            Symbol* sym = parser.resolveSymbol(name, "");
            if (!sym) return "none";
            char buf[16];
            if (!sym->isAddress || sym->segment.empty() || !parser.segments.count(sym->segment)) {
                snprintf(buf, sizeof buf, "$%04X", (unsigned)((sym->value + add) & 0xFFFF));
                return std::string("abs:") + buf;
            }
            uint32_t base = parser.segments.at(sym->segment)->startAddress;
            if (base == 0xFFFFFFFF) base = 0;
            uint32_t off = sym->value - base + (uint32_t)add;
            O45Segment seg = segIdFromName(sym->segment);
            if (seg == SEG_TEXT) {
                auto t = textSubSegOffset.find(sym->segment);
                if (t != textSubSegOffset.end()) off += t->second;
            }
            static const char* names[] = {"", "", "text", "data", "bss", "zp"};
            return std::string("seg:") + (seg >= SEG_TEXT && seg <= SEG_ZP ? names[seg] : "text") + "+" +
                   std::to_string(off);
        };
        std::vector<std::string> types, vars;
        for (const auto& stmt : parser.statements) {
            if (stmt->type != AssemblerParser::Statement::DIRECTIVE || stmt->dir.arguments.empty()) continue;
            const std::string& text = stmt->dir.arguments[0];
            if (stmt->dir.name == "dbg_type") {
                // a long layout goes on in "+ NAME members..." entries
                std::istringstream ws(text);
                std::string kind, name, size, m, cur;
                ws >> kind >> name >> size;
                cur = kind + " " + name + " " + size;
                while (ws >> m) {
                    if (cur.size() + 1 + m.size() > 230) { types.push_back(cur); cur = "+ " + name; }
                    cur += " " + m;
                }
                types.push_back(cur);
            } else if (stmt->dir.name == "dbg_var") {
                // kind scope name type location
                size_t sp = text.rfind(' ');
                if (sp == std::string::npos) continue;
                vars.push_back(text.substr(0, sp + 1) + location(text.substr(sp + 1)));
            }
        }
        // each named procedure's code: "func LABEL seg:text+OFFSET LENGTH"
        // (static functions too, which have no export), when the object has
        // debug information
        if (!vars.empty() || !types.empty()) {
            std::vector<std::pair<const AssemblerParser::Statement*, std::string>> open;
            for (const auto& stmt : parser.statements) {
                if (stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;
                if (stmt->instr.mnemonic == "proc") {
                    open.push_back({stmt.get(), stmt->procCtx && !stmt->procCtx->unnamed ? stmt->procCtx->name : ""});
                } else if (stmt->instr.mnemonic == "endproc" && !open.empty()) {
                    auto [start, name] = open.back();
                    open.pop_back();
                    if (name.empty() || start->segmentName != stmt->segmentName) continue;
                    std::string loc = location("sym:" + name);
                    if (loc.rfind("seg:text+", 0) != 0) continue;
                    uint32_t len = stmt->address + stmt->size - start->address;
                    vars.push_back("func " + name + " " + loc + " " + std::to_string(len));
                }
            }
        }
        pack(OPT_DEBUG_TYPES, types);
        pack(OPT_DEBUG_VARS, vars);
    }

    return writer.emit();
}
