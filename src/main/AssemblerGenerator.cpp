#include "AssemblerGenerator.hpp"
#include "AssemblerParser.hpp"
#include "M65Emitter.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "AssemblerSimulatedOps.hpp"
#include "StringUtil.hpp"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <fstream>

// Forward declarations for helper functions if they are static in AssemblerParser.cpp
// or just re-implement them if they are simple enough and used here.

static uint8_t toPetscii(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    if (c >= 'A' && c <= 'Z') return c + 32;
    return (uint8_t)c;
}

static uint8_t toScreencode(char c) {
    uint8_t u = (uint8_t)c;
    if (u == 0x40) return 0x00;             // @ → 0
    if (u >= 0x41 && u <= 0x5A) return u - 0x40;  // A-Z → 1-26
    if (u >= 0x5B && u <= 0x5F) return u - 0x40;  // [\]^_ → 27-31
    if (u >= 0x61 && u <= 0x7A) return u - 0x60;  // a-z → 1-26
    return u;                               // space, digits, punctuation unchanged
}

static std::vector<uint8_t> encodeFloat(double val) {
    std::vector<uint8_t> result(5);
    if (val == 0.0) { std::fill(result.begin(), result.end(), 0); return result; }
    int exponent;
    double mantissa = std::frexp(std::abs(val), &exponent);
    result[0] = (uint8_t)(exponent + 0x80);
    uint32_t mVal = (uint32_t)(mantissa * 4294967296.0);
    result[1] = (uint8_t)((mVal >> 24) & 0x7F) | (val < 0 ? 0x80 : 0x00);
    result[2] = (uint8_t)(mVal >> 16);
    result[3] = (uint8_t)(mVal >> 8);
    result[4] = (uint8_t)mVal;
    return result;
}

std::vector<uint8_t> AssemblerGenerator::generate(AssemblerParser* parser, bool isPrg) {
    std::vector<uint8_t> binary;
    uint32_t start = parser->getFirstOrgAddress();
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

    for (auto& [name, symbol] : parser->symbolTable) if (symbol.isVariable) symbol.value = symbol.initialValue;

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
                if (!isDeadCode) stmt->emitFn(parser, e, stmt.get());
                if (binaryPtr) stmt->bytes.assign(binaryPtr->begin() + binaryStartIdx, binaryPtr->end());
                continue;
            }

            if (stmt->type == AssemblerParser::Statement::BASIC_UPSTART) {
                if (!isDeadCode) {
                    uint32_t addr = parser->evaluateExpressionAt(stmt->basicUpstartTokenIndex, stmt->scopePrefix);
                    std::string aS = std::to_string(addr);
                    while (aS.length() < 4) aS = " " + aS;
                    if (aS.length() > 4) aS = aS.substr(aS.length() - 4);
                    e.emitByte(0x0b); e.emitByte(0x08); e.emitByte(0x0a); e.emitByte(0x00);
                    e.emitByte(0x9e); // SYS
                    for (char c : aS) e.emitByte(toPetscii(c));
                    e.emitByte(0x00); e.emitByte(0x00); e.emitByte(0x00);
                }
                continue;
            }

            if (stmt->type == AssemblerParser::Statement::INSTRUCTION) {
                if (stmt->isSimulatedOp()) continue; // Handled above
                if (stmt->instr.mnemonic == "proc") {
                    pass2ProcStack.push_back(currentPass2Proc);
                    currentPass2Proc = stmt->procCtx;
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
                    isDeadCode = false;
                    continue;
                } else if (stmt->instr.mnemonic == "call") {
                    if (!isDeadCode) {
                        for (const auto& arg : stmt->instr.callArgs) {
                            bool isB = (arg.size() >= 2 && arg.substr(0, 2) == "B#");
                            std::string v = isB ? arg.substr(2) : ( (arg.size() >= 2 && arg.substr(0, 2) == "W#") ? arg.substr(2) : (arg[0] == '#' ? arg.substr(1) : arg));
                            uint32_t val;
                            Symbol* sym = parser->resolveSymbol(v, stmt->scopePrefix);
                            if (sym) val = sym->value; else val = parseNumericLiteral(v);
                            if (!isB && !arg.empty() && arg[0] != '#' && arg.size() >= 2 && arg.substr(0,2) != "W#" && sym) isB = (sym->size == 1);
                            if (isB) { e.lda_imm(val & 0xFF); e.pha(); } else { e.emitInstruction("phw", AddressingMode::IMMEDIATE16, val & 0xFFFF, true); }
                        }
                        Symbol* sym = parser->resolveSymbol(stmt->instr.operand, stmt->scopePrefix);
                        uint32_t target = sym ? sym->value : 0;
                        e.emitInstruction("jsr", AddressingMode::ABSOLUTE, target, true);
                    }
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
                                if (parser->isRelocatableSymbol(symName)) forceAbs = true;
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
                            e.emitByte(op);
                            e.emitByte((uint8_t)v);
                            Symbol* tsym = parser->resolveSymbol(stmt->instr.bitBranchTarget, stmt->scopePrefix);
                            uint32_t t = tsym ? tsym->value : 0;
                            int32_t off = (int32_t)t - (int32_t)(stmt->address + 3);
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

                                    // Overflow check for 8-bit modes
                                    if (val > 0xFF) {
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
                                }
                                e.emitInstruction(stmt->instr.mnemonic, resolvedMode, val, hasValue);

                                // Update MachineState for regular instructions
                                // (emitInstruction doesn't go through typed methods)
                                if (hasValue) {
                                    std::string mn2 = stmt->instr.mnemonic;
                                    std::transform(mn2.begin(), mn2.end(), mn2.begin(), ::toupper);
                                    if (resolvedMode == AddressingMode::IMMEDIATE) {
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
                                                    e.machineState().setReloc(targetReg, symName,
                                                        isLo ? ValueInfo::RELOC_LO : ValueInfo::RELOC_HI,
                                                        val);
                                                }
                                            }
                                        }
                                    }
                                }
                            } else { // Branch instructions
                                uint32_t t;
                                if (stmt->instr.operandTokenIndex >= 0) {
                                    t = parser->evaluateExpressionAt(stmt->instr.operandTokenIndex, stmt->scopePrefix);
                                } else if (!stmt->instr.operand.empty() && parser->symbolTable.count(stmt->instr.operand)) {
                                    t = parser->symbolTable.at(stmt->instr.operand).value;
                                } else {
                                    t = stmt->address; // fallback: branch to self
                                }
                                if (stmt->instr.mnemonic == "bsr") {
                                    int32_t off = (int32_t)t - (int32_t)(stmt->address + 2);
                                    e.emitInstruction("bsr", AddressingMode::RELATIVE16, (uint32_t)(uint16_t)(int16_t)off, true);
                                    // BSR is a subroutine call (like JSR) — invalidate all state
                                    e.machineState().invalidateAll();
                                } else if (stmt->size == 2) {
                                    int32_t off2 = (int32_t)t - (int32_t)(stmt->address + 2);
                                    e.emitInstruction(stmt->instr.mnemonic, AddressingMode::RELATIVE, (uint32_t)(uint8_t)(int8_t)off2, true);
                                } else {
                                    // 16-bit relative: offset from PC+2 per MEGA65 book
                                    int32_t off3 = (int32_t)t - (int32_t)(stmt->address + 2);
                                    e.emitInstruction(stmt->instr.mnemonic, AddressingMode::RELATIVE16, (uint32_t)(uint16_t)(int16_t)off3, true);
                                }
                            }
                        }
                    }
                    if (stmt->instr.mnemonic == "rts" || stmt->instr.mnemonic == "rtn" || stmt->instr.mnemonic == "rti") isDeadCode = true;
                }
            } else if (stmt->type == AssemblerParser::Statement::DIRECTIVE) {
                if (!isDeadCode || stmt->dir.name == "org") {
                    if (stmt->dir.name == "var") {
                        if (stmt->dir.varType == Directive::ASSIGN) {
                            uint32_t val = parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                            parser->symbolTable[stmt->dir.varName].value = val;
                        } else if (stmt->dir.varType == Directive::INC) parser->symbolTable[stmt->dir.varName].value++;
                        else if (stmt->dir.varType == Directive::DEC) parser->symbolTable[stmt->dir.varName].value--;
                    }
                    else if (stmt->dir.name == "local") {
                        if (stmt->dir.varType == Directive::ASSIGN) {
                            uint32_t val = parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                            auto& sym = parser->symbolTable[stmt->dir.varName];
                            sym.value = val;
                            sym.frameOffset = (int)val;
                            sym.isFrameRelative = true;
                        }
                    }
                    else if (stmt->dir.name == "cleanup") { if (currentPass2Proc) currentPass2Proc->totalParamSize += parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix); }
                    else if (stmt->dir.name == "byte") for (const auto& a : stmt->dir.arguments) e.emitByte((uint8_t)parseNumericLiteral(a));
                    else if (stmt->dir.name == "word") {
                        for (const auto& a : stmt->dir.arguments) {
                            // Try numeric first, fall back to symbol with relocation
                            try {
                                e.emitWord((uint16_t)parseNumericLiteral(a));
                            } catch (...) {
                                Symbol* sym = parser->resolveSymbol(a, stmt->scopePrefix);
                                uint16_t val = sym ? (uint16_t)sym->value : 0;
                                e.emitWordReloc(a, val);
                            }
                        }
                    }
                    else if (stmt->dir.name == "dword" || stmt->dir.name == "long") for (const auto& a : stmt->dir.arguments) { uint32_t v = parseNumericLiteral(a); e.emitWord(v & 0xFFFF); e.emitWord(v >> 16); }
                    else if (stmt->dir.name == "float") for (const auto& a : stmt->dir.arguments) { double v = std::stod(a); std::vector<uint8_t> enc = encodeFloat(v); for (uint8_t eb : enc) e.emitByte(eb); }
                    else if (stmt->dir.name == "text") { if (!stmt->dir.arguments.empty()) for (char c : stmt->dir.arguments[0]) e.emitByte(toPetscii(c)); }
                    else if (stmt->dir.name == "ascii") { if (!stmt->dir.arguments.empty()) for (char c : stmt->dir.arguments[0]) e.emitByte((uint8_t)c); }
                    else if (stmt->dir.name == "screencode") { if (!stmt->dir.arguments.empty()) for (char c : stmt->dir.arguments[0]) e.emitByte(toScreencode(c)); }
                    else if (stmt->dir.name == "res") {
                        uint32_t count = parser->evaluateExpressionAt(stmt->dir.tokenIndex, stmt->scopePrefix);
                        uint8_t fill = 0;
                        if (stmt->dir.arguments.size() > 1) fill = (uint8_t)parseNumericLiteral(stmt->dir.arguments[1]);
                        for (uint32_t i = 0; i < count; ++i) e.emitByte(fill);
                    }
                    else if (stmt->dir.name == "array") {
                        uint32_t count = stmt->dir.arguments.empty() ? 0 : (uint32_t)parseNumericLiteral(stmt->dir.arguments[0]);
                        for (uint32_t i = 0; i < count; ++i) e.emitByte(0);
                    }
                    else if (stmt->dir.name == "align" || stmt->dir.name == "balign") {
                        uint8_t fill = 0;
                        if (stmt->dir.arguments.size() > 1) fill = (uint8_t)parseNumericLiteral(stmt->dir.arguments[1]);
                        for (int i = 0; i < stmt->size; ++i) e.emitByte(fill);
                    }
                    else if (stmt->dir.name == "fillto") {
                        uint8_t fill = 0;
                        if (stmt->dir.arguments.size() > 1) fill = (uint8_t)parseNumericLiteral(stmt->dir.arguments[1]);
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
            throw std::runtime_error("line " + std::to_string(stmt->line) + ": " + ex.what());
          }
        }
    }
}
