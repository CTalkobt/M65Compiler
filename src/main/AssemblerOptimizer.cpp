#include "AssemblerOptimizer.hpp"
#include "AssemblerParser.hpp"
#include "MachineState.hpp"
#include <algorithm>
#include <iostream>
#include <map>

bool AssemblerOptimizer::optimize(AssemblerParser* parser, bool verbose) {
    bool changed = false;

    // Report helper: emits optimization action to stderr when verbose
    auto report = [&](const char* pass, const AssemblerParser::Statement* s, const std::string& action) {
        if (!verbose) return;
        std::cerr << "opt: ";
        if (!s->sourceFile.empty()) std::cerr << s->sourceFile << ":";
        if (s->sourceLine > 0) std::cerr << s->sourceLine << ": ";
        else if (s->line > 0) std::cerr << "asm:" << s->line << ": ";
        std::cerr << "[" << pass << "] " << action << std::endl;
    };

    MachineState ms;

    // Parse a numeric operand string to a value.
    // Handles decimal, $hex, %binary. Returns true if parseable.
    auto parseNum = [](const std::string& op, int64_t& val) -> bool {
        if (op.empty()) return false;
        try {
            if (op[0] == '$') {
                val = std::stol(op.substr(1), nullptr, 16);
                return true;
            } else if (op[0] == '%') {
                val = std::stol(op.substr(1), nullptr, 2);
                return true;
            } else if (op[0] >= '0' && op[0] <= '9') {
                val = std::stol(op, nullptr, 10);
                return true;
            } else if (op[0] == '-' && op.size() > 1) {
                val = std::stol(op, nullptr, 10);
                return true;
            }
        } catch (...) {}
        return false;
    };

    // Map mnemonic suffix to RegId: LDA→A, LDX→X, etc.
    auto loadRegId = [](const std::string& m) -> RegId {
        char last = m.back();
        if (last == 'A') return REG_A;
        if (last == 'X') return REG_X;
        if (last == 'Y') return REG_Y;
        if (last == 'Z') return REG_Z;
        return REG_A;
    };

    auto storeRegId = [](const std::string& m) -> RegId {
        char last = m.back();
        if (last == 'A') return REG_A;
        if (last == 'X') return REG_X;
        if (last == 'Y') return REG_Y;
        if (last == 'Z') return REG_Z;
        return REG_A;
    };

    // Track STW.SP frame variable stores for dead-store elimination (symbolic, not address-based)
    std::map<std::string, std::string> stackVarLastValue;

    // Build proc-name → clobber info lookup from parsed procedures.
    struct ProcClobbers {
        uint8_t regMask = 0xFF;
        bool known = false;
    };
    std::map<std::string, ProcClobbers> procClobbers;
    for (const auto& [addr, ctx] : parser->getProcedures()) {
        if (ctx && ctx->hasFuncAttrs) {
            procClobbers[ctx->name] = {ctx->regClobbersMask, true};
        }
    }

    // --- Pass 1: Tail-call optimization (JSR + RTS → JMP) ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;
        if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = s->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);

        if (m == "JSR" && s->instr.mode == AddressingMode::ABSOLUTE) {
            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            if (!next->label.empty()) continue;
            if (next->type != AssemblerParser::Statement::INSTRUCTION) continue;

            std::string nm = next->instr.mnemonic;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);

            if ((nm == "RTS" || nm == "RTN") && next->instr.mode == AddressingMode::IMPLIED) {
                report("tail-call", s, "JSR " + s->instr.operand + " + RTS → JMP (saved 1 byte)");
                s->instr.mnemonic = "jmp";
                next->deleted = true;
                next->size = 0;
                changed = true;
            }
        }
    }

    // --- Pass 1b: Eliminate no-op branches (bra to next instruction) ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;
        if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = s->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);

        if (m == "BRA" && s->instr.mode == AddressingMode::RELATIVE) {
            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            if (!next->label.empty()) {
                const auto& lbl = next->label;
                const auto& op = s->instr.operand;
                bool match = (lbl == op);
                if (!match && !s->scopePrefix.empty()) {
                    match = (lbl == s->scopePrefix + op);
                }
                if (!match) {
                    auto colonPos = lbl.rfind(':');
                    if (colonPos != std::string::npos) {
                        match = (lbl.substr(colonPos + 1) == op ||
                                 lbl.substr(colonPos) == ":" + op);
                    }
                }
                if (match) {
                    report("no-op-bra", s, "BRA to next instruction eliminated (saved 2 bytes)");
                    s->deleted = true;
                    s->size = 0;
                    changed = true;
                }
            }
        }
    }

    // --- Pass 1c: Invert conditional branch over BRA ---
    {
        auto invertBranch = [](const std::string& mnemonic) -> std::string {
            std::string m = mnemonic;
            std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            if (m == "BEQ") return "bne";
            if (m == "BNE") return "beq";
            if (m == "BCC") return "bcs";
            if (m == "BCS") return "bcc";
            if (m == "BMI") return "bpl";
            if (m == "BPL") return "bmi";
            if (m == "BVC") return "bvs";
            if (m == "BVS") return "bvc";
            return "";
        };

        for (size_t i = 0; i < parser->statements.size(); ++i) {
            auto* s = parser->statements[i].get();
            if (s->deleted) continue;
            if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;
            std::string inv = invertBranch(s->instr.mnemonic);
            if (inv.empty()) continue;
            std::string mUp = s->instr.mnemonic;
            std::transform(mUp.begin(), mUp.end(), mUp.begin(), ::toupper);
            if (mUp != "BEQ" && mUp != "BNE" && mUp != "BCC" && mUp != "BCS" &&
                mUp != "BMI" && mUp != "BPL" && mUp != "BVC" && mUp != "BVS") continue;

            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            if (!next->label.empty()) continue;
            if (next->type != AssemblerParser::Statement::INSTRUCTION) continue;

            std::string nm = next->instr.mnemonic;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
            if (nm != "BRA") continue;

            size_t k = j + 1;
            while (k < parser->statements.size() && parser->statements[k]->deleted) ++k;
            if (k >= parser->statements.size()) continue;

            auto* afterBra = parser->statements[k].get();
            if (afterBra->label.empty()) continue;

            const auto& condTarget = s->instr.operand;
            const auto& afterLabel = afterBra->label;
            bool match = (condTarget == afterLabel);
            if (!match && !s->scopePrefix.empty())
                match = (afterLabel == s->scopePrefix + condTarget);
            if (!match) {
                auto cp = afterLabel.rfind(':');
                if (cp != std::string::npos)
                    match = (afterLabel.substr(cp + 1) == condTarget);
            }
            if (!match) continue;

            report("branch-invert", s, std::string(s->instr.mnemonic) + " + BRA → " + inv + " (saved 2 bytes)");
            s->instr.mnemonic = inv;
            s->instr.operand = next->instr.operand;
            s->instr.operandTokenIndex = next->instr.operandTokenIndex;
            s->exprTokenIndex = next->exprTokenIndex;
            next->deleted = true;
            next->size = 0;
            changed = true;
        }
    }

    // --- Pass 1d: JMP→BRA conversion when target is within ±127 bytes ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;
        if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = s->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);

        if ((m == "JMP" || m == "JSR") && s->instr.mode == AddressingMode::ABSOLUTE) {
            uint32_t targetAddr = 0;
            bool resolved = false;
            if (s->instr.operandTokenIndex >= 0) {
                try {
                    targetAddr = parser->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix);
                    resolved = true;
                } catch (...) {}
            }
            if (!resolved) continue;

            if (m == "JMP") {
                uint32_t braPC = s->address + 2;
                int32_t offset = (int32_t)targetAddr - (int32_t)braPC;
                if (offset >= -128 && offset <= 127) {
                    report("jmp-bra", s, "JMP → BRA (saved 1 byte, offset " + std::to_string(offset) + ")");
                    s->instr.mnemonic = "bra";
                    s->instr.mode = AddressingMode::RELATIVE;
                    s->size = 2;
                    changed = true;
                }
            } else {
                bool canBSR = resolved;
                if (canBSR && s->instr.operandTokenIndex >= 0) {
                    std::string operand = s->instr.operand;
                    if (operand.empty() && s->instr.operandTokenIndex < (int)parser->tokens.size())
                        operand = parser->tokens[s->instr.operandTokenIndex].value;
                    if (operand.empty() || operand[0] == '$' || operand[0] == '%' ||
                        (operand[0] >= '0' && operand[0] <= '9'))
                        canBSR = false;
                    if (canBSR && (parser->isRelocatableSymbol(operand) ||
                        parser->isRelocatableSymbol(s->scopePrefix + operand)))
                        canBSR = false;
                }
                if (canBSR) {
                    uint32_t bsrPC = s->address + 3;
                    int32_t offset = (int32_t)targetAddr - (int32_t)bsrPC;
                    if (offset >= -32768 && offset <= 32767) {
                        report("jsr-bsr", s, "JSR " + s->instr.operand + " → BSR (position-independent)");
                        s->instr.mnemonic = "bsr";
                        s->instr.mode = AddressingMode::RELATIVE16;
                        changed = true;
                    }
                }
            }
        }
    }

    // --- Pass 2: MachineState-driven register/memory tracking and optimization ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;

        // Barrier: non-local labels reset all knowledge
        if (!s->label.empty() && s->label[0] != '@') {
            ms.invalidateAll();
            stackVarLastValue.clear();
        }

        bool isStackStoreOptimizationCandidate = false;

        // STW.SP redundant store elimination (symbolic frame variable names)
        if (s->type == AssemblerParser::Statement::STW && s->instr.mnemonic == "STW.SP") {
            isStackStoreOptimizationCandidate = true;
            std::string immediateValue = s->instr.operand;
            std::string varName = parser->tokens[s->exprTokenIndex].value;

            if (stackVarLastValue.count(varName) && stackVarLastValue[varName] == immediateValue) {
                report("dead-store", s, "redundant STW.SP " + varName + " eliminated");
                s->deleted = true; s->size = 0; changed = true; continue;
            } else {
                stackVarLastValue[varName] = immediateValue;
            }
        }

        // Invalidate stack variable state for memory-modifying instructions
        if (!isStackStoreOptimizationCandidate) {
            if (s->type == AssemblerParser::Statement::INSTRUCTION) {
                std::string M = s->instr.mnemonic;
                std::transform(M.begin(), M.end(), M.begin(), ::toupper);
                if (M == "STA" || M == "STX" || M == "STY" || M == "STZ" ||
                    M == "JSR" || M == "JMP" || M == "CALL" || M == "BSR" ||
                    M == "RTS" || M == "RTN" || M == "RTI" ||
                    M == "PHA" || M == "PLA" || M == "PHX" || M == "PLX" ||
                    M == "PHY" || M == "PLY" || M == "PHZ" || M == "PLZ" || M == "PHW" ||
                    M == "ADC" || M == "SBC" || M == "AND" || M == "ORA" || M == "EOR" ||
                    M == "ASL" || M == "LSR" || M == "ROL" || M == "ROR" ||
                    M == "DEC" || M == "INC" ||
                    M == "INX" || M == "DEX" || M == "INY" || M == "DEY" || M == "INZ" || M == "DEZ") {
                    stackVarLastValue.clear();
                }
            } else if (s->isSimulatedOp()) {
                stackVarLastValue.clear();
            }
        }

        // Register tracking and optimizations
        if (s->type == AssemblerParser::Statement::INSTRUCTION || s->type == AssemblerParser::Statement::LDW) {
            std::string m;
            if (s->type == AssemblerParser::Statement::LDW) m = "LDAX";
            else if (s->type == AssemblerParser::Statement::STW) m = "STAX";
            else {
                m = s->instr.mnemonic;
                std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            }
            AddressingMode mode = s->instr.mode;
            std::string op = s->instr.operand;

            // Word load tracking (LDW) — invalidates A + second reg
            if (s->type == AssemblerParser::Statement::LDW) {
                ms.invalidateReg(REG_A);
                std::string regStr = op;
                std::transform(regStr.begin(), regStr.end(), regStr.begin(), ::toupper);
                if (regStr == ".AY") ms.invalidateReg(REG_Y);
                else if (regStr == ".AZ") ms.invalidateReg(REG_Z);
                else ms.invalidateReg(REG_X);
                ms.flags.invalidate();
                continue;
            }

            if (s->type == AssemblerParser::Statement::STW) continue;

            // Parse operand to numeric value
            int64_t numVal = 0;
            bool hasNumVal = parseNum(op, numVal);
            bool isImm = (mode == AddressingMode::IMMEDIATE);
            bool isZP = (mode == AddressingMode::BASE_PAGE);
            bool isAbs = (mode == AddressingMode::ABSOLUTE);

            // --- CMP #0 elimination ---
            if (m == "CMP" && isImm && hasNumVal && numVal == 0) {
                if (ms.flags.flagsReflect(REG_A)) {
                    size_t j = i + 1;
                    while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
                    bool safeToElim = true;
                    if (j < parser->statements.size()) {
                        auto* next = parser->statements[j].get();
                        if (next->type == AssemblerParser::Statement::INSTRUCTION) {
                            std::string nm = next->instr.mnemonic;
                            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
                            if (nm == "BCC" || nm == "BCS") safeToElim = false;
                        }
                    }
                    if (safeToElim) {
                        report("cmp-elim", s, "CMP #0 eliminated (flags already set from load)");
                        s->deleted = true; s->size = 0; changed = true; continue;
                    }
                }
            }

            // --- Redundant load elimination (MachineState-based) ---
            // A load is redundant if the register already holds the same value:
            //   1. Same constant (LDA #5 when A is CONSTANT(5))
            //   2. Same memory source (LDA $20 when A is SAME_AS_MEM($20) and mem[$20] unchanged)
            //   3. Store-forwarding (STA $20; ... LDA $20 when A still holds what was stored)
            if (m == "LDA" || m == "LDX" || m == "LDY" || m == "LDZ") {
                RegId r = loadRegId(m);
                bool redundant = false;

                if (isImm && hasNumVal) {
                    // Check: register already holds this constant?
                    redundant = ms.reg[r].isConst(numVal & 0xFF);
                } else if (isZP && hasNumVal) {
                    uint8_t zpAddr = (uint8_t)(numVal & 0xFF);
                    // Check: register mirrors this ZP address and memory unchanged?
                    if (ms.reg[r].isSameAsMem(zpAddr)) {
                        redundant = true;
                    }
                    // Check: memory mirrors this register (store-forwarding)?
                    // After STA $24, zpMem[$24] = SAME_AS_REG(A). A subsequent LDA $24
                    // is redundant because A still holds the value that was stored.
                    else if (ms.getZP(zpAddr).isSameAs(r)) {
                        redundant = true;
                    }
                    // Check: both register and memory hold the same known constant?
                    else if (ms.reg[r].isConst() && ms.getZP(zpAddr).isConst()
                             && ms.reg[r].constVal == ms.getZP(zpAddr).constVal) {
                        redundant = true;
                    }
                } else if (isAbs && hasNumVal) {
                    uint16_t addr = (uint16_t)(numVal & 0xFFFF);
                    // For absolute addresses in ZP range, check zpMem
                    if (addr < 256) {
                        if (ms.reg[r].isSameAsMem((uint8_t)addr)) redundant = true;
                        else if (ms.getZP((uint8_t)addr).isSameAs(r)) redundant = true;
                        else if (ms.reg[r].isConst() && ms.getZP((uint8_t)addr).isConst()
                                 && ms.reg[r].constVal == ms.getZP((uint8_t)addr).constVal)
                            redundant = true;
                    } else {
                        auto memInfo = ms.getAbsMem(addr);
                        if (memInfo.isSameAs(r)) redundant = true;
                        else if (ms.reg[r].isConst() && memInfo.isConst()
                            && ms.reg[r].constVal == memInfo.constVal)
                            redundant = true;
                    }
                }

                if (redundant) {
                    report("redundant-load", s, m + " " + op + " eliminated (" + m.substr(2) + " already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }

            // --- MachineState updates ---
            if (m == "LDA" || m == "LDX" || m == "LDY" || m == "LDZ") {
                RegId r = loadRegId(m);
                if (isImm && hasNumVal) {
                    ms.setConst(r, numVal & 0xFF);
                } else if (isZP && hasNumVal) {
                    ms.setRegFromZP(r, (uint8_t)(numVal & 0xFF));
                } else {
                    ms.invalidateReg(r);
                    ms.flags.setNZ(r);
                }
            }
            else if (m == "STA" || m == "STX" || m == "STY" || m == "STZ") {
                RegId r = storeRegId(m);
                if (isZP && hasNumVal) {
                    ms.storeZP((uint8_t)(numVal & 0xFF), r);
                } else if (isAbs && hasNumVal) {
                    uint16_t addr = (uint16_t)(numVal & 0xFFFF);
                    if (addr < 256) ms.storeZP((uint8_t)addr, r);
                    else ms.storeAbs(addr, r);
                }
                // Stores don't modify the source register or flags
            }

            // --- Transfer instructions ---
            else if (m == "TAX") { ms.setTransfer(REG_X, REG_A); }
            else if (m == "TXA") { ms.setTransfer(REG_A, REG_X); }
            else if (m == "TAY") { ms.setTransfer(REG_Y, REG_A); }
            else if (m == "TYA") { ms.setTransfer(REG_A, REG_Y); }
            else if (m == "TAZ") { ms.setTransfer(REG_Z, REG_A); }
            else if (m == "TZA") { ms.setTransfer(REG_A, REG_Z); }
            else if (m == "TSX") { ms.setTransfer(REG_X, REG_SP); }

            // --- Control flow ---
            else if (m == "JMP") { ms.invalidateAll(); }

            // --- JSR: selective invalidation with clobber info ---
            else if (m == "JSR" && mode == AddressingMode::ABSOLUTE) {
                auto it = procClobbers.find(op);
                if (it != procClobbers.end() && it->second.known) {
                    uint8_t mask = it->second.regMask;
                    if (mask & 0x01) ms.invalidateReg(REG_A);
                    if (mask & 0x02) ms.invalidateReg(REG_X);
                    if (mask & 0x04) ms.invalidateReg(REG_Y);
                    if (mask & 0x08) ms.invalidateReg(REG_Z);
                } else {
                    ms.invalidateAll();
                }
                ms.flags.invalidate();
                ms.invalidateAllMem();
            }
            else if (m == "JSR" || m == "CALL" || m == "BSR") {
                ms.invalidateAll();
            }

            // --- Push/Pull ---
            else if (m == "PHA" || m == "PHX" || m == "PHY" || m == "PHZ" || m == "PHW") {
                ms.spModified();
            }
            else if (m == "PLA") { ms.spModified(); ms.invalidateReg(REG_A); }
            else if (m == "PLX") { ms.spModified(); ms.invalidateReg(REG_X); }
            else if (m == "PLY") { ms.spModified(); ms.invalidateReg(REG_Y); }
            else if (m == "PLZ") { ms.spModified(); ms.invalidateReg(REG_Z); }

            // --- RTS/RTN/RTI ---
            else if (m == "RTS" || m == "RTN" || m == "RTI") { ms.invalidateAll(); }

            // --- ALU: modifies A and flags ---
            else if (m == "ADC" || m == "SBC" || m == "AND" || m == "ORA" || m == "EOR") {
                ms.invalidateReg(REG_A);
                ms.flags.invalidate();
            }
            else if (m == "ASL" || m == "LSR" || m == "ROL" || m == "ROR") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    ms.invalidateReg(REG_A);
                } else if (isZP && hasNumVal) {
                    ms.invalidateZP((uint8_t)(numVal & 0xFF));
                } else if (isAbs && hasNumVal) {
                    ms.invalidateAbs((uint16_t)(numVal & 0xFFFF));
                }
                ms.flags.invalidate();
            }
            else if (m == "INC" || m == "DEC") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    ms.invalidateReg(REG_A);
                } else if (isZP && hasNumVal) {
                    ms.invalidateZP((uint8_t)(numVal & 0xFF));
                } else if (isAbs && hasNumVal) {
                    ms.invalidateAbs((uint16_t)(numVal & 0xFFFF));
                }
                ms.flags.invalidate();
            }
            else if (m == "INX" || m == "DEX") { ms.invalidateReg(REG_X); ms.flags.invalidate(); }
            else if (m == "INY" || m == "DEY") { ms.invalidateReg(REG_Y); ms.flags.invalidate(); }
            else if (m == "INZ" || m == "DEZ") { ms.invalidateReg(REG_Z); ms.flags.invalidate(); }

            // --- Compare: flags only ---
            else if (m == "CMP" || m == "CPX" || m == "CPY" || m == "CPZ") {
                if (isImm && hasNumVal) {
                    RegId cmpReg = (m == "CMP") ? REG_A : (m == "CPX") ? REG_X : (m == "CPY") ? REG_Y : REG_Z;
                    if (ms.reg[cmpReg].isConst()) {
                        uint8_t rv = (uint8_t)(ms.reg[cmpReg].constVal & 0xFF);
                        uint8_t v = (uint8_t)(numVal & 0xFF);
                        ms.flags.z = (rv == v) ? FlagState::F_SET : FlagState::F_CLEAR;
                        ms.flags.c = (rv >= v) ? FlagState::F_SET : FlagState::F_CLEAR;
                        uint8_t diff = rv - v;
                        ms.flags.n = (diff & 0x80) ? FlagState::F_SET : FlagState::F_CLEAR;
                        ms.flags.nzSourceReg = -1;
                    } else {
                        ms.flags.invalidate();
                    }
                } else {
                    ms.flags.invalidate();
                }
            }

            // --- Clear: sets register to 0 ---
            else if (m == "CLA") { ms.setConst(REG_A, 0); }
            else if (m == "CLX") { ms.setConst(REG_X, 0); }
            else if (m == "CLY") { ms.setConst(REG_Y, 0); }
            else if (m == "CLZ") { ms.setConst(REG_Z, 0); }

            // --- CLC/SEC ---
            else if (m == "CLC") { ms.flags.setCarry(false); }
            else if (m == "SEC") { ms.flags.setCarry(true); }

            // Branches: flags/registers valid for fall-through. Labels handle merge points.

        } else if (s->isSimulatedOp()) {
            // Instead of blanket invalidateAll, track what each simulated op clobbers.
            // Most 16-bit ops leave result in A:X and preserve Y/Z/memory.
            using ST = AssemblerParser::Statement;
            auto stype = s->type;
            if (stype == ST::ADD16 || stype == ST::SUB16 || stype == ST::ADDS16 || stype == ST::SUBS16 ||
                stype == ST::AND16 || stype == ST::ORA16 || stype == ST::EOR16 ||
                stype == ST::NEG16 || stype == ST::NOT16 || stype == ST::ABS16 ||
                stype == ST::MUL || stype == ST::DIV ||
                stype == ST::LSL16 || stype == ST::LSR16 || stype == ST::ASR16 ||
                stype == ST::ROL16 || stype == ST::ROR16 ||
                stype == ST::CMP16 || stype == ST::CMP_S16 ||
                stype == ST::CHKZERO8 || stype == ST::CHKZERO16 ||
                stype == ST::CHKNONZERO8 || stype == ST::CHKNONZERO16 ||
                stype == ST::SELECT || stype == ST::SXT8) {
                // Result in A:X, Y/Z preserved, flags clobbered
                ms.invalidateReg(REG_A);
                ms.invalidateReg(REG_X);
                ms.flags.invalidate();
            } else if (stype == ST::LDW || stype == ST::LDAX || stype == ST::LDAX_FP) {
                // Load into A:X (or A:Y/A:Z for variants)
                ms.invalidateReg(REG_A);
                ms.invalidateReg(REG_X);
                ms.flags.invalidate();
            } else if (stype == ST::STW || stype == ST::STAX || stype == ST::STAX_FP) {
                // Store from A:X — doesn't clobber registers, but may modify memory
                ms.invalidateAllMem();
            } else if (stype == ST::INC_FP || stype == ST::DEC_FP ||
                       stype == ST::INC16_FP || stype == ST::DEC16_FP) {
                // Frame inc/dec: clobbers flags, may clobber A (dec.16f), preserves X/Y/Z
                ms.invalidateReg(REG_A);
                ms.flags.invalidate();
                ms.invalidateAllStack();
            } else if (stype == ST::STACK_INC || stype == ST::STACK_DEC ||
                       stype == ST::STACK_INC8 || stype == ST::STACK_DEC8) {
                // Stack inc/dec: clobbers A/X, flags
                ms.invalidateReg(REG_A);
                ms.invalidateReg(REG_X);
                ms.flags.invalidate();
            } else {
                // Unknown simulated op — conservatively invalidate all
                ms.invalidateAll();
            }
            stackVarLastValue.clear();
        }
    }

    // --- Pass 3: Function tail deduplication (with re-scanning for convergence) ---
    // Iteratively find and eliminate duplicate instruction sequences at function tails.
    // Re-scans after each pass because earlier optimizations may:
    // - Enable new deduplication opportunities
    // - Change branch sizes, affecting which sequences are worth extracting
    {
        bool tailDedupChanged = true;
        int dedupPass = 0;
        const int MAX_DEDUP_PASSES = 10;

        while (tailDedupChanged && dedupPass < MAX_DEDUP_PASSES) {
            tailDedupChanged = false;
            dedupPass++;

            struct TailInfo {
                size_t startIdx;
                size_t endIdx;
                std::vector<size_t> instrIndices;
                std::string sig;
                size_t tailSize;
            };
            std::map<std::string, std::vector<TailInfo>> tailsBySignature;

        // Find all procedure end points and extract tail sequences
        for (size_t i = 0; i < parser->statements.size(); ++i) {
            auto* s = parser->statements[i].get();
            if (s->deleted) continue;
            if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;
            std::string m = s->instr.mnemonic;
            std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            if (m != "ENDPROC") continue;

            // Walk backward to find the start of the tail sequence.
            // Skip over directives (like .func_flags, .reg_clobbers), then collect instructions.
            std::vector<size_t> tailIndices;
            size_t j = i - 1;

            // First, skip directives immediately before endproc
            while ((int)j >= 0) {
                auto* prev = parser->statements[j].get();
                if (prev->deleted) {
                    --j;
                    continue;
                }
                if (prev->type == AssemblerParser::Statement::DIRECTIVE) {
                    --j;
                    continue;
                }
                break;
            }

            // Now collect consecutive instructions until we hit a non-instruction
            while ((int)j >= 0) {
                auto* prev = parser->statements[j].get();
                if (prev->deleted) {
                    --j;
                    continue;
                }
                // Stop at labels (they're targets and shouldn't be part of the tail)
                if (!prev->label.empty()) break;
                // Stop at directives
                if (prev->type == AssemblerParser::Statement::DIRECTIVE) break;
                // Only collect instructions
                if (prev->type != AssemblerParser::Statement::INSTRUCTION) break;
                tailIndices.push_back(j);
                --j;
            }

            // Reverse to get tail in correct order (backward walk reversed)
            std::reverse(tailIndices.begin(), tailIndices.end());

            // Only consider tails with 2+ instructions (worth deduplicating)
            if (tailIndices.size() < 2) continue;

            // Build signature and compute size
            std::string sig;
            size_t tailSize = 0;
            for (size_t idx : tailIndices) {
                auto* instr = parser->statements[idx].get();
                std::string m = instr->instr.mnemonic;
                std::transform(m.begin(), m.end(), m.begin(), ::toupper);
                sig += m + "|" + std::to_string((int)instr->instr.mode) + "|" + instr->instr.operand + "|";
                tailSize += instr->size;
            }

            TailInfo info{tailIndices.front(), i, tailIndices, sig, tailSize};
            tailsBySignature[sig].push_back(info);
            }

            // Deduplicate exact matches (one signature per pass)
            for (auto& [sig, tails] : tailsBySignature) {
                if (tails.size() < 2) continue;
                if (tails[0].tailSize < 3) continue;

                size_t savedBytes = (tails[0].tailSize - 2) * (tails.size() - 1);
                if (savedBytes < 2) continue;

                std::string sharedLabel = "__tail_" + std::to_string(reinterpret_cast<uintptr_t>(&sig));
                bool isFirst = true;

                for (auto& tail : tails) {
                    if (isFirst) {
                        auto labelStmt = std::make_unique<AssemblerParser::Statement>();
                        labelStmt->type = AssemblerParser::Statement::DIRECTIVE;
                        labelStmt->label = sharedLabel;
                        labelStmt->line = parser->statements[tail.startIdx]->line;
                        parser->statements.insert(
                            parser->statements.begin() + tail.startIdx,
                            std::move(labelStmt)
                        );
                        isFirst = false;
                    } else {
                        auto* firstInstr = parser->statements[tail.startIdx].get();
                        auto braStmt = std::make_unique<AssemblerParser::Statement>();
                        braStmt->type = AssemblerParser::Statement::INSTRUCTION;
                        braStmt->instr.mnemonic = "bra";
                        braStmt->instr.mode = AddressingMode::RELATIVE;
                        braStmt->instr.operand = sharedLabel;
                        braStmt->size = 2;
                        braStmt->line = firstInstr->line;
                        braStmt->sourceFile = firstInstr->sourceFile;
                        braStmt->sourceLine = firstInstr->sourceLine;

                        for (size_t idx : tail.instrIndices) {
                            parser->statements[idx]->deleted = true;
                            parser->statements[idx]->size = 0;
                        }

                        parser->statements.insert(
                            parser->statements.begin() + tail.startIdx,
                            std::move(braStmt)
                        );

                        report("tail-dedup", parser->statements[tail.startIdx].get(),
                               "tail → bra " + sharedLabel + " (saved " + std::to_string(tails[0].tailSize - 2) + " bytes)");
                        changed = true;
                        tailDedupChanged = true;
                    }
                }
                break;  // Only process one signature per pass
            }
        }
    }

    return changed;
}
