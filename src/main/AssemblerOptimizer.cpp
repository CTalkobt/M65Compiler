#include "AssemblerOptimizer.hpp"
#include "AssemblerParser.hpp"
#include <algorithm>
#include <map>

bool AssemblerOptimizer::optimize(AssemblerParser* parser) {
    bool changed = false;
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
                s->instr.mnemonic = "jmp";
                // operand and mode (ABSOLUTE) stay the same; size stays 3 bytes
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

    // --- Pass 2: Register tracking and redundant load elimination ---
    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;

        // Barrier: non-local labels reset all knowledge
        if (!s->label.empty() && s->label[0] != '@') {
            invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            stackVarLastValue.clear();
            invalidateMem();
        }

        bool isStackStoreOptimizationCandidate = false;

        // STW.SP redundant store elimination
        if (s->type == AssemblerParser::Statement::STW && s->instr.mnemonic == "STW.SP") {
            isStackStoreOptimizationCandidate = true;
            std::string immediateValue = s->instr.operand;
            std::string varName = parser->tokens[s->exprTokenIndex].value;

            if (stackVarLastValue.count(varName) && stackVarLastValue[varName] == immediateValue) {
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
                if (redundant) { s->deleted = true; s->size = 0; changed = true; continue; }
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
                if (redundant) { s->deleted = true; s->size = 0; changed = true; continue; }
            }
            if (m == "LDY") {
                bool redundant = (regY.known && regY.mode == mode && regY.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regY.imm == op);
                if (redundant) { s->deleted = true; s->size = 0; changed = true; continue; }
            }
            if (m == "LDZ") {
                bool redundant = (regZ.known && regZ.mode == mode && regZ.var == op) ||
                                 (mode == AddressingMode::IMMEDIATE && regZ.imm == op);
                if (redundant) { s->deleted = true; s->size = 0; changed = true; continue; }
            }

            // --- Register state tracking ---
            if (m == "LDA") {
                regA.known = true; regA.mode = mode; regA.var = op;
                regA.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STA") {
                // A still holds its value — don't invalidate.
                // Record what was stored so subsequent LDA from same addr can be eliminated.
                std::string key = makeMemKey(mode, op);
                if (!key.empty()) {
                    if (regA.known) {
                        memTrack[key] = {regA};
                    } else {
                        // A holds an unknown value, but after STA $addr, A and $addr
                        // are guaranteed to hold the same value. Track A as "loaded from addr"
                        // so a subsequent LDA $addr is recognized as redundant.
                        regA.known = true;
                        regA.mode = mode;
                        regA.var = op;
                        regA.imm = "";
                        memTrack[key] = {regA};
                    }
                }
            }
            else if (m == "LDX") {
                regX.known = true; regX.mode = mode; regX.var = op;
                regX.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STX") {
                // X unchanged. Record store for bidirectional tracking.
                std::string key = makeMemKey(mode, op);
                if (!key.empty()) {
                    if (regX.known) {
                        memTrack[key] = {regX};
                    } else {
                        regX.known = true;
                        regX.mode = mode;
                        regX.var = op;
                        regX.imm = "";
                        memTrack[key] = {regX};
                    }
                }
            }
            else if (m == "LDY") {
                regY.known = true; regY.mode = mode; regY.var = op;
                regY.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STY") { /* Y unchanged */ }
            else if (m == "LDZ") {
                regZ.known = true; regZ.mode = mode; regZ.var = op;
                regZ.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STZ") { /* Z unchanged */ }

            // --- Transfer instructions ---
            else if (m == "TAX") { regX = regA; }
            else if (m == "TXA") { regA = regX; }
            else if (m == "TAY") { regY = regA; }
            else if (m == "TYA") { regA = regY; }
            else if (m == "TAZ") { regZ = regA; }
            else if (m == "TZA") { regA = regZ; }
            else if (m == "TSX") { invalidate(regX); }

            // --- Control flow ---
            else if (m == "JMP") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
            }

            // --- JSR: Phase 2 selective invalidation ---
            else if (m == "JSR" && mode == AddressingMode::ABSOLUTE) {
                auto it = procClobbers.find(op);
                if (it != procClobbers.end() && it->second.known) {
                    uint8_t mask = it->second.regMask;
                    if (mask & 0x01) invalidate(regA);
                    if (mask & 0x02) invalidate(regX);
                    if (mask & 0x04) invalidate(regY);
                    if (mask & 0x08) invalidate(regZ);
                } else {
                    invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                }
                invalidateMem(); // callee may modify memory
            }
            else if (m == "JSR") {
                // Indirect JSR — invalidate all
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
            }
            else if (m == "CALL" || m == "BSR") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
                invalidateMem();
            }

            // --- Push: does NOT modify any register ---
            else if (m == "PHA" || m == "PHX" || m == "PHY" || m == "PHZ" || m == "PHW") {
                // Push reads a register but doesn't change it — no invalidation
            }

            // --- Pull: modifies ONLY the target register ---
            else if (m == "PLA") { invalidate(regA); }
            else if (m == "PLX") { invalidate(regX); }
            else if (m == "PLY") { invalidate(regY); }
            else if (m == "PLZ") { invalidate(regZ); }

            // --- RTS/RTN/RTI: function exit, no optimization across ---
            else if (m == "RTS" || m == "RTN" || m == "RTI") {
                // Nothing after this is reachable (until next label resets state)
            }

            // --- ALU: modifies A and flags ---
            else if (m == "ADC" || m == "SBC" || m == "AND" || m == "ORA" || m == "EOR") {
                invalidate(regA);
            }
            else if (m == "ASL" || m == "LSR" || m == "ROL" || m == "ROR") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    invalidate(regA);
                }
                // Memory-mode shifts don't affect registers
            }
            else if (m == "INC" || m == "DEC") {
                if (mode == AddressingMode::ACCUMULATOR || mode == AddressingMode::IMPLIED) {
                    invalidate(regA);
                }
                // Memory-mode inc/dec don't affect A
            }
            else if (m == "INX" || m == "DEX") {
                invalidate(regX);
                if (regA.known && (regA.mode == AddressingMode::BASE_PAGE_X_INDIRECT ||
                    regA.mode == AddressingMode::ABSOLUTE_X || regA.mode == AddressingMode::BASE_PAGE_X))
                    invalidate(regA);
                if (regY.known && (regY.mode == AddressingMode::ABSOLUTE_X || regY.mode == AddressingMode::BASE_PAGE_X))
                    invalidate(regY);
            }
            else if (m == "INY" || m == "DEY") {
                invalidate(regY);
                // Also invalidate registers loaded via Y-indexed addressing
                if (regA.known && (regA.mode == AddressingMode::BASE_PAGE_INDIRECT_Y ||
                    regA.mode == AddressingMode::ABSOLUTE_Y || regA.mode == AddressingMode::BASE_PAGE_Y))
                    invalidate(regA);
                if (regX.known && (regX.mode == AddressingMode::BASE_PAGE_INDIRECT_Y ||
                    regX.mode == AddressingMode::ABSOLUTE_Y || regX.mode == AddressingMode::BASE_PAGE_Y))
                    invalidate(regX);
            }
            else if (m == "INZ" || m == "DEZ") { invalidate(regZ); }

            // --- Compare: does not modify registers (only flags) ---
            else if (m == "CMP" || m == "CPX" || m == "CPY" || m == "CPZ") {
                // No register invalidation — only flags change
            }

            // --- Clear: sets register to 0 ---
            else if (m == "CLA") { regA.known = true; regA.imm = "#$00"; regA.var = ""; regA.mode = AddressingMode::IMMEDIATE; }
            else if (m == "CLX") { regX.known = true; regX.imm = "#$00"; regX.var = ""; regX.mode = AddressingMode::IMMEDIATE; }
            else if (m == "CLY") { regY.known = true; regY.imm = "#$00"; regY.var = ""; regY.mode = AddressingMode::IMMEDIATE; }
            else if (m == "CLZ") { regZ.known = true; regZ.imm = "#$00"; regZ.var = ""; regZ.mode = AddressingMode::IMMEDIATE; }

            // --- Anything else: conservatively invalidate A ---
            // (branches, bit tests, etc. — safe to not invalidate since they don't modify regs)
            // Only truly unknown instructions should invalidate.
        } else if (s->isSimulatedOp()) {
            // Simulated ops are complex multi-instruction expansions.
            // Conservatively invalidate everything.
            invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            stackVarLastValue.clear();
        }
    }
    return changed;
}
