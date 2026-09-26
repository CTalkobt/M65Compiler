// Phase 99.1: Cross-Module Address Space Database
// Tracks __far variables and access patterns across translation units

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase99 {

// Access pattern information for a variable
struct AccessPattern {
    std::string functionName;
    int accessCount = 0;
    bool isSequential = false;      // Accessed with other variables in sequence
    bool isLoopNested = false;      // Accessed inside loops
    std::set<std::string> coAccessedVariables;  // Variables accessed together
};

// Complete profile of a __far variable
struct FarVariableProfile {
    std::string name;
    std::string moduleName;
    size_t sizeBytes = 0;
    int alignment = 1;              // Power of 2 alignment requirement
    bool isArray = false;
    int arrayDimensions = 0;

    // Access information
    std::map<std::string, AccessPattern> accessPatterns;  // Per-function patterns
    int totalAccessCount = 0;
    std::set<std::string> accessingModules;

    // Optimization hints
    int suggestedBank = -1;
    double coLocationBenefit = 0.0;  // Estimated benefit of co-location
    std::set<std::string> coLocationCandidates;
};

// Cross-module database for far variable analysis
class CrossModuleDatabase {
public:
    CrossModuleDatabase() = default;
    ~CrossModuleDatabase() = default;

    // Recording phase: Collect variable information
    void recordFarVariable(const std::string& varName,
                         const std::string& moduleName,
                         size_t sizeBytes,
                         int alignment = 1);

    void recordVariableAccess(const std::string& varName,
                             const std::string& functionName,
                             const std::string& moduleName,
                             int accessCount = 1);

    void recordCoAccess(const std::string& var1,
                       const std::string& var2,
                       const std::string& functionName,
                       int coAccessCount);

    // Analysis phase: Compute optimization benefits
    void analyzeCoLocationBenefits();
    void computeBankAssignmentCosts();

    // Query phase: Get optimization data
    std::vector<std::shared_ptr<FarVariableProfile>> getAllVariables() const;
    std::shared_ptr<FarVariableProfile> getVariable(const std::string& varName) const;
    std::vector<std::shared_ptr<FarVariableProfile>> getVariablesByModule(const std::string& moduleName) const;

    // Cross-module queries
    std::vector<std::pair<std::string, std::string>> getCoLocationPairs() const;
    int getCoAccessCount(const std::string& var1, const std::string& var2) const;

    // Statistics
    int getVariableCount() const { return variables_.size(); }
    int getModuleCount() const { return modules_.size(); }
    size_t getTotalFarMemoryNeeded() const;
    double getAverageBankUtilization() const;

private:
    // Storage
    std::map<std::string, std::shared_ptr<FarVariableProfile>> variables_;
    std::set<std::string> modules_;

    // Co-access tracking (var1 + var2 → count)
    std::map<std::pair<std::string, std::string>, int> coAccessMatrix_;

    // Helper methods
    std::shared_ptr<FarVariableProfile> getOrCreateVariable(const std::string& varName,
                                                            const std::string& moduleName);
};

} // namespace phase99

