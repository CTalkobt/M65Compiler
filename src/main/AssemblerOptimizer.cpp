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
    struct RegState {
        bool known = false;
        std::string var;
        AddressingMode mode;
        std::string imm;

        bool operator!=(const RegState& other) const {
            if (known != other.known) return true;
            if (!known) return false;
            return var != other.var || mode != other.mode || imm != other.imm;
        }
    };
    RegState regA, regX, regY, regZ;

    // MachineState for value-aware optimizations (constant propagation, flag tracking)
    MachineState ms;

    // Parse an immediate operand string to a numeric value.
    // Returns true if parseable, fills 'val'. Handles decimal, $hex, %binary.
    auto parseImm = [](const std::string& op, int64_t& val) -> bool {
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

    std::map<std::string, std::string> stackVarLastValue;

    // Track what register value was last stored to each memory location.
    // Key: "mode:operand" (e.g., "BP:$20"), Value: the RegState at time of store.
    // Enables: lda #1; sta $20; ... lda $20 → delete the lda (A still holds #1)
    struct MemEntry { RegState val; };
    std::map<std::string, MemEntry> memTrack;

    auto makeMemKey = [](AddressingMode mode, const std::string& op) -> std::string {
        // Only track simple addressing modes (ZP, absolute) — not indirect or indexed
        // Use operand string as key (BASE_PAGE $20 and ABSOLUTE $0020 are the same location)
        if (mode == AddressingMode::BASE_PAGE || mode == AddressingMode::ABSOLUTE)
            return op;
        return "";
    };

    auto invalidate = [&](RegState& r) { r.known = false; r.var = ""; r.imm = ""; };

    auto invalidateMem = [&]() { memTrack.clear(); };

    // Invalidate memory entries that might alias with a store to the given address
    auto invalidateMemAt = [&](const std::string& key) {
        memTrack.erase(key);
    };

    // Build proc-name → clobber info lookup from parsed procedures.
    // Used for selective register invalidation at JSR call sites (Phase 2).
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
    // Must run before register tracking pass since it changes control flow.
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;
        if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = s->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);

        // Match JSR with absolute addressing (not indirect JSR)
        if (m == "JSR" && s->instr.mode == AddressingMode::ABSOLUTE) {
            // Find next non-deleted statement
            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            // Must not have a label (label = branch target, can't delete)
            if (!next->label.empty()) continue;
            if (next->type != AssemblerParser::Statement::INSTRUCTION) continue;

            std::string nm = next->instr.mnemonic;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);

            // Only plain RTS/RTN (IMPLIED mode, no stack cleanup operand)
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
            // Find next non-deleted statement
            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            // If the branch target matches the next statement's label, it's a no-op.
            // Labels may be scope-qualified (e.g., "proc:@label") while the operand
            // uses the unqualified name ("@label"). Match either way.
            if (!next->label.empty()) {
                const auto& lbl = next->label;
                const auto& op = s->instr.operand;
                bool match = (lbl == op);
                if (!match && !s->scopePrefix.empty()) {
                    match = (lbl == s->scopePrefix + op);
                }
                if (!match) {
                    // Also check if label ends with the operand (suffix match after ':')
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
    // Pattern: Bcc +2; BRA target → B!cc target (saves 2 bytes)
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
            return ""; // not invertible
        };

        for (size_t i = 0; i < parser->statements.size(); ++i) {
            auto* s = parser->statements[i].get();
            if (s->deleted) continue;
            if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;
            // Only consider conditional branch instructions (not BRA, BBS, BBR, BRK, etc.)
            std::string inv = invertBranch(s->instr.mnemonic);
            if (inv.empty()) continue;
            // Verify it's actually a branch with a relative target
            std::string mUp = s->instr.mnemonic;
            std::transform(mUp.begin(), mUp.end(), mUp.begin(), ::toupper);
            if (mUp != "BEQ" && mUp != "BNE" && mUp != "BCC" && mUp != "BCS" &&
                mUp != "BMI" && mUp != "BPL" && mUp != "BVC" && mUp != "BVS") continue;

            // Find next non-deleted statement
            size_t j = i + 1;
            while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
            if (j >= parser->statements.size()) continue;

            auto* next = parser->statements[j].get();
            if (!next->label.empty()) continue; // label = branch target from elsewhere
            if (next->type != AssemblerParser::Statement::INSTRUCTION) continue;

            std::string nm = next->instr.mnemonic;
            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
            if (nm != "BRA") continue;

            // Check that the conditional branch targets the instruction after the BRA.
            // Find the statement after the BRA
            size_t k = j + 1;
            while (k < parser->statements.size() && parser->statements[k]->deleted) ++k;
            if (k >= parser->statements.size()) continue;

            auto* afterBra = parser->statements[k].get();
            // The conditional branch should target afterBra's label
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

            // Invert: replace conditional branch with inverted condition + BRA's target
            report("branch-invert", s, std::string(s->instr.mnemonic) + " + BRA → " + inv + " (saved 2 bytes)");
            s->instr.mnemonic = inv;
            s->instr.operand = next->instr.operand;
            s->instr.operandTokenIndex = next->instr.operandTokenIndex;
            s->exprTokenIndex = next->exprTokenIndex;
            // Delete the BRA
            next->deleted = true;
            next->size = 0;
            changed = true;
        }
    }

    // --- Pass 1d: JMP→BRA conversion when target is within ±127 bytes ---
    // Uses addresses from the previous assembler pass (available after pass 1).
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;
        if (s->type != AssemblerParser::Statement::INSTRUCTION) continue;

        std::string m = s->instr.mnemonic;
        std::transform(m.begin(), m.end(), m.begin(), ::toupper);

        if ((m == "JMP" || m == "JSR") && s->instr.mode == AddressingMode::ABSOLUTE) {
            // Resolve the target address
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
                // JMP (3 bytes) → BRA (2 bytes) when target within ±127
                // BRA offset computed from PC+2 (after 2-byte instruction)
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
                // JSR (3 bytes) → BSR (3 bytes, relative16) for position-independent code
                // Only convert when operand is a symbolic label (not a bare numeric address).
                // Skip for extern/relocatable symbols — their addresses aren't final.
                bool canBSR = resolved;
                if (canBSR && s->instr.operandTokenIndex >= 0) {
                    std::string operand = s->instr.operand;
                    if (operand.empty() && s->instr.operandTokenIndex < (int)parser->tokens.size())
                        operand = parser->tokens[s->instr.operandTokenIndex].value;
                    // Skip bare numeric addresses (user explicitly chose JSR $addr)
                    if (operand.empty() || operand[0] == '$' || operand[0] == '%' ||
                        (operand[0] >= '0' && operand[0] <= '9'))
                        canBSR = false;
                    // Skip extern/relocatable symbols
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

    // --- Pass 2: Register tracking and redundant load elimination ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;

        // Barrier: non-local labels reset all knowledge
        if (!s->label.empty() && s->label[0] != '@') {
            invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            stackVarLastValue.clear();
            invalidateMem();
            ms.invalidateAll();
        }

        bool isStackStoreOptimizationCandidate = false;

        // STW.SP redundant store elimination
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
                    // For stores to simple addresses, only invalidate that address in memTrack.
                    // For everything else (JSR, indirect stores, ALU ops), invalidate all.
                    if (M == "STA" || M == "STX" || M == "STY" || M == "STZ") {
                        std::string key = makeMemKey(s->instr.mode, s->instr.operand);
                        if (!key.empty()) invalidateMemAt(key);
                        else invalidateMem(); // indirect/indexed store — can't track
                    } else {
                        invalidateMem();
                    }
                }
            } else if (s->isSimulatedOp()) {
                stackVarLastValue.clear();
                invalidateMem();
            }
        }

        // Register tracking and redundant load elimination
        if (s->type == AssemblerParser::Statement::INSTRUCTION || s->type == AssemblerParser::Statement::LDW) {
            // Normalize mnemonic to uppercase for all comparisons
            std::string m;
            if (s->type == AssemblerParser::Statement::LDW) m = "LDAX";
            else if (s->type == AssemblerParser::Statement::STW) m = "STAX";
            else {
                m = s->instr.mnemonic;
                std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            }
            AddressingMode mode = s->instr.mode;
            std::string op = s->instr.operand;

            // Word load tracking (LDW)
            if (s->type == AssemblerParser::Statement::LDW) {
                std::string regStr = s->instr.operand;
                std::transform(regStr.begin(), regStr.end(), regStr.begin(), ::toupper);
                char r2 = 'X';
                if (regStr == ".AY") r2 = 'Y';
                else if (regStr == ".AZ") r2 = 'Z';
                regA.known = true; regA.mode = AddressingMode::NONE;
                if (r2 == 'X') regX.known = true;
                else if (r2 == 'Y') regY.known = true;
                else if (r2 == 'Z') regZ.known = true;
                continue;
            }

            if (s->type == AssemblerParser::Statement::STW) continue;

            // Parse immediate operand to numeric value for MachineState
            int64_t immVal = 0;
            bool hasImmVal = (mode == AddressingMode::IMMEDIATE) && parseImm(op, immVal);

            // --- CMP #0 elimination (MachineState flag-aware) ---
            // After a load instruction (LDA/LDX/LDY/LDZ), N/Z flags already reflect the
            // loaded value. A subsequent CMP #0 is redundant — it produces identical N/Z
            // (and always sets C=1). Eliminate if only BEQ/BNE/BMI/BPL follow.
            if (m == "CMP" && mode == AddressingMode::IMMEDIATE && hasImmVal && immVal == 0) {
                // CMP #0 is redundant if N/Z flags already reflect A's value
                if (ms.flags.flagsReflect(REG_A)) {
                    // Verify the branch following only tests N/Z (not C)
                    size_t j = i + 1;
                    while (j < parser->statements.size() && parser->statements[j]->deleted) ++j;
                    bool safeToElim = true;
                    if (j < parser->statements.size()) {
                        auto* next = parser->statements[j].get();
                        if (next->type == AssemblerParser::Statement::INSTRUCTION) {
                            std::string nm = next->instr.mnemonic;
                            std::transform(nm.begin(), nm.end(), nm.begin(), ::toupper);
                            // BCC/BCS test carry — CMP #0 sets C=1 which they may depend on
                            if (nm == "BCC" || nm == "BCS") safeToElim = false;
                        }
                    }
                    if (safeToElim) {
                        report("cmp-elim", s, "CMP #0 eliminated (flags already set from load)");
                        s->deleted = true; s->size = 0; changed = true; continue;
                    }
                }
            }

            // --- Redundant load elimination ---
            // Check 1: register already holds the value (same source)
            // Check 2: register holds the value that was last stored to this address (bidirectional)
            if (m == "LDA") {
                bool redundant = (regA.known && regA.mode == mode && regA.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regA.imm == op);
                if (!redundant) {
                    std::string key = makeMemKey(mode, op);
                    if (!key.empty()) {
                        auto mit = memTrack.find(key);
                        if (mit != memTrack.end() && regA.known &&
                            regA.imm == mit->second.val.imm && regA.mode == mit->second.val.mode &&
                            regA.var == mit->second.val.var) {
                            redundant = true;
                        }
                    }
                }
                if (redundant) {
                    report("redundant-load", s, "LDA " + op + " eliminated (A already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }
            if (m == "LDX") {
                bool redundant = (regX.known && regX.mode == mode && regX.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regX.imm == op);
                if (!redundant) {
                    std::string key = makeMemKey(mode, op);
                    if (!key.empty()) {
                        auto mit = memTrack.find(key);
                        if (mit != memTrack.end() && regX.known &&
                            regX.imm == mit->second.val.imm && regX.mode == mit->second.val.mode &&
                            regX.var == mit->second.val.var) {
                            redundant = true;
                        }
                    }
                }
                if (redundant) {
                    report("redundant-load", s, "LDX " + op + " eliminated (X already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }
            if (m == "LDY") {
                bool redundant = (regY.known && regY.mode == mode && regY.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regY.imm == op);
                if (redundant) {
                    report("redundant-load", s, "LDY " + op + " eliminated (Y already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }
            if (m == "LDZ") {
                bool redundant = (regZ.known && regZ.mode == mode && regZ.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regZ.imm == op);
                if (redundant) {
                    report("redundant-load", s, "LDZ " + op + " eliminated (Z already holds value)");
                    s->deleted = true; s->size = 0; changed = true; continue;
                }
            }

            // --- Register state tracking (string-based + MachineState) ---
            if (m == "LDA") {
                regA.known = true; regA.mode = mode; regA.var = op;
                regA.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
                if (hasImmVal) ms.setConst(REG_A, immVal & 0xFF);
                else { ms.invalidateReg(REG_A); ms.flags.setNZ(REG_A); }
            }
            else if (m == "STA") {
                std::string key = makeMemKey(mode, op);
                if (!key.empty()) {
                    if (regA.known) {
                        memTrack[key] = {regA};
                    } else {
                        regA.known = true; regA.mode = mode; regA.var = op; regA.imm = "";
                        memTrack[key] = {regA};
                    }
                }
                // MachineState: STA doesn't modify A or flags
            }
            else if (m == "LDX") {
                regX.known = true; regX.mode = mode; regX.var = op;
                regX.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
                if (hasImmVal) ms.setConst(REG_X, immVal & 0xFF);
                else { ms.invalidateReg(REG_X); ms.flags.setNZ(REG_X); }
            }
            else if (m == "STX") {
                std::string key = makeMemKey(mode, op);
                if (!key.empty()) {
                    if (regX.known) { memTrack[key] = {regX}; }
                    else { regX.known = true; regX.mode = mode; regX.var = op; regX.imm = ""; memTrack[key] = {regX}; }
                }
            }
            else if (m == "LDY") {
                regY.known = true; regY.mode = mode; regY.var = op;
                regY.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
                if (hasImmVal) ms.setConst(REG_Y, immVal & 0xFF);
                else { ms.invalidateReg(REG_Y); ms.flags.setNZ(REG_Y); }
            }
            else if (m == "STY") { /* Y unchanged */ }
            else if (m == "LDZ") {
                regZ.known = true; regZ.mode = mode; regZ.var = op;
                regZ.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
                if (hasImmVal) ms.setConst(REG_Z, immVal & 0xFF);
                else { ms.invalidateReg(REG_Z); ms.flags.setNZ(REG_Z); }
            }
            else if (m == "STZ") { /* Z unchanged */ }

            // --- Transfer instructions ---
            else if (m == "TAX") { regX = regA; ms.setTransfer(REG_X, REG_A); }
            else if (m == "TXA") { regA = regX; ms.setTransfer(REG_A, REG_X); }
            else if (m == "TAY") { regY = regA; ms.setTransfer(REG_Y, REG_A); }
            else if (m == "TYA") { regA = regY; ms.setTransfer(REG_A, REG_Y); }
            else if (m == "TAZ") { regZ = regA; ms.setTransfer(REG_Z, REG_A); }
            else if (m == "TZA") { regA = regZ; ms.setTransfer(REG_A, REG_Z); }
            else if (m == "TSX") { invalidate(regX); ms.setTransfer(REG_X, REG_SP); }

            // --- Control flow ---
            else if (m == "JMP") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
                ms.invalidateAll();
            }

            // --- JSR: Phase 2 selective invalidation ---
            else if (m == "JSR" && mode == AddressingMode::ABSOLUTE) {
                auto it = procClobbers.find(op);
                if (it != procClobbers.end() && it->second.known) {
                    uint8_t mask = it->second.regMask;
                    if (mask & 0x01) { invalidate(regA); ms.invalidateReg(REG_A); }
                    if (mask & 0x02) { invalidate(regX); ms.invalidateReg(REG_X); }
                    if (mask & 0x04) { invalidate(regY); ms.invalidateReg(REG_Y); }
                    if (mask & 0x08) { invalidate(regZ); ms.invalidateReg(REG_Z); }
                } else {
                    invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                    ms.invalidateAll();
                }
                invalidateMem();
                ms.flags.invalidate();
                ms.invalidateAllMem();
            }
            else if (m == "JSR") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
                ms.invalidateAll();
            }
            else if (m == "CALL" || m == "BSR") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
                ms.invalidateAll();
            }

            // --- Push: does NOT modify any register ---
            else if (m == "PHA" || m == "PHX" || m == "PHY" || m == "PHZ" || m == "PHW") {
                ms.spModified();
            }

            // --- Pull: modifies ONLY the target register ---
            else if (m == "PLA") { invalidate(regA); ms.spModified(); ms.invalidateReg(REG_A); }
            else if (m == "PLX") { invalidate(regX); ms.spModified(); ms.invalidateReg(REG_X); }
            else if (m == "PLY") { invalidate(regY); ms.spModified(); ms.invalidateReg(REG_Y); }
            else if (m == "PLZ") { invalidate(regZ); ms.spModified(); ms.invalidateReg(REG_Z); }

            // --- RTS/RTN/RTI: function exit ---
            else if (m == "RTS" || m == "RTN" || m == "RTI") {
                ms.invalidateAll();
            }

            // --- ALU: modifies A and flags ---
            else if (m == "ADC" || m == "SBC" || m == "AND" || m == "ORA" || m == "EOR") {
                invalidate(regA);
                ms.invalidateReg(REG_A);
                ms.flags.invalidate();
            }
            else if (m == "ASL" || m == "LSR" || m == "ROL" || m == "ROR") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    invalidate(regA);
                    ms.invalidateReg(REG_A);
                }
                ms.flags.invalidate();
            }
            else if (m == "INC" || m == "DEC") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    invalidate(regA);
                    ms.invalidateReg(REG_A);
                }
                ms.flags.invalidate();
            }
            else if (m == "INX" || m == "DEX") {
                invalidate(regX);
                if (regA.known && (regA.mode == AddressingMode::BASE_PAGE_X_INDIRECT ||
                    regA.mode == AddressingMode::ABSOLUTE_X || regA.mode == AddressingMode::BASE_PAGE_X))
                    invalidate(regA);
                if (regY.known && (regY.mode == AddressingMode::ABSOLUTE_X || regY.mode == AddressingMode::BASE_PAGE_X))
                    invalidate(regY);
                ms.invalidateReg(REG_X);
                ms.flags.invalidate();
            }
            else if (m == "INY" || m == "DEY") {
                invalidate(regY);
                if (regA.known && (regA.mode == AddressingMode::BASE_PAGE_INDIRECT_Y ||
                    regA.mode == AddressingMode::ABSOLUTE_Y || regA.mode == AddressingMode::BASE_PAGE_Y))
                    invalidate(regA);
                if (regX.known && (regX.mode == AddressingMode::BASE_PAGE_INDIRECT_Y ||
                    regX.mode == AddressingMode::ABSOLUTE_Y || regX.mode == AddressingMode::BASE_PAGE_Y))
                    invalidate(regX);
                ms.invalidateReg(REG_Y);
                ms.flags.invalidate();
            }
            else if (m == "INZ" || m == "DEZ") { invalidate(regZ); ms.invalidateReg(REG_Z); ms.flags.invalidate(); }

            // --- Compare: does not modify registers (only flags) ---
            else if (m == "CMP" || m == "CPX" || m == "CPY" || m == "CPZ") {
                // String-based: no register invalidation — only flags change
                // MachineState: update flags
                if (m == "CMP" && hasImmVal) {
                    if (ms.reg[REG_A].isConst()) {
                        uint8_t a = (uint8_t)(ms.reg[REG_A].constVal & 0xFF);
                        uint8_t v = (uint8_t)(immVal & 0xFF);
                        ms.flags.z = (a == v) ? FlagState::F_SET : FlagState::F_CLEAR;
                        ms.flags.c = (a >= v) ? FlagState::F_SET : FlagState::F_CLEAR;
                        uint8_t diff = a - v;
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
            else if (m == "CLA") { regA.known = true; regA.imm = "#$00"; regA.var = ""; regA.mode = AddressingMode::IMMEDIATE; ms.setConst(REG_A, 0); }
            else if (m == "CLX") { regX.known = true; regX.imm = "#$00"; regX.var = ""; regX.mode = AddressingMode::IMMEDIATE; ms.setConst(REG_X, 0); }
            else if (m == "CLY") { regY.known = true; regY.imm = "#$00"; regY.var = ""; regY.mode = AddressingMode::IMMEDIATE; ms.setConst(REG_Y, 0); }
            else if (m == "CLZ") { regZ.known = true; regZ.imm = "#$00"; regZ.var = ""; regZ.mode = AddressingMode::IMMEDIATE; ms.setConst(REG_Z, 0); }

            // --- CLC/SEC: carry flag ---
            else if (m == "CLC") { ms.flags.setCarry(false); }
            else if (m == "SEC") { ms.flags.setCarry(true); }

            // --- Branches: conservative — invalidate all at branch targets ---
            else if (m == "BRA" || m == "BEQ" || m == "BNE" || m == "BCC" || m == "BCS" ||
                     m == "BMI" || m == "BPL" || m == "BVC" || m == "BVS") {
                // Flags and registers are still valid *after* a non-taken branch.
                // But we can't know if the target merges different state, so
                // we leave state unchanged for fall-through. Labels already reset state.
            }

            // --- Anything else: conservatively invalidate MachineState ---
            // (branches, bit tests, etc. — safe to not invalidate since they don't modify regs)
        } else if (s->isSimulatedOp()) {
            // Simulated ops are complex multi-instruction expansions.
            // Conservatively invalidate everything.
            invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            stackVarLastValue.clear();
            ms.invalidateAll();
        }
    }
    return changed;
}
