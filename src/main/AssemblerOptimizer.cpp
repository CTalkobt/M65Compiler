#include "AssemblerOptimizer.hpp"
#include "AssemblerParser.hpp"
#include "MachineState.hpp"
#include "OpEffect.hpp"
#include "O45Reader.hpp"
#include "O45Types.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <fstream>

bool AssemblerOptimizer::optimize(AssemblerParser* parser, bool verbose, bool traceMachState) {
    return optimizeInternal(parser, nullptr, verbose, traceMachState, parser->optimizationLevel);
}

bool AssemblerOptimizer::optimizeInternal(
    AssemblerParser* parser,
    const std::map<std::string, ExternalFuncInfo>* externalFuncs,
    bool verbose,
    bool traceMachState,
    int optimizationLevel
) {
    bool changed = false;
    int tailCounter = 0;

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

    // 1. Add local procedures with function attributes
    for (const auto& [addr, ctx] : parser->getProcedures()) {
        if (ctx && ctx->hasFuncAttrs) {
            procClobbers[ctx->name] = {ctx->regClobbersMask, true};
        }
    }

    // 2. Phase 5: Add external functions from linked object files
    if (externalFuncs) {
        for (const auto& [funcName, extFunc] : *externalFuncs) {
            procClobbers[funcName] = {extFunc.regMask, true};
            if (verbose) {
                std::cerr << "opt: loaded external " << funcName << " clobber mask $"
                          << std::hex << (int)extFunc.regMask << std::dec << std::endl;
            }
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
                bool canBSR = resolved && optimizationLevel > 0;  // Only optimize JSR to BSR if optimization is enabled
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

    // --- Pass 1c: Report nested loop multiplication pattern (info only) ---
    // Pattern: LDAX + MUL.16 .AX, #N appears multiple times with same source in nested loops
    // This is a nested loop pattern optimization opportunity that would require code generation changes
    // Example: grid[r][c] in nested loop where r*WIDTH is computed for each iteration
    // Future optimization: Cache r*WIDTH once before inner loop, reuse within loop

    // For now, we detect and report the pattern but don't optimize (complex to do at assembler level)
    // The code generator would need to recognize this pattern and hoist the multiplication out of inner loop

    // --- Pass 2: MachineState-driven register/memory tracking and optimization ---
    bool inNoOptRegion = false;

    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;

        // Check for no-optimization directives
        if (s->type == AssemblerParser::Statement::DIRECTIVE) {
            std::string dir = s->dir.name;
            std::transform(dir.begin(), dir.end(), dir.begin(), ::tolower);
            if (dir == "noopt_start") {
                inNoOptRegion = true;
            } else if (dir == "noopt_end") {
                inNoOptRegion = false;
            }
            // Directives don't need optimization, skip to next statement
            continue;
        }

        // Barrier: non-local labels reset all knowledge
        if (!s->label.empty() && s->label[0] != '@') {
            ms.invalidateAll();
            stackVarLastValue.clear();
        }

        // Skip all optimizations for statements in no-optimize regions
        if (inNoOptRegion) {
            // Statement will be emitted as-is, unoptimized
            continue;
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

            // --- FCMP peephole: cmp #$FF + beq/bne → bmi/bpl when flags reflect A (Issue #126) ---
            // FCMP returns -1($FF)/0/1 in A. When N/Z flags already reflect A
            // (from a prior LDA), the CMP #$FF can be eliminated:
            // cmp #$FF; beq → bmi (N=1 means A=$FF)
            // cmp #$FF; bne → bpl (N=0 means A=$00 or $01)
            // DISABLED: This optimization causes assembler errors when BMI/BPL are generated
            // with absolute addressing, which they don't support (relative-only branches).
            // TODO: Re-enable only when addressing modes can be guaranteed to be relative.
            if (false && m == "CMP" && isImm && hasNumVal && numVal == 255 && ms.flags.flagsReflect(REG_A)) {
                size_t j = i + 1;
                while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
                if (j < parser->statements.size()) {
                    auto* next = parser->statements[j].get();
                    if (next->type == AssemblerParser::Statement::INSTRUCTION) {
                        std::string nm = next->instr.mnemonic;
                        std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
                        if (nm == "BEQ") {
                            report("fcmp-opt", s, "CMP #$FF + BEQ → BMI");
                            s->deleted = true; s->size = 0;
                            next->instr.mnemonic = "BMI";
                            changed = true; continue;
                        }
                        if (nm == "BNE") {
                            report("fcmp-opt", s, "CMP #$FF + BNE → BPL");
                            s->deleted = true; s->size = 0;
                            next->instr.mnemonic = "BPL";
                            changed = true; continue;
                        }
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

                // Never eliminate LDY #0 — it's used before indirect indexed addressing and must be retained
                if (m == "LDY" && isImm && hasNumVal && (numVal & 0xFF) == 0) {
                    redundant = false;
                } else if (isImm && hasNumVal) {
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

                // CRITICAL: Don't eliminate loads immediately before control-flow change
                // Loads before branches, labels, or calls are CRITICAL because they
                // establish register state at control-flow merge points. Eliminating
                // them breaks Bug #183 where return value loads were removed before
                // branches to common epilogue.
                // CRITICAL: Check if load comes immediately before control-flow instruction.
                // Look ahead up to 3 statements (e.g., LDA $28; LDX $29; BRA @label)
                bool isBeforeCFChange = false;
                for (size_t lookahead = 1; lookahead <= 3 && i + lookahead < parser->statements.size(); ++lookahead) {
                    auto* nextStmt = parser->statements[i + lookahead].get();
                    if (!nextStmt || nextStmt->deleted) continue;

                    if (!nextStmt->label.empty()) {
                        isBeforeCFChange = true;
                        break;
                    } else if (nextStmt->type == AssemblerParser::Statement::INSTRUCTION) {
                        const std::string& mn = nextStmt->instr.mnemonic;
                        if (mn == "BRA" || mn == "BEQ" || mn == "BNE" || mn == "BMI" ||
                            mn == "BPL" || mn == "BCS" || mn == "BCC" || mn == "BVS" ||
                            mn == "BVC" || mn == "RTS" || mn == "RTI" || mn == "JMP" ||
                            mn == "JSR" || mn == "BRL" || mn == "JML" || mn == "RTL") {
                            isBeforeCFChange = true;
                            break;
                        }
                    } else {
                        // Non-instruction, non-label statement - keep searching
                        continue;
                    }
                }

                if (redundant && !isBeforeCFChange) {
                    report("redundant-load", s, m + " " + op + " eliminated (" + m.substr(2) + " already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }

            // --- Store-load pair elimination ---
            // Pattern: STA $zp; ...; LDA $zp when the value is only used for testing
            // If LDA $zp is immediately followed by a branch (BNE, BEQ, etc.) and
            // the stored ZP location is not modified between store and load, eliminate both.
            if (m == "STA" && isZP && hasNumVal) {
                uint8_t zpAddr = (uint8_t)(numVal & 0xFF);
                size_t j = i + 1;
                bool foundLoad = false;
                bool zpModified = false;

                // Scan ahead for LDA $zp within next 3-5 instructions
                for (size_t k = 0; k < 5 && j < parser->statements.size(); ++k, ++j) {
                    auto* next = parser->statements[j].get();
                    if (next->deleted) { ++k; continue; }
                    if (next->type != AssemblerParser::Statement::INSTRUCTION) break;

                    std::string nm = next->instr.mnemonic;
                    std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);

                    // Check if this instruction modifies our ZP address
                    if ((nm == "STA" || nm == "STX" || nm == "STY" || nm == "STZ") &&
                        next->instr.mode == AddressingMode::BASE_PAGE) {
                        int64_t modAddr = 0;
                        if (parseNum(next->instr.operand, modAddr) && (uint8_t)(modAddr & 0xFF) == zpAddr) {
                            zpModified = true;
                            break;
                        }
                    }

                    // Check if this is LDA $zp (same address)
                    if (nm == "LDA" && next->instr.mode == AddressingMode::BASE_PAGE) {
                        int64_t loadAddr = 0;
                        if (parseNum(next->instr.operand, loadAddr) && (uint8_t)(loadAddr & 0xFF) == zpAddr) {
                            foundLoad = true;
                            // Check if next-next instruction is a test branch
                            size_t m_idx = j + 1;
                            while (m_idx < parser->statements.size() && parser->statements[m_idx]->deleted) ++m_idx;
                            if (m_idx < parser->statements.size()) {
                                auto* afterLoad = parser->statements[m_idx].get();
                                if (afterLoad->type == AssemblerParser::Statement::INSTRUCTION) {
                                    std::string afterNm = afterLoad->instr.mnemonic;
                                    std::transform(afterNm.begin(), afterNm.end(), afterNm.begin(), ::toupper);
                                    // Only optimize if followed by a branch that tests the loaded value
                                    if (afterNm == "BNE" || afterNm == "BEQ" || afterNm == "BMI" || afterNm == "BPL" ||
                                        afterNm == "BCS" || afterNm == "BCC" || afterNm == "BVS" || afterNm == "BVC") {
                                        // Found the pattern: STA $zp; LDA $zp; Bxx
                                        // Eliminate the store and load (keep the branch)
                                        int s_size = s->size;
                                        int n_size = next->size;
                                        s->deleted = true; s->size = 0;
                                        next->deleted = true; next->size = 0;
                                        report("store-load-pair", s,
                                               "STA/LDA $" + std::string(1, '0' + (zpAddr >> 4)) + std::string(1, "0123456789ABCDEF"[zpAddr & 0xF]) +
                                               " pair eliminated before " + afterNm + " (saved " + std::to_string(s_size + n_size) + " bytes, was: " +
                                               std::to_string(s_size) + "+" + std::to_string(n_size) + ")");
                                        changed = true;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }

            // --- Preserve X=SP optimization: convert LDX #imm / STX zp to LDA #imm / STA zp ---
            // When X holds SP (for stack-relative addressing), avoid clobbering it with
            // temporary loads. If we're loading a small immediate into X just to save it to
            // a scratch ZP, use A instead to keep X = SP for upcoming stack operations.
            if (m == "LDX" && isImm && hasNumVal && ms.xHoldsSP()) {
                uint8_t immVal = (uint8_t)(numVal & 0xFF);
                size_t j = i + 1;
                while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
                if (j < parser->statements.size()) {
                    auto* next = parser->statements[j].get();
                    if (next->type == AssemblerParser::Statement::INSTRUCTION && next->label.empty()) {
                        std::string nextM = next->instr.mnemonic;
                        std::transform(nextM.begin(), nextM.end(), nextM.begin(), ::toupper);
                        int64_t stxAddr = 0;
                        if (nextM == "STX" && next->instr.mode == AddressingMode::BASE_PAGE &&
                            parseNum(next->instr.operand, stxAddr)) {
                            uint8_t zpAddr = (uint8_t)(stxAddr & 0xFF);
                            // Safe to convert LDX #imm / STX zp to LDA #imm / STA zp if:
                            // 1. Nothing between LDX and STX modifies X (just checked: only STX)
                            // 2. STX is to a scratch ZP that won't interfere with future stack ops
                            // Common scratch ZP locations: $02, $03, $04, etc. (typically < $20)
                            if (zpAddr < 0x20) {
                                // Convert: LDX #imm → LDA #imm, STX zp → STA zp
                                s->instr.mnemonic = "LDA";
                                next->instr.mnemonic = "STA";
                                // Update MachineState: A holds the immediate, X still = SP
                                ms.setConst(REG_A, immVal);
                                report("preserve-x-sp", s, "LDX #" + op + " / STX → LDA/STA (preserve X=SP)");
                                changed = true;
                                // Don't process MachineState updates for this instruction;
                                // we've manually set the state above
                                continue;
                            }
                        }
                    }
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
            else if (m == "TSX") {
                // Redundant TSX elimination: skip if X already holds SP
                if (ms.xHoldsSP()) {
                    report("tsx-redundant", s, "TSX eliminated (X already holds SP)");
                    s->deleted = true; s->size = 0; changed = true;
                } else {
                    ms.setTransfer(REG_X, REG_SP);
                }
            }

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
            // Use centralized OpEffect table — keyed by mnemonic
            const OpEffect& effect = getOpEffect(s->instr.mnemonic);
            effect.apply(ms);
            stackVarLastValue.clear();
        }

        // MachineState trace: dump register/flag state after each statement
        if (traceMachState && !s->deleted) {
            std::string loc;
            if (!s->sourceFile.empty()) {
                auto slash = s->sourceFile.rfind('/');
                loc = (slash != std::string::npos ? s->sourceFile.substr(slash + 1) : s->sourceFile);
                if (s->sourceLine > 0) loc += ":" + std::to_string(s->sourceLine);
            } else if (s->line > 0) {
                loc = "asm:" + std::to_string(s->line);
            }
            std::string what;
            if (s->type == AssemblerParser::Statement::INSTRUCTION)
                what = s->instr.mnemonic + " " + s->instr.operand;
            else if (s->isSimulatedOp())
                what = s->instr.mnemonic + " " + s->instr.operand;
            else if (!s->label.empty())
                what = s->label + ":";
            else
                continue; // skip directives/blank lines
            std::cerr << "ms: " << loc << ": " << what << "\n" << ms.dumpLine() << "\n";
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

                std::string sharedLabel = "__tail_opt_dedup_" + std::to_string(tailCounter++);
                bool isFirst = true;

                for (auto& tail : tails) {
                    if (isFirst) {
                        auto* refStmt = parser->statements[tail.startIdx].get();
                        auto labelStmt = std::make_unique<AssemblerParser::Statement>();
                        labelStmt->type = AssemblerParser::Statement::DIRECTIVE;
                        labelStmt->label = sharedLabel;
                        labelStmt->line = refStmt->line;
                        labelStmt->segmentName = refStmt->segmentName;
                        labelStmt->localLabelScope = refStmt->localLabelScope;
                        labelStmt->scopePrefix = refStmt->scopePrefix;
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
                        braStmt->segmentName = firstInstr->segmentName;
                        braStmt->localLabelScope = firstInstr->localLabelScope;
                        braStmt->scopePrefix = firstInstr->scopePrefix;

                        int totalSaved = 0;
                        for (size_t idx : tail.instrIndices) {
                            totalSaved += parser->statements[idx]->size;
                            parser->statements[idx]->deleted = true;
                            parser->statements[idx]->size = 0;
                        }
                        report("tail-dedup-suffix", firstInstr, "tail → bra " + sharedLabel + " (suffix match, saved " + std::to_string(totalSaved) + " bytes)");

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

            // Suffix matching for function tails: if one tail is a suffix of another,
            // the shorter one can jump into the middle of the longer one
            std::vector<TailInfo> allTails;
            for (auto& [sig, tailList] : tailsBySignature) {
                for (auto& tail : tailList) {
                    allTails.push_back(tail);
                }
            }

            for (size_t i = 0; i < allTails.size(); ++i) {
                for (size_t j = 0; j < allTails.size(); ++j) {
                    if (i == j) continue;
                    if (allTails[i].instrIndices.size() >= allTails[j].instrIndices.size()) continue;

                    const auto& shortTail = allTails[i].instrIndices;
                    const auto& longTail = allTails[j].instrIndices;

                    if (longTail.size() < shortTail.size()) continue;

                    // Check if short is suffix of long
                    bool isSuffix = true;
                    for (size_t k = 0; k < shortTail.size(); ++k) {
                        auto* shortInstr = parser->statements[shortTail[k]].get();
                        auto* longInstr = parser->statements[longTail[longTail.size() - shortTail.size() + k]].get();

                        if (shortInstr->instr.mnemonic != longInstr->instr.mnemonic ||
                            shortInstr->instr.mode != longInstr->instr.mode ||
                            shortInstr->instr.operand != longInstr->instr.operand) {
                            isSuffix = false;
                            break;
                        }
                    }

                    if (!isSuffix) continue;

                    size_t savings = allTails[i].tailSize - 2;
                    if (savings < 2) continue;

                    std::string offsetLabel = "__tail_opt_dedup_" + std::to_string(tailCounter++) + "_into";

                    size_t insertPos = longTail[longTail.size() - shortTail.size()];
                    auto* refStmt = parser->statements[insertPos].get();
                    auto labelStmt = std::make_unique<AssemblerParser::Statement>();
                    labelStmt->type = AssemblerParser::Statement::DIRECTIVE;
                    labelStmt->label = offsetLabel;
                    labelStmt->line = refStmt->line;
                    labelStmt->segmentName = refStmt->segmentName;
                    labelStmt->localLabelScope = refStmt->localLabelScope;
                    labelStmt->scopePrefix = refStmt->scopePrefix;
                    parser->statements.insert(
                        parser->statements.begin() + insertPos,
                        std::move(labelStmt)
                    );

                    auto* firstInstr = parser->statements[allTails[i].instrIndices.front()].get();
                    auto braStmt = std::make_unique<AssemblerParser::Statement>();
                    braStmt->type = AssemblerParser::Statement::INSTRUCTION;
                    braStmt->instr.mnemonic = "bra";
                    braStmt->instr.mode = AddressingMode::RELATIVE;
                    braStmt->instr.operand = offsetLabel;
                    braStmt->size = 2;
                    braStmt->line = firstInstr->line;
                    braStmt->sourceFile = firstInstr->sourceFile;
                    braStmt->sourceLine = firstInstr->sourceLine;
                    braStmt->segmentName = firstInstr->segmentName;
                    braStmt->localLabelScope = firstInstr->localLabelScope;
                    braStmt->scopePrefix = firstInstr->scopePrefix;

                    for (size_t idx : allTails[i].instrIndices) {
                        parser->statements[idx]->deleted = true;
                        parser->statements[idx]->size = 0;
                    }

                    parser->statements.insert(
                        parser->statements.begin() + allTails[i].instrIndices.front(),
                        std::move(braStmt)
                    );

                    report("tail-dedup-suffix", firstInstr,
                           "tail → bra " + offsetLabel + " (suffix match, saved " + std::to_string(savings) + " bytes)");
                    changed = true;
                    tailDedupChanged = true;
                    break;
                }
                if (tailDedupChanged) break;
            }
        }
    }

    // --- Pass 4: In-method sequence jumping (extract duplicate sequences) ---
    // EXPERIMENTAL - DISABLED by default due to size accounting bug causing null bytes in code (issue #89)
    // Find duplicate instruction sequences and create shared routines.
    // Conservative: only extract 8+ byte sequences appearing 2+ times with 4+ byte savings.

    struct SeqMatch {
        size_t startIdx = 0;  // First statement index
        std::vector<size_t> instrIndices;
        size_t seqSize = 0;
    };

    // Collect non-overlapping 4-instruction sequences
    std::set<size_t> processed;  // Already-processed instruction indices
    std::map<std::string, std::vector<SeqMatch>> seqsBySignature;

    if (parser->enableExperimental) for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* stmt = parser->statements[i].get();
        if (stmt->deleted || processed.count(i) || !stmt->label.empty()) continue;
        if (stmt->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = stmt->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);
        if (m == "RTS" || m == "RTN" || m == "JMP" || m == "JSR" || m == "BSR" ||
            m == "BRA" || m == "BEQ" || m == "BNE" || m == "BCS" || m == "BCC" ||
            m == "BMI" || m == "BPL" || m == "BVS" || m == "BVC") continue;

        std::vector<size_t> seqIdx;
        std::string sig;
        size_t totSize = 0;

        // Collect 4 consecutive instructions (skip already-processed ones)
        for (size_t j = i; j < parser->statements.size() && seqIdx.size() < 4; ++j) {
            if (processed.count(j)) continue;
            auto* s = parser->statements[j].get();
            if (s->deleted || !s->label.empty()) continue;
            if (s->type != AssemblerParser::Statement::INSTRUCTION) break;

            std::string nm = s->instr.mnemonic;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
            if (nm == "RTS" || nm == "RTN" || nm == "JMP" || nm == "JSR" || nm == "BSR" ||
                nm == "BRA" || nm == "BEQ" || nm == "BNE" || nm == "BCS" || nm == "BCC" ||
                nm == "BMI" || nm == "BPL" || nm == "BVS" || nm == "BVC") break;

            seqIdx.push_back(j);
            sig += nm + ":" + s->instr.operand + ";";
            totSize += s->size;
        }

        if (seqIdx.size() < 4 || totSize < 8) continue;

        // Mark these as processed to avoid overlaps
        for (size_t idx : seqIdx) {
            processed.insert(idx);
        }

        SeqMatch match;
        match.startIdx = i;
        match.instrIndices = seqIdx;
        match.seqSize = totSize;
        seqsBySignature[sig].push_back(match);
    }

    // Extract sequences appearing 2+ times with sufficient savings
    std::vector<std::unique_ptr<AssemblerParser::Statement>> sharedRoutines;
    int routineCounter = 0;

    for (auto& [sig, matches] : seqsBySignature) {
        if (matches.size() < 2) continue;

        size_t seqSize = matches[0].seqSize;
        size_t occCount = matches.size();
        size_t origSize = seqSize * occCount;
        size_t callSize = 3 * occCount;
        size_t routineSize = seqSize + 1;
        size_t newSize = callSize + routineSize;

        if (newSize >= origSize) continue;

        size_t savings = origSize - newSize;
        if (savings < 4) continue;

        std::string routineName = "__seq_" + std::to_string(routineCounter++);

        // Copy sequence to routine
        for (size_t idx : matches[0].instrIndices) {
            auto* stmt = parser->statements[idx].get();
            auto copy = std::make_unique<AssemblerParser::Statement>(*stmt);
            sharedRoutines.push_back(std::move(copy));
        }

        auto rts = std::make_unique<AssemblerParser::Statement>();
        rts->type = AssemblerParser::Statement::INSTRUCTION;
        rts->instr.mnemonic = "rts";
        rts->instr.mode = AddressingMode::IMPLIED;
        rts->size = 1;
        rts->segmentName = parser->statements[matches[0].instrIndices[0]]->segmentName;
        sharedRoutines.push_back(std::move(rts));

        // Replace occurrences (reverse order)
        std::sort(matches.rbegin(), matches.rend(),
                  [](const auto& a, const auto& b) { return a.startIdx > b.startIdx; });

        for (auto& match : matches) {
            auto* firstStmt = parser->statements[match.instrIndices.front()].get();

            auto bsr = std::make_unique<AssemblerParser::Statement>();
            bsr->type = AssemblerParser::Statement::INSTRUCTION;
            bsr->instr.mnemonic = "bsr";
            bsr->instr.mode = AddressingMode::RELATIVE16;
            bsr->instr.operand = routineName;
            bsr->size = 3;
            bsr->line = firstStmt->line;
            bsr->sourceFile = firstStmt->sourceFile;
            bsr->sourceLine = firstStmt->sourceLine;
            bsr->segmentName = firstStmt->segmentName;
            bsr->localLabelScope = firstStmt->localLabelScope;
            bsr->scopePrefix = firstStmt->scopePrefix;

            int deleted_total = 0;
            for (size_t idx : match.instrIndices) {
                deleted_total += parser->statements[idx]->size;
                parser->statements[idx]->deleted = true;
                parser->statements[idx]->size = 0;
            }
            int bsr_size = 3;
            int actual_savings_per_call = deleted_total - bsr_size;

            parser->statements.insert(
                parser->statements.begin() + match.instrIndices.front(),
                std::move(bsr)
            );

            report("seq-extract", firstStmt,
                   "sequence → bsr " + routineName + " (saved " + std::to_string(actual_savings_per_call) + " bytes)");
            changed = true;
        }
    }

    if (!sharedRoutines.empty()) {
        auto label = std::make_unique<AssemblerParser::Statement>();
        label->type = AssemblerParser::Statement::DIRECTIVE;
        label->label = "__shared_routines";
        label->segmentName = sharedRoutines[0]->segmentName;
        parser->statements.push_back(std::move(label));

        for (auto& routine : sharedRoutines) {
            parser->statements.push_back(std::move(routine));
        }
    }

    return changed;
}

// ============================================================================
// Phase 5: Inter-TU Optimization — Load external object files
// ============================================================================
// Reads .o45 object files and extracts function attributes (clobber masks,
// leaf flags) to enable optimization of call sites to external functions.
// ============================================================================

bool AssemblerOptimizer::optimizeWithExternalObjects(
    AssemblerParser* parser,
    const std::vector<std::string>& objectFiles,
    bool verbose,
    bool traceMachState
) {
    // Load external object files and extract function attributes
    std::map<std::string, ExternalFuncInfo> externalFuncs;

    for (const auto& objFile : objectFiles) {
        // Read .o45 file
        std::ifstream file(objFile, std::ios::binary);
        if (!file.is_open()) {
            if (verbose) std::cerr << "phase5: warning: cannot open object file: " << objFile << std::endl;
            continue;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        O45File obj;
        std::string errorMsg;
        if (!O45Reader::read(data, obj, errorMsg)) {
            if (verbose) std::cerr << "phase5: warning: failed to read " << objFile << ": " << errorMsg << std::endl;
            continue;
        }

        // Extract function attributes from exports
        for (const auto& exp : obj.exports) {
            if (exp.hasFuncAttr) {
                uint8_t regMask = exp.funcAttr.regClobbers;
                bool isLeaf = (exp.funcAttr.flags & FUNC_FLAG_LEAF) != 0;

                externalFuncs[exp.name] = {regMask, isLeaf};

                if (verbose) {
                    std::cerr << "phase5: loaded " << exp.name << " from " << objFile
                              << " (regMask=$" << std::hex << (int)regMask << std::dec
                              << ", leaf=" << (isLeaf ? "yes" : "no") << ")" << std::endl;
                }
            }
        }
    }

    if (verbose && !externalFuncs.empty()) {
        std::cerr << "phase5: loaded " << externalFuncs.size() << " external function(s)" << std::endl;
    }

    // Optimize with external function info
    bool changed = optimizeInternal(parser, &externalFuncs, verbose, traceMachState, parser->optimizationLevel);

    // Log summary
    if (verbose && !externalFuncs.empty()) {
        std::cerr << "phase5: optimization complete with " << externalFuncs.size()
                  << " external function attribute(s)" << std::endl;
    }

    return changed;
}
