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

    auto invalidate = [&](RegState& r) { r.known = false; r.var = ""; r.imm = ""; };

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

    for (size_t i = 0; i < parser->statements.size(); ++i) {
        auto* s = parser->statements[i].get();
        if (s->deleted) continue;

        // Barrier: non-local labels reset all knowledge
        if (!s->label.empty() && s->label[0] != '@') {
            invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            stackVarLastValue.clear();
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
                }
            } else if (s->isSimulatedOp()) {
                stackVarLastValue.clear();
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
            if (m == "LDA" && ((regA.known && regA.mode == mode && regA.var == op) || (mode == AddressingMode::IMMEDIATE && regA.imm == op))) {
                s->deleted = true; s->size = 0; changed = true; continue;
            }
            if (m == "LDX" && ((regX.known && regX.mode == mode && regX.var == op) || (mode == AddressingMode::IMMEDIATE && regX.imm == op))) {
                s->deleted = true; s->size = 0; changed = true; continue;
            }
            if (m == "LDY" && ((regY.known && regY.mode == mode && regY.var == op) || (mode == AddressingMode::IMMEDIATE && regY.imm == op))) {
                s->deleted = true; s->size = 0; changed = true; continue;
            }
            if (m == "LDZ" && ((regZ.known && regZ.mode == mode && regZ.var == op) || (mode == AddressingMode::IMMEDIATE && regZ.imm == op))) {
                s->deleted = true; s->size = 0; changed = true; continue;
            }

            // --- Register state tracking ---
            if (m == "LDA") {
                regA.known = true; regA.mode = mode; regA.var = op;
                regA.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STA") {
                // A still holds its value — don't invalidate.
                // Don't track "A came from addr" because addr could be
                // modified by side effects before a subsequent LDA.
            }
            else if (m == "LDX") {
                regX.known = true; regX.mode = mode; regX.var = op;
                regX.imm = (mode == AddressingMode::IMMEDIATE) ? op : "";
            }
            else if (m == "STX") { /* X unchanged, no addr tracking */ }
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
            }
            else if (m == "JSR") {
                // Indirect JSR — invalidate all
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
            }
            else if (m == "CALL" || m == "BSR") {
                invalidate(regA); invalidate(regX); invalidate(regY); invalidate(regZ);
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
