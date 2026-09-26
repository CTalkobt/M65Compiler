// Phase 99.4: Bank Setup Optimization for Code Generation
// Optimizes bank setup instruction placement and caching

#pragma once

#include "MemoryBankAssigner.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase99 {

// Decision for bank setup placement
struct BankSetupDecision {
    std::string variableName;
    int targetBank = -1;
    bool canHoistOutsideLoop = false;
    bool shouldUseRegisterCache = false;
    int cacheRegisterSlot = -1;
    bool isHotVariable = false;
};

// Bank access pattern info
struct BankAccessPattern {
    std::string variableName;
    int bank = -1;
    int accessCount = 0;
    bool inLoop = false;
    bool inNestedLoop = false;
    std::set<std::string> coAccessedVariables;
};

// Bank setup optimizer
class BankSetupOptimizer {
public:
    explicit BankSetupOptimizer(const BankLayout& layout)
        : layout_(layout) {}

    // Analyze bank setup opportunities
    std::map<std::string, BankSetupDecision> analyzeBankSetupPlacement();

    // Determine if bank setup can be hoisted outside loop
    bool canHoistBankSetup(const std::string& varName,
                          bool inLoop,
                          int coAccessCount) const;

    // Check if variable benefits from register caching
    bool shouldUseCaching(const std::string& varName,
                         int accessCount,
                         bool inLoop) const;

    // Get hot variables (frequently accessed)
    std::vector<std::string> getHotVariables(int threshold = 10) const;

    // Record access pattern for analysis
    void recordAccess(const BankAccessPattern& pattern);

private:
    const BankLayout& layout_;
    std::map<std::string, BankAccessPattern> accessPatterns_;

    // Helper methods
    int computeSetupCost() const { return 4; }  // 4 bytes for bank setup
    int computeAccessCost(const std::string& varName) const;
    double computeCachingBenefit(const BankAccessPattern& pattern) const;
};

// Bank state tracker for register caching
class BankCacheManager {
public:
    BankCacheManager() : currentBank_(-1), cacheRegister_(0x20) {}

    // Start tracking cache state for a function
    void startFunction();

    // Record bank setup operation
    void recordBankSetup(int bank);

    // Record bank access
    void recordBankAccess(int bank);

    // Check if bank is currently cached
    bool isBankCached(int bank) const;

    // Get cache register location
    int getCacheRegister() const { return cacheRegister_; }

    // Reset cache state (branch, function call, etc.)
    void invalidateCache();

    // Track loop context
    void enterLoop(const std::string& loopLabel);
    void exitLoop();

    // Generate cache update code
    std::string generateCacheCheckCode(int targetBank) const;
    std::string generateCacheUpdateCode(int newBank) const;

    // Query current state
    int getCurrentBank() const { return currentBank_; }
    bool isInLoop() const { return !loopStack_.empty(); }

private:
    int currentBank_;
    int cacheRegister_;
    std::vector<std::string> loopStack_;

    // Helper
    std::string bankImmediateCode(int bank) const;
};

} // namespace phase99

