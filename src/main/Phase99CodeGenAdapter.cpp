// Phase 99.4.3: CodeGenerator Integration Implementation
#include "Phase99CodeGenAdapter.hpp"
#include <sstream>
#include <algorithm>

namespace phase99 {

void Phase99CodeGenAdapter::initializeBankLayout() {
    cacheManager_.startFunction();
    optimizer_.analyzeBankSetupPlacement();
}

std::map<std::string, FarVarCodeGenDecision> Phase99CodeGenAdapter::analyzeFarVariableCodeGen() {
    decisions_.clear();

    // Get all variables from layout
    for (const auto& assignment : layout_.assignments) {
        FarVarCodeGenDecision decision;
        decision.variableName = assignment.variableName;
        decision.targetBank = assignment.assignedBank;

        // Analyze bank setup opportunities
        auto setupDecisions = optimizer_.analyzeBankSetupPlacement();
        if (setupDecisions.count(assignment.variableName)) {
            const auto& setupDecision = setupDecisions[assignment.variableName];
            decision.hoistBankSetup = setupDecision.canHoistOutsideLoop;
            decision.useCaching = setupDecision.shouldUseRegisterCache;
            decision.cacheRegister = setupDecision.cacheRegisterSlot;
        }

        // Generate optimization hint
        std::ostringstream hint;
        if (decision.hoistBankSetup) {
            hint << "Hoist bank setup outside loop";
        }
        if (decision.useCaching) {
            if (!hint.str().empty()) hint << "; ";
            hint << "Use register caching";
        }
        decision.optimizationHint = hint.str();

        decisions_[assignment.variableName] = decision;
    }

    return decisions_;
}

std::string Phase99CodeGenAdapter::generateBankSetupCode(const std::string& varName) {
    int bank = integrator_.getBankForVariable(varName);
    if (bank < 0) {
        return "";  // Not a far variable
    }

    // Check if we can skip setup (already cached)
    if (cacheManager_.isBankCached(bank)) {
        return "; Bank already cached\n";
    }

    std::ostringstream code;
    code << "    lda #$" << std::hex << bank << std::dec << "\n";
    code << "    sta $FFF8\n";

    // Update cache
    cacheManager_.recordBankSetup(bank);

    return code.str();
}

std::string Phase99CodeGenAdapter::generateLoopOptimizedCode(const std::string& varName,
                                                            bool inLoop) {
    if (!inLoop) {
        return generateBankSetupCode(varName);
    }

    int bank = integrator_.getBankForVariable(varName);
    if (bank < 0) {
        return "";
    }

    // In loop: use cache checking code
    if (decisions_.count(varName) && decisions_[varName].useCaching) {
        std::ostringstream code;

        // Generate cache check
        code << cacheManager_.generateCacheCheckCode(bank);

        // Generate bank setup with cache update
        code << "    lda #$" << std::hex << bank << std::dec << "\n";
        code << cacheManager_.generateCacheUpdateCode(bank);

        cacheManager_.recordBankSetup(bank);
        return code.str();
    }

    // No caching: standard setup (but may be hoisted)
    if (decisions_.count(varName) && decisions_[varName].hoistBankSetup) {
        // Assume already setup before loop
        return "; Bank setup hoisted before loop\n";
    }

    return generateBankSetupCode(varName);
}

void Phase99CodeGenAdapter::enterLoop(const std::string& loopLabel) {
    cacheManager_.enterLoop(loopLabel);
}

void Phase99CodeGenAdapter::exitLoop() {
    cacheManager_.exitLoop();
}

std::string Phase99CodeGenAdapter::getOptimizationHint(const std::string& varName) const {
    if (decisions_.count(varName)) {
        return decisions_.at(varName).optimizationHint;
    }
    return "";
}

void Phase99CodeGenAdapter::recordVariableAccess(const std::string& varName,
                                                int accessCount,
                                                bool inLoop) {
    BankAccessPattern pattern;
    pattern.variableName = varName;
    pattern.accessCount = accessCount;
    pattern.inLoop = inLoop;
    pattern.bank = integrator_.getBankForVariable(varName);

    optimizer_.recordAccess(pattern);
}

std::string Phase99CodeGenAdapter::generateBankImmediate(int bank) const {
    std::ostringstream code;
    code << "lda #$" << std::hex << bank << std::dec;
    return code.str();
}

std::string Phase99CodeGenAdapter::generateBankStore() const {
    return "sta $FFF8";
}

bool Phase99CodeGenAdapter::isCached(const std::string& varName) const {
    int bank = integrator_.getBankForVariable(varName);
    return bank >= 0 && cacheManager_.isBankCached(bank);
}

} // namespace phase99

