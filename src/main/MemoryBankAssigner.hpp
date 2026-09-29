// Phase 99.2: Bank Assignment Optimization Engine
// Computes optimal bank layout for __far variables across modules

#pragma once

#include "CrossModuleVariableDatabase.hpp"
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase99 {

// Result of bank assignment algorithm
struct BankAssignment {
    std::string variableName;
    int assignedBank = -1;
    size_t offsetInBank = 0;
    double estimatedBenefit = 0.0;
};

// Overall bank layout
struct BankLayout {
    std::vector<BankAssignment> assignments;
    std::vector<int> bankUtilization;  // Per-bank usage in bytes
    int totalBanksRequired = 0;
    double totalBenefit = 0.0;
    double averageUtilization = 0.0;
};

// Bank assignment optimization algorithm
class BankAssignmentEngine {
public:
    explicit BankAssignmentEngine(const CrossModuleDatabase& db) : database_(db) {}

    // Main optimization: compute optimal bank layout
    BankLayout optimizeBankAssignment();

    // Advanced options
    void setMaxBanks(int maxBanks) { maxBanks_ = maxBanks; }
    void setAlignmentStrict(bool strict) { alignmentStrict_ = strict; }
    void setCoLocationWeight(double weight) { coLocationWeight_ = weight; }

    // Query results
    int getBankForVariable(const std::string& varName) const;
    size_t getOffsetInBank(const std::string& varName) const;

private:
    const CrossModuleDatabase& database_;
    int maxBanks_ = 256;
    bool alignmentStrict_ = false;
    double coLocationWeight_ = 1.0;

    std::map<std::string, BankAssignment> assignments_;

    // Algorithm stages
    void buildCoLocationGraph();
    void computePackingOrder();
    void runBinPacking();
    void optimizeLocalityWithinBanks();

    // Helper methods
    bool canPlaceInBank(const std::shared_ptr<FarVariableProfile>& var,
                       int bank, size_t currentUsage) const;
    int computeAlignedOffset(size_t currentOffset, int alignment) const;
    double scoreCoLocation(const std::string& var1, const std::string& var2) const;

    // State
    std::map<int, std::vector<std::string>> bankContents_;  // bank -> variables
};

} // namespace phase99

