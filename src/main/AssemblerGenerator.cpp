#include "AssemblerLexer.hpp"
#include "AssemblerGenerator.hpp"
#include "AssemblerParser.hpp"
#include "M65Emitter.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "CBMFloat.hpp"
#include "AssemblerSimulatedOps.hpp"
#include "StringUtil.hpp"
#include <algorithm>
#include <stdexcept>

// The machine state below feeds the simulated instructions' constant
// forwarding (a ZP byte known to hold a constant becomes #imm). The tracking
// after each regular instruction models immediate, base-page, absolute,
// accumulator and implied forms of the common instructions; this forgets what
// any other instruction changes: the 32-bit (Q) instructions, indexed and
// indirect loads/stores, and read-modify-write instructions on memory.
static void forgetUnmodelledEffects(MachineState& ms, const std::string& mnemonic, bool quad,
                                    AddressingMode mode, uint32_t val, bool hasValue) {
    std::string M = mnemonic;
    std::transform(M.begin(), M.end(), M.begin(), ::toupper);
    bool regMode = mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED;
    if (quad) {
        if (M != "STQ" && M != "CMPQ" && M != "BITQ") {
            ms.invalidateReg(REG_A); ms.invalidateReg(REG_X);
            ms.invalidateReg(REG_Y); ms.invalidateReg(REG_Z);
        }
        if (M != "STQ") ms.flags.invalidate();
        if (!regMode && (M == "STQ" || M == "ASLQ" || M == "LSRQ" || M == "ROLQ" || M == "RORQ" ||
                         M == "ASRQ" || M == "INQ" || M == "DEQ"))
            ms.invalidateAllMem();
        return;
    }
    bool isStore = M == "STA" || M == "STX" || M == "STY" || M == "STZ";
    bool isRMW = M == "INC" || M == "DEC" || M == "ASL" || M == "LSR" || M == "ROL" || M == "ROR" ||
                 M == "ASR" || M == "TSB" || M == "TRB" || M == "INW" || M == "DEW" || M == "ASW" ||
                 M == "ROW" || (M.size() == 4 && (M.compare(0, 3, "RMB") == 0 || M.compare(0, 3, "SMB") == 0));
    if (regMode || mode == AddressingMode::IMMEDIATE) return;
    if (mode == AddressingMode::BASE_PAGE || mode == AddressingMode::ABSOLUTE) {
        // modelled: loads, ALU, stores to ZP, INC..ROR on ZP; not: stores and
        // RMW on absolute addresses, the other RMW instructions on ZP
        if (!hasValue) { if (isStore || isRMW) ms.invalidateAllMem(); return; }
        bool zp = mode == AddressingMode::BASE_PAGE;
        bool wideRMW = M == "INW" || M == "DEW" || M == "ASW" || M == "ROW";
        bool modelledZP = zp && (isStore || M == "INC" || M == "DEC" || M == "ASL" || M == "LSR" ||
                                 M == "ROL" || M == "ROR");
        if ((isStore || isRMW) && !modelledZP) {
            for (uint32_t b = 0; b < (wideRMW ? 2u : 1u); b++) {
                if (zp) ms.invalidateZP((uint8_t)(val + b));
                else ms.invalidateAbs((uint16_t)(val + b));
            }
            if (isRMW) ms.flags.invalidate();
        }
        return;
    }
    // indexed and indirect forms
    if (M == "LDA" || M == "ADC" || M == "SBC" || M == "AND" || M == "ORA" || M == "EOR") {
        ms.invalidateReg(REG_A); ms.flags.invalidate();
    } else if (M == "LDX") { ms.invalidateReg(REG_X); ms.flags.invalidate(); }
    else if (M == "LDY") { ms.invalidateReg(REG_Y); ms.flags.invalidate(); }
    else if (M == "LDZ") { ms.invalidateReg(REG_Z); ms.flags.invalidate(); }
    else if (isStore) ms.invalidateAllMem();
    else if (isRMW) { ms.invalidateAllMem(); ms.flags.invalidate(); }
    else if (M == "CMP" || M == "CPX" || M == "CPY" || M == "CPZ" || M == "BIT") ms.flags.invalidate();
}
#include <cmath>
#include <iostream>
#include <fstream>

// Forward declarations for helper functions if they are static in AssemblerParser.cpp
// or just re-implement them if they are simple enough and used here.

static bool isBasePageOperandMode(AddressingMode m) {
    return m == AddressingMode::BASE_PAGE || m == AddressingMode::BASE_PAGE_X || m == AddressingMode::BASE_PAGE_Y ||
           m == AddressingMode::BASE_PAGE_X_INDIRECT || m == AddressingMode::BASE_PAGE_INDIRECT_Y ||
           m == AddressingMode::BASE_PAGE_INDIRECT_Z || m == AddressingMode::FLAT_INDIRECT_Z;
}


// .float: the CBM 40-bit format, rounded to nearest (as cc45's constants)
static std::vector<uint8_t> encodeFloat(double val) {
    std::vector<uint8_t> result(5);
    doubleToCBM40(val, result.data());
    return result;
}

std::vector<uint8_t> AssemblerGenerator::generate(AssemblerParser* parser, bool isPrg) {
    std::vector<uint8_t> binary;
    uint32_t start = parser->getFirstOrgAddress();

    // For PRG files with no explicit .org, check for BASIC_UPSTART directive
    if (isPrg && start == 0xFFFFFFFF) {
        for (const auto& stmt : parser->statements) {
            if (stmt->type == AssemblerParser::Statement::BASIC_UPSTART) {
                start = 0x2000;  // Standard MEGA65 code entry point
                break;
            }
        }
    }

    if (isPrg && start != 0xFFFFFFFF) {
        binary.push_back((uint8_t)(start & 0xFF));
        binary.push_back((uint8_t)(start >> 8));
    }
    M65Emitter e(binary, parser->getZPStart()); e.setSpBase(parser->getSpBase()); e.setScratchZP(parser->getScratchZP());
    if (start != 0xFFFFFFFF) {
        e.setAddress(start);
    }
    generate(parser, e);
    return binary;
}

void AssemblerGenerator::generate(AssemblerParser* parser, M65Emitter& e, const std::string& singleSegment) {
    std::shared_ptr<AssemblerParser::ProcContext> currentPass2Proc;
    std::vector<std::shared_ptr<AssemblerParser::ProcContext>> pass2ProcStack;

    // .var symbols replay their assignments in statement order, as in pass 2
    // (labels and equates stay as pass 2 finalized them)
    for (auto& [name, symbol] : parser->symbolTable)
        if (symbol.isVariable) symbol.value = symbol.initialValue;

    std::vector<std::string> order = parser->requestedSegmentOrder;
    if (order.empty()) {
        order = {"init", "code", "data", "bss"};
    }
    
    // Create a list of all known segments to ensure they are all processed
    std::vector<std::string> allSegNames;
    if (!singleSegment.empty()) {
        allSegNames.push_back(singleSegment);
    } else {
        for (const auto& s : order) {
            if (parser->segments.count(s)) allSegNames.push_back(s);
        }
        // Also include any segments not in the requested order
        for (auto const& seg : parser->segmentOrder) {
            bool found = false;
            for (const auto& s : order) if (s == seg->name) { found = true; break; }
            if (!found) allSegNames.push_back(seg->name);
        }
    }

    for (const auto& segName : allSegNames) {
        if (segName == "bss") continue; // Skip BSS segments for binary output
        auto seg = parser->segments[segName];
        if (seg->startAddress == 0xFFFFFFFF) continue; // Skip segments with no content

        bool isDeadCode = false;
        for (auto& stmt : parser->statements) {
            if (stmt->segmentName != segName) continue;

            if (stmt->address > e.getAddress()) {
                e.setAddress(stmt->address);
            }

            parser->pc = stmt->address; // Update parser pc for .PC evaluations
            parser->currentLocalScope_ = stmt->localLabelScope; // Restore auto-scope for @ labels
            e.setRelocScope(stmt->scopePrefix, stmt->localLabelScope);  // for recorded symbol relocs
            if (stmt->deleted) continue; 
            
            std::vector<uint8_t>* binaryPtr = e.getBinary();
            size_t binaryStartIdx = binaryPtr ? binaryPtr->size() : 0;
            // At statement boundaries: labels invalidate everything (branch merge points).
            // Simulated ops: .fp ops don't need pre-invalidation (M65Emitter tracks state
            // through every instruction they emit). Non-.fp ops invalidate registers.
            // Regular instructions: only invalidate what they actually modify — the
            // LDA/STA tracking below handles register + memory state correctly.
            if (!stmt->label.empty()) {
                e.machineState().invalidateAll();
            } else if (stmt->isSimulatedOp()) {
                // All simulated ops: invalidate registers and flags before emission.
                // Pass2 sizing uses a fresh M65Emitter per op, so tsxCached() always
                // emits TSX. The generator must match by resetting state here —
                // otherwise tsxCached() may skip TSX (fewer bytes than predicted),
                // causing gap bytes ($00/BRK) in the binary.
                // tsxCached() still optimizes within a single op's expansion.
                for (int r = 0; r < REG_COUNT; r++)
                    e.machineState().invalidateReg((RegId)r);
                e.machineState().flags.invalidate();
            }
            // Regular instructions: don't pre-invalidate — let the typed methods
            // (lda_imm/sta_zp via emitInstruction) and our post-emission tracking
            // handle state updates accurately.
          try {
            if (!stmt->label.empty()) {
                isDeadCode = false;
                e.emitLabel(stmt->label);
            }

            // Unified simulated op emission via emitFn dispatch
            if (stmt->isSimulatedOp()) {
                if (!isDeadCode) {
                    parser->resolveExprOperand(stmt.get());
                    size_t relocsBefore = e.symbolRelocs().size();
                    stmt->emitFn(parser, e, stmt.get());
                    // A relocatable symbol in the operand that the expansion
                    // recorded no relocation for would be assembled at its
                    // offset in its segment: refuse it
                    int t = stmt->instr.operandTokenIndex;
                    if (t < 0 || (stmt->exprTokenIndex >= 0 && stmt->exprTokenIndex < t)) t = stmt->exprTokenIndex;
                    for (; t >= 0 && t < (int)parser->tokens.size(); ++t) {
                        const auto& tok = parser->tokens[t];
                        if (tok.type == AssemblerTokenType::NEWLINE || tok.type == AssemblerTokenType::END_OF_FILE) break;
                        if (tok.type != AssemblerTokenType::IDENTIFIER ||
                            !parser->isRelocatableSymbolIn(tok.value, stmt->scopePrefix)) continue;
                        bool found = false;
                        for (size_t r = relocsBefore; r < e.symbolRelocs().size() && !found; ++r)
                            found = e.symbolRelocs()[r].symbolName == tok.value;
                        if (!found)
                            throw std::runtime_error("'" + stmt->instr.mnemonic + "' can't relocate '" + tok.value +
                                "' (an address in a relocatable segment); load or store it with plain instructions");
                    }
                }
                if (binaryPtr) stmt->bytes.assign(binaryPtr->begin() + binaryStartIdx, binaryPtr->end());
                continue;
            }

            if (stmt->type == AssemblerParser::Statement::BASIC_UPSTART) {
                if (!isDeadCode) {
                    uint32_t addr = parser->evaluateExpressionAt(stmt->basicUpstartTokenIndex, stmt->scopePrefix);
                    std::string aS = std::to_string(addr & 0xFFFF);
                    int digits = stmt->basicUpstartDigits;
                    if ((int)aS.length() > digits)
                        throw std::runtime_error(".basicUpstart: SYS " + aS + " needs " + std::to_string(aS.length()) +
                            " digits; give the address as a number, or define the label before .basicUpstart");
                    while ((int)aS.length() < digits) aS = " " + aS;
                    // 10 SYS <addr>: the link points at the end-of-program
                    // marker after this line (stub address + 6 + digits)
                    uint32_t link = stmt->address + 6 + (uint32_t)digits;
                    e.emitByte((uint8_t)(link & 0xFF)); e.emitByte((uint8_t)(link >> 8));
                    e.emitByte(0x0a); e.emitByte(0x00);
                    e.emitByte(0x9e); // SYS
                    for (char c : aS) e.emitByte(asmstr::toPetscii(c));
                    e.emitByte(0x00); e.emitByte(0x00); e.emitByte(0x00);
                }
                continue;
            }

            if (stmt->type == AssemblerParser::Statement::INSTRUCTION) {
                if (stmt->isSimulatedOp()) continue; // Handled above
                if (stmt->instr.mnemonic == "proc") {
                    pass2ProcStack.push_back(currentPass2Proc);
                    currentPass2Proc = stmt->procCtx;
                    if (currentPass2Proc && currentPass2Proc->zarLen > 0 && e.objectMode())
                        e.setZar(currentPass2Proc->zarStart, currentPass2Proc->zarLen, currentPass2Proc->name + "__zar");
                    else
                        e.setZar(-1, 0, "");
                    // No prologue — frame pointer removed
                    continue;
                } else if (stmt->instr.mnemonic == "endproc") {
                    if (!isDeadCode) {
                        // Always emit plain RTS — caller handles stack cleanup via PLZ
                        // (RTS #N opcode $62 is unreliable on some 45GS02 hardware)
                        e.emitInstruction("rts", AddressingMode::IMPLIED);
                    }
                    currentPass2Proc = pass2ProcStack.empty() ? nullptr : pass2ProcStack.back();
                    if (!pass2ProcStack.empty()) pass2ProcStack.pop_back();
                    if (currentPass2Proc && currentPass2Proc->zarLen > 0 && e.objectMode())
                        e.setZar(currentPass2Proc->zarStart, currentPass2Proc->zarLen, currentPass2Proc->name + "__zar");
                    else
                        e.setZar(-1, 0, "");
                    isDeadCode = false;
                    continue;
                } else { // Handle all other instructions
                    if (!isDeadCode) {
                        AddressingMode resolvedMode = stmt->instr.mode;
                        if (!stmt->instr.forceMode && stmt->instr.operandTokenIndex != -1 && (stmt->instr.mode == AddressingMode::BASE_PAGE || stmt->instr.mode == AddressingMode::ABSOLUTE ||
                            stmt->instr.mode == AddressingMode::BASE_PAGE_X || stmt->instr.mode == AddressingMode::ABSOLUTE_X ||
                            stmt->instr.mode == AddressingMode::BASE_PAGE_Y || stmt->instr.mode == AddressingMode::ABSOLUTE_Y)) {
                            uint32_t val = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                            bool fitsIn8 = (val <= 0xFF);
                            bool forceAbs = (stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp");
                            // Force absolute for symbols in relocatable segments (BSS, extern)
                            if (fitsIn8 && !forceAbs && stmt->instr.operandTokenIndex < (int)parser->tokens.size() &&
                                parser->tokens[stmt->instr.operandTokenIndex].type == AssemblerTokenType::IDENTIFIER) {
                                std::string symName = stmt->scopePrefix + parser->tokens[stmt->instr.operandTokenIndex].value;
                                if (!parser->symbolTable.count(symName)) symName = parser->tokens[stmt->instr.operandTokenIndex].value;
                                if (parser->isRelocatableSymbol(symName) &&
                                    !parser->isZpSegmentSymbolIn(parser->tokens[stmt->instr.operandTokenIndex].value, stmt->scopePrefix))
                                    forceAbs = true;
                            }
                            if (stmt->instr.mode == AddressingMode::BASE_PAGE || stmt->instr.mode == AddressingMode::ABSOLUTE) resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE : AddressingMode::ABSOLUTE;
                            else if (stmt->instr.mode == AddressingMode::BASE_PAGE_X || stmt->instr.mode == AddressingMode::ABSOLUTE_X) resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_X : AddressingMode::ABSOLUTE_X;
                            else if (stmt->instr.mode == AddressingMode::BASE_PAGE_Y || stmt->instr.mode == AddressingMode::ABSOLUTE_Y) resolvedMode = (fitsIn8 && !forceAbs) ? AddressingMode::BASE_PAGE_Y : AddressingMode::ABSOLUTE_Y;
                            // Promote base-page to absolute if instruction lacks base-page mode
                            const auto& opMap = AssemblerOpcodeDatabase::getOpcodeMap();
                            std::string lm = stmt->instr.mnemonic;
                            std::transform(lm.begin(), lm.end(), lm.begin(), ::tolower);
                            // Strip Q suffix for quad instructions (adcq→adc) to match opcode map
                            std::string lookupM = lm;
                            if (lookupM.size() > 1 && lookupM.back() == 'q' && lookupM != "ldq" && lookupM != "stq"
                                && lookupM.substr(0,3) != "beq" && lookupM.substr(0,3) != "bne" && lookupM.substr(0,3) != "bra" && lookupM.substr(0,3) != "bsr")
                                lookupM = lookupM.substr(0, lookupM.size() - 1);
                            if (resolvedMode == AddressingMode::BASE_PAGE && opMap.find({lookupM, AddressingMode::BASE_PAGE}) == opMap.end())
                                resolvedMode = AddressingMode::ABSOLUTE;
                            else if (resolvedMode == AddressingMode::BASE_PAGE_X && opMap.find({lookupM, AddressingMode::BASE_PAGE_X}) == opMap.end())
                                resolvedMode = AddressingMode::ABSOLUTE_X;
                            else if (resolvedMode == AddressingMode::BASE_PAGE_Y && opMap.find({lookupM, AddressingMode::BASE_PAGE_Y}) == opMap.end())
                                resolvedMode = AddressingMode::ABSOLUTE_Y;
                        }
                        bool isQuad = (stmt->instr.mnemonic.size() > 1 && stmt->instr.mnemonic.back() == 'q' && stmt->instr.mnemonic != "beq" && stmt->instr.mnemonic != "bne" && stmt->instr.mnemonic != "bra" && stmt->instr.mnemonic != "bsr");
                        if (isQuad) { e.emitByte(0x42); e.emitByte(0x42); }
                        e.setQuadNext(isQuad);
                        e.setIndexReach(stmt->bpIndex);
                        if (resolvedMode == AddressingMode::FLAT_INDIRECT_Z) e.eom();


                        if (stmt->instr.mnemonic == "asw" && resolvedMode == AddressingMode::ABSOLUTE) {
                            uint32_t addr = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                            e.asw_abs(addr);
                            continue;
                        } else if (stmt->instr.mnemonic == "row" && resolvedMode == AddressingMode::ABSOLUTE) {
                            uint32_t addr = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                            e.row_abs(addr);
                            continue;
                        } else if (stmt->instr.mode == AddressingMode::BASE_PAGE_RELATIVE) {
                            Symbol* sym = parser->resolveSymbol(stmt->instr.operand, stmt->scopePrefix);
                            uint32_t v = sym ? sym->value : parseNumericLiteral(stmt->instr.operand);
                            uint8_t op = AssemblerOpcodeDatabase::getOpcode(stmt->instr.mnemonic, resolvedMode);
                            if (sym && sym->segment == "zp" && parser->isZpSegmentSymbolIn(stmt->instr.operand, stmt->scopePrefix))
                                e.recordSymbolRelocLo(stmt->instr.operand, 0);
                            e.emitByte(op);
                            e.emitByte(e.zarOperand((uint8_t)v));
                            int32_t off;
                            if (stmt->hasBranchOffset) {
                                off = stmt->resolvedBranchOffset;
                            } else {
                                Symbol* tsym = parser->resolveSymbol(stmt->instr.bitBranchTarget, stmt->scopePrefix);
                                uint32_t t = tsym ? tsym->value : 0;
                                off = (int32_t)t - (int32_t)(stmt->address + 3);
                            }
                            e.emitByte((uint8_t)off);
                        } else if (stmt->instr.mnemonic == "rtn") {
                            uint32_t v = 0;
                            if (stmt->instr.operandTokenIndex != -1) {
                                // Check if there's actually a token to evaluate
                                if (stmt->instr.operandTokenIndex < (int)parser->tokens.size() &&
                                    parser->tokens[stmt->instr.operandTokenIndex].type != AssemblerTokenType::NEWLINE &&
                                    parser->tokens[stmt->instr.operandTokenIndex].type != AssemblerTokenType::END_OF_FILE) {
                                    v = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                                }
                            }
                            if (v == 0) e.emitInstruction("rts", AddressingMode::IMPLIED);
                            else e.emitInstruction("rts", AddressingMode::IMMEDIATE, v, true);
                        } else { // Handle generic instructions and their operands
                            bool isBranch = (stmt->instr.mnemonic == "beq" || stmt->instr.mnemonic == "bne" || stmt->instr.mnemonic == "bra" || stmt->instr.mnemonic == "bcc" || stmt->instr.mnemonic == "bcs" || stmt->instr.mnemonic == "bpl" || stmt->instr.mnemonic == "bmi" || stmt->instr.mnemonic == "bvc" || stmt->instr.mnemonic == "bvs" || stmt->instr.mnemonic == "bsr");
                            uint32_t val = 0;
                            bool hasValue = false;
                            if (!isBranch) {
                                if (resolvedMode == AddressingMode::IMMEDIATE || resolvedMode == AddressingMode::STACK_RELATIVE || resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_Z || resolvedMode == AddressingMode::FLAT_INDIRECT_Z || resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_SP_Y || resolvedMode == AddressingMode::BASE_PAGE_X_INDIRECT || resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_Y || resolvedMode == AddressingMode::BASE_PAGE || resolvedMode == AddressingMode::BASE_PAGE_X || resolvedMode == AddressingMode::BASE_PAGE_Y || resolvedMode == AddressingMode::ABSOLUTE || resolvedMode == AddressingMode::ABSOLUTE_X || resolvedMode == AddressingMode::ABSOLUTE_Y || resolvedMode == AddressingMode::ABSOLUTE_INDIRECT || resolvedMode == AddressingMode::ABSOLUTE_X_INDIRECT || resolvedMode == AddressingMode::IMMEDIATE16) {
                                    val = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                                    hasValue = true;

                                    // Overflow check for 8-bit modes (-128..-1 is a byte too: #-1 is $FF)
                                    if (val > 0xFF && !((int32_t)val < 0 && (int32_t)val >= -128)) {
                                        bool is8BitMode = (resolvedMode == AddressingMode::IMMEDIATE || 
                                                           resolvedMode == AddressingMode::BASE_PAGE || 
                                                           resolvedMode == AddressingMode::BASE_PAGE_X || 
                                                           resolvedMode == AddressingMode::BASE_PAGE_Y ||
                                                           resolvedMode == AddressingMode::INDIRECT ||
                                                           resolvedMode == AddressingMode::BASE_PAGE_X_INDIRECT ||
                                                           resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_Y ||
                                                           resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_Z ||
                                                           resolvedMode == AddressingMode::BASE_PAGE_INDIRECT_SP_Y ||
                                                           resolvedMode == AddressingMode::STACK_RELATIVE ||
                                                           resolvedMode == AddressingMode::FLAT_INDIRECT_Z);
                                        if (is8BitMode) {
                                            parser->addWarning("line " + std::to_string(stmt->line) + ": warning: 8-bit operand overflow (value " + std::to_string(val) + " truncated to " + std::to_string(val & 0xFF) + ")");
                                        }
                                    }
                                    // -Woverflow / -Wunderflow (off by default)
                                    if (parser->warnOverflow || parser->warnUnderflow) {
                                        parser->currentSourceFile_ = stmt->sourceFile;
                                        bool isImm = (resolvedMode == AddressingMode::IMMEDIATE || resolvedMode == AddressingMode::IMMEDIATE16);
                                        if ((int32_t)val < 0) {
                                            if (isImm) parser->checkImmediateUnderflow((int32_t)val, stmt->line, 1);
                                            else parser->checkAddressUnderflow((int32_t)val, stmt->line, 1);
                                        } else if (!isImm || resolvedMode == AddressingMode::IMMEDIATE16) {
                                            parser->checkAddressOverflow(val, resolvedMode, stmt->line, 1);
                                        }
                                    }
                                }
                                // a "zp" segment label's byte: relocated in the program's base page
                                if (hasValue && isBasePageOperandMode(resolvedMode)) {
                                    std::string zs = parser->zpSegmentOperand(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                                    if (!zs.empty()) e.recordSymbolRelocLo(zs, (int)((val >> 8) & 0xFF));
                                }
                                e.emitInstruction(stmt->instr.mnemonic, resolvedMode, val, hasValue);
                                forgetUnmodelledEffects(e.machineState(), stmt->instr.mnemonic, isQuad, resolvedMode, val, hasValue);

                                // Update MachineState for regular instructions
                                // (emitInstruction doesn't go through typed methods)
                                if (hasValue) {
                                    std::string mn2 = stmt->instr.mnemonic;
                                    std::transform(mn2.begin(), mn2.end(), mn2.begin(), ::toupper);
                                    if (resolvedMode == AddressingMode::IMMEDIATE) {
                                        // Phase 78: SMC placeholder detection (#$00 in SAC functions)
                                        if (val == 0 && e.isSACMode() &&
                                            (mn2 == "LDA" || mn2 == "LDX" || mn2 == "LDY" || mn2 == "LDZ")) {
                                            // Placeholder immediate for SMC parameter
                                            // Will be patched by linker with actual parameter value
                                            e.recordImmediateReloc("__smc_param_" + std::to_string(e.getAddress()), false);
                                        }

                                        // LDA #imm → setConst (setReloc handled below for </>sym)
                                        if (mn2 == "LDA") e.machineState().setConst(REG_A, val & 0xFF);
                                        else if (mn2 == "LDX") e.machineState().setConst(REG_X, val & 0xFF);
                                        else if (mn2 == "LDY") e.machineState().setConst(REG_Y, val & 0xFF);
                                        else if (mn2 == "LDZ") e.machineState().setConst(REG_Z, val & 0xFF);
                                        // ALU imm: clobbers A and flags
                                        else if (mn2 == "ADC" || mn2 == "SBC" || mn2 == "AND" ||
                                                 mn2 == "ORA" || mn2 == "EOR") {
                                            e.machineState().invalidateReg(REG_A);
                                            e.machineState().flags.invalidate();
                                        }
                                        // CMP/CPX/CPY: flags only
                                        else if (mn2 == "CMP" || mn2 == "CPX" || mn2 == "CPY") {
                                            e.machineState().flags.invalidate();
                                        }
                                    } else if (resolvedMode == AddressingMode::IMMEDIATE16) {
                                        // PHW #imm16: pushes 2 bytes, modifies SP
                                        if (mn2 == "PHW") e.machineState().spModified();
                                    } else if (resolvedMode == AddressingMode::BASE_PAGE) {
                                        // Stores: propagate register value to ZP memory
                                        if (mn2 == "STA") e.machineState().storeZP((uint8_t)val, REG_A);
                                        else if (mn2 == "STX") e.machineState().storeZP((uint8_t)val, REG_X);
                                        else if (mn2 == "STY") e.machineState().storeZP((uint8_t)val, REG_Y);
                                        else if (mn2 == "STZ") e.machineState().storeZP((uint8_t)val, REG_Z);
                                        // Loads: propagate ZP memory to register
                                        else if (mn2 == "LDA") e.machineState().setRegFromZP(REG_A, (uint8_t)val);
                                        else if (mn2 == "LDX") e.machineState().setRegFromZP(REG_X, (uint8_t)val);
                                        else if (mn2 == "LDY") e.machineState().setRegFromZP(REG_Y, (uint8_t)val);
                                        else if (mn2 == "LDZ") e.machineState().setRegFromZP(REG_Z, (uint8_t)val);
                                        // ALU from ZP: clobbers A and flags
                                        else if (mn2 == "ADC" || mn2 == "SBC" || mn2 == "AND" ||
                                                 mn2 == "ORA" || mn2 == "EOR") {
                                            e.machineState().invalidateReg(REG_A);
                                            e.machineState().flags.invalidate();
                                        }
                                        // RMW on ZP: invalidate memory + flags
                                        else if (mn2 == "INC" || mn2 == "DEC" || mn2 == "ASL" ||
                                                 mn2 == "LSR" || mn2 == "ROL" || mn2 == "ROR") {
                                            e.machineState().invalidateZP((uint8_t)val);
                                            e.machineState().flags.invalidate();
                                        }
                                    } else if (resolvedMode == AddressingMode::ACCUMULATOR ||
                                               resolvedMode == AddressingMode::IMPLIED) {
                                        // Accumulator ALU
                                        if (mn2 == "ASL" || mn2 == "LSR" || mn2 == "ROL" ||
                                            mn2 == "ROR" || mn2 == "INC" || mn2 == "DEC" || mn2 == "NEG") {
                                            e.machineState().invalidateReg(REG_A);
                                            e.machineState().flags.invalidate();
                                        }
                                        // Transfers
                                        else if (mn2 == "TAX") e.machineState().setTransfer(REG_X, REG_A);
                                        else if (mn2 == "TXA") e.machineState().setTransfer(REG_A, REG_X);
                                        else if (mn2 == "TAY") e.machineState().setTransfer(REG_Y, REG_A);
                                        else if (mn2 == "TYA") e.machineState().setTransfer(REG_A, REG_Y);
                                        else if (mn2 == "TAZ") e.machineState().setTransfer(REG_Z, REG_A);
                                        else if (mn2 == "TZA") e.machineState().setTransfer(REG_A, REG_Z);
                                        else if (mn2 == "TSX") e.machineState().setTransfer(REG_X, REG_SP);
                                        else if (mn2 == "INX" || mn2 == "DEX") { e.machineState().invalidateReg(REG_X); e.machineState().flags.invalidate(); }
                                        else if (mn2 == "INY" || mn2 == "DEY") { e.machineState().invalidateReg(REG_Y); e.machineState().flags.invalidate(); }
                                        else if (mn2 == "INZ" || mn2 == "DEZ") { e.machineState().invalidateReg(REG_Z); e.machineState().flags.invalidate(); }
                                        // Push/pull
                                        else if (mn2 == "PHA" || mn2 == "PHX" || mn2 == "PHY" || mn2 == "PHZ") e.machineState().spModified();
                                        else if (mn2 == "PLA") { e.machineState().spModified(); e.machineState().invalidateReg(REG_A); }
                                        else if (mn2 == "PLX") { e.machineState().spModified(); e.machineState().invalidateReg(REG_X); }
                                        else if (mn2 == "PLY") { e.machineState().spModified(); e.machineState().invalidateReg(REG_Y); }
                                        else if (mn2 == "PLZ") { e.machineState().spModified(); e.machineState().invalidateReg(REG_Z); }
                                        // Carry
                                        else if (mn2 == "CLC") e.machineState().flags.setCarry(false);
                                        else if (mn2 == "SEC") e.machineState().flags.setCarry(true);
                                        // JSR/JMP: invalidate all
                                        else if (mn2 == "JSR" || mn2 == "JMP" || mn2 == "RTS" || mn2 == "RTI") e.machineState().invalidateAll();
                                    } else if (resolvedMode == AddressingMode::ABSOLUTE) {
                                        // Loads from absolute: unknown register value
                                        if (mn2 == "LDA") { e.machineState().invalidateReg(REG_A); e.machineState().flags.setNZ(REG_A); }
                                        else if (mn2 == "LDX") { e.machineState().invalidateReg(REG_X); e.machineState().flags.setNZ(REG_X); }
                                        else if (mn2 == "LDY") { e.machineState().invalidateReg(REG_Y); e.machineState().flags.setNZ(REG_Y); }
                                        else if (mn2 == "LDZ") { e.machineState().invalidateReg(REG_Z); e.machineState().flags.setNZ(REG_Z); }
                                        // ALU from abs: clobbers A + flags
                                        else if (mn2 == "ADC" || mn2 == "SBC" || mn2 == "AND" ||
                                                 mn2 == "ORA" || mn2 == "EOR") {
                                            e.machineState().invalidateReg(REG_A);
                                            e.machineState().flags.invalidate();
                                        }
                                        // JSR absolute: invalidate all
                                        else if (mn2 == "JSR") e.machineState().invalidateAll();
                                        // PHW abs: pushes 2 bytes, modifies SP
                                        else if (mn2 == "PHW") e.machineState().spModified();
                                    }
                                } else {
                                    // No value — check implied/accumulator mode instructions
                                    std::string mn2 = stmt->instr.mnemonic;
                                    std::transform(mn2.begin(), mn2.end(), mn2.begin(), ::toupper);
                                    if (mn2 == "TAX") e.machineState().setTransfer(REG_X, REG_A);
                                    else if (mn2 == "TXA") e.machineState().setTransfer(REG_A, REG_X);
                                    else if (mn2 == "TAY") e.machineState().setTransfer(REG_Y, REG_A);
                                    else if (mn2 == "TYA") e.machineState().setTransfer(REG_A, REG_Y);
                                    else if (mn2 == "TAZ") e.machineState().setTransfer(REG_Z, REG_A);
                                    else if (mn2 == "TZA") e.machineState().setTransfer(REG_A, REG_Z);
                                    else if (mn2 == "TSX") e.machineState().setTransfer(REG_X, REG_SP);
                                    else if (mn2 == "PHA" || mn2 == "PHX" || mn2 == "PHY" || mn2 == "PHZ") e.machineState().spModified();
                                    else if (mn2 == "PLA") { e.machineState().spModified(); e.machineState().invalidateReg(REG_A); }
                                    else if (mn2 == "PLX") { e.machineState().spModified(); e.machineState().invalidateReg(REG_X); }
                                    else if (mn2 == "PLY") { e.machineState().spModified(); e.machineState().invalidateReg(REG_Y); }
                                    else if (mn2 == "PLZ") { e.machineState().spModified(); e.machineState().invalidateReg(REG_Z); }
                                    else if (mn2 == "CLC") e.machineState().flags.setCarry(false);
                                    else if (mn2 == "SEC") e.machineState().flags.setCarry(true);
                                    else if (mn2 == "INX" || mn2 == "DEX") { e.machineState().invalidateReg(REG_X); e.machineState().flags.invalidate(); }
                                    else if (mn2 == "INY" || mn2 == "DEY") { e.machineState().invalidateReg(REG_Y); e.machineState().flags.invalidate(); }
                                    else if (mn2 == "INZ" || mn2 == "DEZ") { e.machineState().invalidateReg(REG_Z); e.machineState().flags.invalidate(); }
                                    else if (mn2 == "RTS" || mn2 == "RTI" || mn2 == "JMP") e.machineState().invalidateAll();
                                }

                                // Track relocatable symbol bytes in MachineState
                                // for constant-memory forwarding (RELOC_CONST).
                                // Pattern: LDA #<sym / LDX #>sym with relocatable symbol
                                if (hasValue && resolvedMode == AddressingMode::IMMEDIATE &&
                                    stmt->instr.operandTokenIndex >= 0) {
                                    std::string mn = stmt->instr.mnemonic;
                                    std::transform(mn.begin(), mn.end(), mn.begin(), ::toupper);
                                    RegId targetReg = REG_A;
                                    bool isLoad = false;
                                    if (mn == "LDA") { targetReg = REG_A; isLoad = true; }
                                    else if (mn == "LDX") { targetReg = REG_X; isLoad = true; }
                                    else if (mn == "LDY") { targetReg = REG_Y; isLoad = true; }
                                    else if (mn == "LDZ") { targetReg = REG_Z; isLoad = true; }
                                    if (isLoad) {
                                        int ti = stmt->instr.operandTokenIndex;
                                        // Skip # token
                                        if (ti < (int)parser->tokens.size() &&
                                            parser->tokens[ti].type == AssemblerTokenType::HASH) ti++;
                                        // Check for < or > prefix (lo/hi byte operator)
                                        if (ti < (int)parser->tokens.size() &&
                                            (parser->tokens[ti].type == AssemblerTokenType::LESS_THAN ||
                                             parser->tokens[ti].type == AssemblerTokenType::GREATER_THAN)) {
                                            bool isLo = (parser->tokens[ti].type == AssemblerTokenType::LESS_THAN);
                                            ti++;
                                            if (ti < (int)parser->tokens.size() &&
                                                parser->tokens[ti].type == AssemblerTokenType::IDENTIFIER) {
                                                std::string symName = parser->tokens[ti].value;
                                                Symbol* sym = parser->resolveSymbol(symName, stmt->scopePrefix);
                                                if (sym) {
                                                    // (the whole value: '<' / '>' apply to all after them)
                                                    int64_t full = sym->value;
                                                    try { full = parser->evaluateExpressionAt(ti, stmt->scopePrefix); } catch (...) {}
                                                    e.machineState().setReloc(targetReg, symName,
                                                        isLo ? ValueInfo::RELOC_LO : ValueInfo::RELOC_HI,
                                                        val, full);
                                                }
                                            }
                                        }
                                    }
                                }
                            } else { // Branch instructions
                                int32_t off;
                                if (stmt->hasBranchOffset) {
                                    off = stmt->resolvedBranchOffset;
                                } else {
                                    // Fallback: compute offset (shouldn't happen in normal flow)
                                    uint32_t t;
                                    if (stmt->instr.operandTokenIndex >= 0) {
                                        t = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                                    } else if (!stmt->instr.operand.empty() && parser->symbolTable.count(stmt->instr.operand)) {
                                        t = parser->symbolTable.at(stmt->instr.operand).value;
                                    } else {
                                        t = stmt->address;
                                    }
                                    off = (int32_t)t - (int32_t)(stmt->address + 2);
                                }
                                if (stmt->instr.mnemonic == "bsr") {
                                    e.emitInstruction("bsr", AddressingMode::RELATIVE16, (uint32_t)(uint16_t)(int16_t)off, true);
                                    e.machineState().invalidateAll();
                                } else if (stmt->size == 2) {
                                    e.emitInstruction(stmt->instr.mnemonic, AddressingMode::RELATIVE, (uint32_t)(uint8_t)(int8_t)off, true);
                                } else {
                                    e.emitInstruction(stmt->instr.mnemonic, AddressingMode::RELATIVE16, (uint32_t)(uint16_t)(int16_t)off, true);
                                }
                            }
                        }
                    }
                    if (stmt->instr.mnemonic == "rts" || stmt->instr.mnemonic == "rtn" || stmt->instr.mnemonic == "rti") isDeadCode = true;
                }
            } else if (stmt->type == AssemblerParser::Statement::DIRECTIVE) {
                // Reset isDeadCode for .noopt_start (indicates executable code ahead).
                // After RTS/RTN/RTI instructions, isDeadCode is true and all following code is skipped.
                // .noopt_start marks regions with critical instructions (like ldy #0 for array access)
                // that must be emitted even if they appear after dead code. Resetting the flag ensures
                // these marked regions are output to the binary.
                if (stmt->dir.name == "noopt_start") {
                    isDeadCode = false;
                }
                // Data after a return (a table, an inline constant) is not dead
                // code: the parser gave it its place, so its bytes are emitted
                // (skipped, they left zeros, or nothing at the segment's end)
                if (stmt->size > 0) isDeadCode = false;
                // .var / .local: each assignment holds from its position on
                // (in dead code too, as pass 2 tracks them)
                if (stmt->dir.name == "var") {
                    if (stmt->dir.varType == Directive::ASSIGN)
                        parser->symbolTable[stmt->dir.varName].value =
                            parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                    else if (stmt->dir.varType == Directive::INC) parser->symbolTable[stmt->dir.varName].value++;
                    else if (stmt->dir.varType == Directive::DEC) parser->symbolTable[stmt->dir.varName].value--;
                } else if (stmt->dir.name == "local" && stmt->dir.varType == Directive::ASSIGN) {
                    uint32_t val = parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                    auto& sym = parser->symbolTable[stmt->dir.varName];
                    sym.value = val;
                    sym.frameOffset = (int)val;
                    sym.isFrameRelative = true;
                }
                if (!isDeadCode || stmt->dir.name == "org") {
                    if (stmt->dir.name == "var" || stmt->dir.name == "local") {
                    }
                    else if (stmt->dir.name == "cleanup") { if (currentPass2Proc) currentPass2Proc->totalParamSize += parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix); }
                    else if (stmt->dir.name == "frameptr_zp") {
                        // .frameptr_zp $FD — Set ZP location for frame pointer (for frame-relative addressing)
                        if (!stmt->dir.arguments.empty()) {
                            uint8_t fp = (uint8_t)parseNumericLiteral(stmt->dir.arguments[0]);
                            e.setFramePointerZP(fp);
                        }
                    }
                    // Phase 78: SAC (Static Allocation Convention) function marker
                    else if (stmt->dir.name == "sac") {
                        // Enable SAC mode for this function
                        // Parameters use static AR buffer instead of stack
                        e.setSACMode(true);
                    }
                    else if (stmt->dir.name == "byte") {
                        // a number, or any expression ('A', 5-1, <label, -1)
                        for (size_t ai = 0; ai < stmt->dir.arguments.size(); ++ai) {
                            int ti = ai < stmt->dir.argTokenIndices.size() ? stmt->dir.argTokenIndices[ai] : -1;
                            const std::string& a = stmt->dir.arguments[ai];
                            // <sym / >sym of a relocatable or external symbol:
                            // the byte with a relocation (R_LOWX / R_HIGH)
                            // (<(sym+1): the name inside the parentheses)
                            int ni = ti + 1;
                            while (ti >= 0 && ni + 1 < (int)parser->tokens.size() &&
                                   parser->tokens[ni].type == AssemblerTokenType::OPEN_PAREN) ni++;
                            if (ti >= 0 && !a.empty() && (a[0] == '<' || a[0] == '>') && ni < (int)parser->tokens.size() &&
                                parser->tokens[ni].type == AssemblerTokenType::IDENTIFIER) {
                                std::string name = parser->tokens[ni].value;
                                bool ext = parser->isExternSymbol(name);
                                // (<(blk_end-blk): a difference of two labels of one segment is a constant)
                                bool moves = ext;
                                if (!ext && parser->isRelocatableSymbolIn(name, stmt->scopePrefix)) {
                                    auto m = parser->segmentMultiples(ti + 1, stmt->scopePrefix);
                                    moves = m.empty();
                                    for (const auto& [seg, k] : m) {
                                        if (k == 0) continue;
                                        Symbol* s0 = parser->resolveSymbol(name, stmt->scopePrefix);
                                        if (k != 1 || !s0 || s0->segment != seg)
                                            throw std::runtime_error("the value can't be relocated: it holds the address of a label of segment '" +
                                                                     seg + "' " + (k == 0x7FFF ? std::string("in part") : std::to_string(k) + " times") + "");
                                        moves = true;
                                    }
                                }
                                if (moves) {
                                    uint32_t full = 0;
                                    try { full = parser->evaluateExpressionAt(ti + 1, stmt->scopePrefix); }
                                    catch (...) { if (!ext) throw; }
                                    if (a[0] == '<') {
                                        e.recordDataRelocLo(name, (uint8_t)(full >> 8));
                                        e.emitByte((uint8_t)full);
                                    } else {
                                        e.recordDataRelocHi(name, (uint8_t)full);
                                        e.emitByte((uint8_t)(full >> 8));
                                    }
                                    continue;
                                }
                            }
                            uint32_t v;
                            try { v = parseNumericLiteral(a); }
                            catch (...) {
                                if (ti < 0) throw;
                                v = parser->evaluateExpressionAt(ti, stmt->scopePrefix);
                            }
                            e.emitByte((uint8_t)v);
                        }
                    }
                    else if (stmt->dir.name == "word") {
                        for (size_t ai = 0; ai < stmt->dir.arguments.size(); ai++) {
                            const auto& a = stmt->dir.arguments[ai];
                            // Try numeric first, otherwise the whole expression
                            // (lbl, lbl+1, lbl-2); a symbol gets a relocation
                            int ti0 = ai < stmt->dir.argTokenIndices.size() ? stmt->dir.argTokenIndices[ai] : -1;
                            // an expression that does not start with a symbol (1+2, -1, (5-1)*2)
                            // is a constant
                            if (ti0 >= 0 && parser->tokens[ti0].type != AssemblerTokenType::IDENTIFIER) {
                                uint32_t v;
                                try { v = parseNumericLiteral(a); }
                                catch (...) { v = parser->evaluateExpressionAt(ti0, stmt->scopePrefix); }
                                e.emitWord((uint16_t)v);
                                continue;
                            }
                            try {
                                e.emitWord((uint16_t)parseNumericLiteral(a));
                            } catch (...) {
                                int ti = ti0;
                                std::string name = ti0 >= 0 ? parser->tokens[ti0].value : a.substr(0, a.find_first_of("+-", 1));
                                Symbol* sym = parser->resolveSymbol(name, stmt->scopePrefix);
                                if (!sym && !parser->isExternSymbol(name))
                                    throw std::runtime_error("undefined symbol '" + name + "' in .word");
                                // a constant (equate, or a label in an .org'd segment):
                                // any expression (K*2, K<<8 | 1)
                                if (ti0 >= 0 && !parser->isRelocatableSymbolIn(name, stmt->scopePrefix)) {
                                    e.emitWord((uint16_t)parser->evaluateExpressionAt(ti0, stmt->scopePrefix));
                                    continue;
                                }
                                // symbol + addend ("+1", "-2"); anything more
                                // complex goes through the expression evaluator
                                std::string rest = a.substr(name.size());
                                // a difference of two labels of one segment
                                // (blk_end-blk, blk_end-blk+1): a constant,
                                // no relocation (twice a label: an error)
                                if (ti0 >= 0) {
                                    auto m = parser->segmentMultiples(ti0, stmt->scopePrefix);
                                    bool moves = m.empty();
                                    for (const auto& [seg, k] : m) {
                                        if (k == 0) continue;
                                        if (k != 1 || !sym || sym->segment != seg)
                                            throw std::runtime_error("the value can't be relocated: it holds the address of a label of segment '" +
                                                                     seg + "' " + (k == 0x7FFF ? std::string("in part") : std::to_string(k) + " times"));
                                        moves = true;
                                    }
                                    if (!moves) {
                                        e.emitWord((uint16_t)parser->evaluateExpressionAt(ti0, stmt->scopePrefix));
                                        continue;
                                    }
                                }
                                uint32_t val = sym ? sym->value : 0;
                                bool simple = true;
                                if (!rest.empty()) {
                                    try {
                                        long add = (long)parseNumericLiteral(rest.substr(1));
                                        val += (uint32_t)(rest[0] == '-' ? -add : add);
                                    } catch (...) { simple = false; }
                                }
                                if (!simple && ti >= 0) val = parser->evaluateExpressionAt(ti, stmt->scopePrefix);
                                e.emitWordReloc(name, (uint16_t)val);
                            }
                        }
                    }
                    else if (stmt->dir.name == "dword" || stmt->dir.name == "long") {
                        for (size_t ai = 0; ai < stmt->dir.arguments.size(); ++ai) {
                            int ti = ai < stmt->dir.argTokenIndices.size() ? stmt->dir.argTokenIndices[ai] : -1;
                            uint32_t v;
                            try { v = parseNumericLiteral(stmt->dir.arguments[ai]); }
                            catch (...) {
                                if (ti < 0) throw;
                                v = parser->evaluateExpressionAt(ti, stmt->scopePrefix);
                            }
                            e.emitWord(v & 0xFFFF); e.emitWord(v >> 16);
                        }
                    }
                    else if (stmt->dir.name == "float") for (const auto& a : stmt->dir.arguments) { double v = std::stod(a); std::vector<uint8_t> enc = encodeFloat(v); for (uint8_t eb : enc) e.emitByte(eb); }
                    else if (stmt->dir.name == "text" || stmt->dir.name == "ascii" || stmt->dir.name == "screencode") {
                        if (!stmt->dir.arguments.empty())
                            for (uint8_t b : asmstr::encode(stmt->dir.arguments[0], stmt->dir.name)) e.emitByte(b);
                    }
                    else if (stmt->dir.name == "res" || stmt->dir.name == "fill" || stmt->dir.name == "space") {
                        uint32_t count = parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                        uint8_t fill = 0;
                        if (stmt->dir.valueTokenIndex >= 0)
                            fill = (uint8_t)parser->evaluateExpressionAt(stmt->dir.valueTokenIndex, stmt->scopePrefix);
                        for (uint32_t i = 0; i < count; ++i) e.emitByte(fill);
                    }
                    else if (stmt->dir.name == "array") {
                        uint32_t count = stmt->dir.arguments.empty() ? 0 : (uint32_t)parseNumericLiteral(stmt->dir.arguments[0]);
                        for (uint32_t i = 0; i < count; ++i) e.emitByte(0);
                    }
                    else if (stmt->dir.name == "align" || stmt->dir.name == "balign") {
                        uint8_t fill = 0;
                        if (stmt->dir.valueTokenIndex >= 0)
                            fill = (uint8_t)parser->evaluateExpressionAt(stmt->dir.valueTokenIndex, stmt->scopePrefix);
                        for (int i = 0; i < stmt->size; ++i) e.emitByte(fill);
                    }
                    else if (stmt->dir.name == "assert") {
                        if (parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix) == 0)
                            throw std::runtime_error(stmt->dir.arguments.empty() ? "assertion failed" : stmt->dir.arguments[0]);
                    }
                    else if (stmt->dir.name == "fillto") {
                        uint8_t fill = 0;
                        if (stmt->dir.valueTokenIndex >= 0)
                            fill = (uint8_t)parser->evaluateExpressionAt(stmt->dir.valueTokenIndex, stmt->scopePrefix);
                        for (int i = 0; i < stmt->size; ++i) e.emitByte(fill);
                    }
                    else if (stmt->dir.name == "import" || stmt->dir.name == "incbin") {
                        std::string filename;
                        if (stmt->dir.name == "import") {
                            if (stmt->dir.arguments.size() < 2 || stmt->dir.arguments[0] != "binary")
                                throw std::runtime_error(".import requires 'binary' keyword");
                            filename = stmt->dir.arguments[1];
                        } else if (stmt->dir.arguments.size() >= 1) {
                            filename = stmt->dir.arguments[0];
                        }
                        if (filename.size() >= 2 && filename.front() == '"' && filename.back() == '"')
                            filename = filename.substr(1, filename.size() - 2);
                        std::ifstream file(filename, std::ios::binary);
                        if (!file)
                            throw std::runtime_error("cannot open binary file '" + filename + "'");
                        char buffer[1024];
                        while (file.read(buffer, sizeof(buffer))) {
                            for (int i = 0; i < (int)file.gcount(); ++i) e.emitByte((uint8_t)buffer[i]);
                        }
                        for (int i = 0; i < (int)file.gcount(); ++i) e.emitByte((uint8_t)buffer[i]);
                    }
                }
            }
            if (binaryPtr) {
                stmt->bytes.assign(binaryPtr->begin() + binaryStartIdx, binaryPtr->end());
            }
          } catch (const std::runtime_error& ex) {
            std::string m = "line " + std::to_string(stmt->line) + ": " + ex.what();
            if (!stmt->asmFile.empty()) throw AsmFileError(stmt->asmFile, m);
            throw std::runtime_error(m);
          }
        }
    }
}
