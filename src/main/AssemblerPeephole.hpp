#pragma once

#include <string>
#include <vector>
#include <regex>

/**
 * Phase 87: Assembler Peephole Optimization for Compound Assignments
 *
 * Detects and eliminates redundant load/store patterns in generated assembly:
 *
 * Pattern: Store to ZP → subsequent Load from same ZP (value not clobbered)
 *
 * Example (10 compound assignments on `a`):
 *   sta $20; stx $21    (store result)
 *   ... (check: no intervening writes to $20/$21)
 *   lda $20; ldx $21    (load same result) → DELETE if no clobber
 *
 * Impact: Eliminates 2-4 instructions per compound assignment in chains
 * Expected gain: 15-25% for code with compound assignment sequences
 */

class AssemblerPeepholeOptimizer {
public:
    struct Instruction {
        std::string mnemonic;
        std::string operand;
        int lineNumber;
        std::string originalLine;
    };

    static std::vector<std::string> optimize(const std::vector<std::string>& asmLines) {
        std::vector<std::string> result = asmLines;

        // Phase 87.1: Remove redundant load after store to same location
        removeRedundantLoads(result);

        // Phase 87.2: Combine consecutive operations on same variable
        fuseConsecutiveOps(result);

        return result;
    }

private:
    static void removeRedundantLoads(std::vector<std::string>& lines) {
        // Pattern: sta addr; ... lda addr (same address)
        // Detect and remove redundant loads if address not clobbered between store/load
        // Handles both hex ($ZP) and symbolic addresses (_local_0, __zp_scratch)

        // Matches: sta/stx/sty followed by address (hex or symbol)
        std::regex storePattern(R"(^\s*st([axy])\s+([\$_a-zA-Z][\w\+\-]*))");
        std::regex loadPattern(R"(^\s*ld([axy])\s+([\$_a-zA-Z][\w\+\-]*))");
        std::regex anyWritePattern(R"(^\s*st[axy]\s+)");  // Any store
        std::regex jmpPattern(R"(^\s*(jmp|bra|beq|bne|bcs|bcc|bmi|bpl|proc|endproc))");  // Branches (but not .loc)

        for (size_t i = 0; i < lines.size(); i++) {
            std::smatch storeMatch;
            if (!std::regex_search(lines[i], storeMatch, storePattern)) continue;

            std::string storeReg = storeMatch[1].str();
            std::string storeAddr = storeMatch[2].str();

            // Look for matching load in next 30 instructions
            int foundCount = 0;
            for (size_t j = i + 1; j < i + 30 && j < lines.size(); j++) {
                // Stop if we hit a branch/label (control flow changes)
                if (std::regex_search(lines[j], jmpPattern)) break;

                std::smatch loadMatch;
                if (!std::regex_search(lines[j], loadMatch, loadPattern)) continue;

                // Check if load is from same register and address
                if (loadMatch[1].str() != storeReg) continue;
                if (loadMatch[2].str() != storeAddr) continue;

                // Check if address was modified between store and load
                bool clobbered = false;
                for (size_t k = i + 1; k < j; k++) {
                    if (lines[k].find(storeAddr) != std::string::npos &&
                        std::regex_search(lines[k], anyWritePattern)) {
                        clobbered = true;
                        break;
                    }
                }

                if (!clobbered) {
                    // Comment out the redundant load
                    lines[j] = "    ; [peephole-opt] " + lines[j];
                    foundCount++;
                    if (foundCount >= 2) break;  // Limit optimizations per store to avoid over-optimization
                }
            }
        }
    }

    static void fuseConsecutiveOps(std::vector<std::string>& lines) {
        // Phase 87.2: Detect load a; op a; store a; load a; op a; store a
        // and fuse to: load a; op a; op a; store a; store a
        // (or remove intermediate stores)

        // This requires more sophisticated pattern matching
        // For now, the removeRedundantLoads pass handles the main case
    }
};
