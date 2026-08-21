// Phase 99.4: Bank Setup Optimization Implementation
#include "Phase99BankOptimizer.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace phase99 {

std::map<std::string, BankSetupDecision> BankSetupOptimizer::analyzeBankSetupPlacement() {
    std::map<std::string, BankSetupDecision> decisions;

    // Get hot variables first
    auto hotVars = getHotVariables(10);
    std::set<std::string> hotVarSet(hotVars.begin(), hotVars.end());

    // Analyze each variable
    for (const auto& [varName, pattern] : accessPatterns_) {
        BankSetupDecision decision;
        decision.variableName = varName;
        decision.targetBank = pattern.bank;
        decision.isHotVariable = hotVarSet.count(varName) > 0;

        // Check if setup can be hoisted
        if (pattern.inLoop) {
            decision.canHoistOutsideLoop = canHoistBankSetup(
                varName, pattern.inLoop, pattern.accessCount);
        }

        // Check if caching benefits
        decision.shouldUseRegisterCache = shouldUseCaching(
            varName, pattern.accessCount, pattern.inLoop);

        if (decision.shouldUseRegisterCache) {
            decision.cacheRegisterSlot = 0x20;  // Temp ZP register
        }

        decisions[varName] = decision;
    }

    return decisions;
}

bool BankSetupOptimizer::canHoistBankSetup(const std::string& varName,
                                          bool inLoop,
                                          int coAccessCount) const {
    if (!inLoop) return false;

    // Cost of bank setup: ~4 bytes
    // Savings per loop iteration: 4 bytes × iterations
    // Hoisting is worth it if:
    // 1. Variable accessed multiple times in loop
    // 2. Co-accessed variables in same bank (no switch needed)

    if (coAccessCount < 2) return false;  // Need at least 2 accesses to justify

    // Estimate loop iterations (conservative)
    const int ESTIMATED_ITERATIONS = 10;  // Typical inner loop

    // Cost of hoisting: 4 bytes (one-time)
    // Savings: 4 bytes × (iterations - 1)
    // Worth it if: 4 * ESTIMATED_ITERATIONS > 4
    return ESTIMATED_ITERATIONS > 1;
}

bool BankSetupOptimizer::shouldUseCaching(const std::string& varName,
                                         int accessCount,
                                         bool inLoop) const {
    // Register caching beneficial when:
    // 1. Accessed multiple times (cost of cache check vs. re-setup)
    // 2. In loops (repeated pattern)
    // 3. Hot variable (frequent access)

    if (!inLoop || accessCount < 3) return false;

    // Caching cost: LDA #bank; CMP ZP; BEQ skip; STA $FFF8
    // ~8 bytes, ~5 cycles per check
    // Setup cost: LDA #bank; STA $FFF8
    // ~4 bytes, ~4 cycles per setup
    // Cache break-even at ~2 setup avoidances

    return accessCount >= 3;  // At least 3 accesses to justify
}

std::vector<std::string> BankSetupOptimizer::getHotVariables(int threshold) const {
    std::vector<std::pair<std::string, int>> vars;

    for (const auto& [varName, pattern] : accessPatterns_) {
        if (pattern.accessCount >= threshold) {
            vars.push_back({varName, pattern.accessCount});
        }
    }

    // Sort by access count (descending)
    std::sort(vars.begin(), vars.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    std::vector<std::string> result;
    for (const auto& [varName, _] : vars) {
        result.push_back(varName);
    }

    return result;
}

void BankSetupOptimizer::recordAccess(const BankAccessPattern& pattern) {
    if (accessPatterns_.count(pattern.variableName)) {
        accessPatterns_[pattern.variableName].accessCount += pattern.accessCount;
        accessPatterns_[pattern.variableName].coAccessedVariables.insert(
            pattern.coAccessedVariables.begin(),
            pattern.coAccessedVariables.end());
    } else {
        accessPatterns_[pattern.variableName] = pattern;
    }
}

int BankSetupOptimizer::computeAccessCost(const std::string& varName) const {
    // Cost of far memory access: ~8 bytes (bank setup + load)
    // Cost of normal memory access: ~3 bytes
    // Savings from optimization: ~5 bytes per optimized access
    return 8;
}

// Bank Cache Manager Implementation

void BankCacheManager::startFunction() {
    currentBank_ = -1;  // Unknown initial state
    loopStack_.clear();
}

void BankCacheManager::recordBankSetup(int bank) {
    currentBank_ = bank;
}

void BankCacheManager::recordBankAccess(int bank) {
    if (bank != currentBank_) {
        currentBank_ = bank;  // Bank changed
    }
}

bool BankCacheManager::isBankCached(int bank) const {
    return currentBank_ == bank && currentBank_ != -1;
}

void BankCacheManager::invalidateCache() {
    currentBank_ = -1;
}

void BankCacheManager::enterLoop(const std::string& loopLabel) {
    loopStack_.push_back(loopLabel);
}

void BankCacheManager::exitLoop() {
    if (!loopStack_.empty()) {
        loopStack_.pop_back();
    }
    // Note: Cache state may need reset on loop exit
    // depending on loop structure (conservative: invalidate)
    if (loopStack_.empty()) {
        invalidateCache();
    }
}

std::string BankCacheManager::generateCacheCheckCode(int targetBank) const {
    std::ostringstream code;

    // Generate code to check if bank is already loaded
    code << "    ; Bank cache check\n";
    code << "    lda #$" << std::hex << targetBank << std::dec << "\n";
    code << "    cmp $" << std::hex << cacheRegister_ << std::dec << "\n";
    code << "    beq @skip_bank_setup\n";

    return code.str();
}

std::string BankCacheManager::generateCacheUpdateCode(int newBank) const {
    std::ostringstream code;

    code << "    ; Update bank cache\n";
    code << "    sta $FFF8               ; Write bank\n";
    code << "    lda #$" << std::hex << newBank << std::dec << "\n";
    code << "    sta $" << std::hex << cacheRegister_ << std::dec << "\n";
    code << "@skip_bank_setup:\n";

    return code.str();
}

std::string BankCacheManager::bankImmediateCode(int bank) const {
    std::ostringstream code;
    code << "lda #$" << std::hex << bank << std::dec;
    return code.str();
}

} // namespace phase99

