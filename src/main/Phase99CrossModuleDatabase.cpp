// Phase 99.1: Cross-Module Address Space Database Implementation
#include "Phase99CrossModuleDatabase.hpp"
#include <algorithm>
#include <cmath>

namespace phase99 {

void CrossModuleDatabase::recordFarVariable(const std::string& varName,
                                            const std::string& moduleName,
                                            size_t sizeBytes,
                                            int alignment) {
    auto var = getOrCreateVariable(varName, moduleName);
    var->sizeBytes = sizeBytes;
    var->alignment = alignment;
    modules_.insert(moduleName);
}

void CrossModuleDatabase::recordVariableAccess(const std::string& varName,
                                              const std::string& functionName,
                                              const std::string& moduleName,
                                              int accessCount) {
    auto var = getOrCreateVariable(varName, moduleName);

    auto it = var->accessPatterns.find(functionName);
    if (it == var->accessPatterns.end()) {
        AccessPattern pattern;
        pattern.functionName = functionName;
        pattern.accessCount = accessCount;
        var->accessPatterns[functionName] = pattern;
    } else {
        it->second.accessCount += accessCount;
    }

    var->totalAccessCount += accessCount;
    var->accessingModules.insert(moduleName);
}

void CrossModuleDatabase::recordCoAccess(const std::string& var1,
                                        const std::string& var2,
                                        const std::string& functionName,
                                        int coAccessCount) {
    // Normalize key (smaller name first for consistency)
    auto key = std::make_pair(
        var1 < var2 ? var1 : var2,
        var1 < var2 ? var2 : var1
    );

    coAccessMatrix_[key] += coAccessCount;

    // Update reciprocal co-access records
    if (variables_.count(var1)) {
        variables_[var1]->coLocationCandidates.insert(var2);
    }
    if (variables_.count(var2)) {
        variables_[var2]->coLocationCandidates.insert(var1);
    }
}

void CrossModuleDatabase::analyzeCoLocationBenefits() {
    // For each variable, compute co-location benefits
    for (auto& [varName, var] : variables_) {
        double maxBenefit = 0.0;
        int maxCoAccess = 0;

        for (const auto& coVar : var->coLocationCandidates) {
            auto key = std::make_pair(
                varName < coVar ? varName : coVar,
                varName < coVar ? coVar : varName
            );

            if (coAccessMatrix_.count(key)) {
                int coAccessCount = coAccessMatrix_[key];
                // Benefit = co-access count * estimated bank switch savings
                // Bank switch costs ~4 cycles, saved per co-access
                double benefit = coAccessCount * 4.0;

                if (benefit > maxBenefit) {
                    maxBenefit = benefit;
                }
                maxCoAccess = std::max(maxCoAccess, coAccessCount);
            }
        }

        var->coLocationBenefit = maxBenefit;
    }
}

void CrossModuleDatabase::computeBankAssignmentCosts() {
    // Estimate cost of each variable in its own bank
    const size_t BANK_SIZE = 65536;  // 64KB banks

    std::vector<std::shared_ptr<FarVariableProfile>> sorted;
    for (auto& [name, var] : variables_) {
        sorted.push_back(var);
    }

    // Sort by total access count (descending) - hot variables first
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a->totalAccessCount > b->totalAccessCount;
        });

    // Bin-packing with co-location hints
    std::map<int, size_t> bankUsage;
    int nextBank = 0;

    for (auto& var : sorted) {
        // Try to find co-location partner in existing bank
        bool placed = false;

        for (int bank = 0; bank < nextBank; bank++) {
            if (bankUsage[bank] + var->sizeBytes <= BANK_SIZE) {
                // Check if any co-location candidates are in this bank
                bool hasCoCandidates = false;
                for (const auto& coVar : var->coLocationCandidates) {
                    if (variables_[coVar]->suggestedBank == bank) {
                        hasCoCandidates = true;
                        break;
                    }
                }

                if (hasCoCandidates || bankUsage[bank] == 0) {
                    var->suggestedBank = bank;
                    bankUsage[bank] += var->sizeBytes;
                    placed = true;
                    break;
                }
            }
        }

        // Create new bank if needed
        if (!placed) {
            if (var->sizeBytes <= BANK_SIZE) {
                var->suggestedBank = nextBank;
                bankUsage[nextBank] = var->sizeBytes;
                nextBank++;
            }
        }
    }
}

std::vector<std::shared_ptr<FarVariableProfile>> CrossModuleDatabase::getAllVariables() const {
    std::vector<std::shared_ptr<FarVariableProfile>> result;
    for (const auto& [name, var] : variables_) {
        result.push_back(var);
    }
    return result;
}

std::shared_ptr<FarVariableProfile> CrossModuleDatabase::getVariable(const std::string& varName) const {
    auto it = variables_.find(varName);
    if (it != variables_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<FarVariableProfile>> CrossModuleDatabase::getVariablesByModule(
    const std::string& moduleName) const {
    std::vector<std::shared_ptr<FarVariableProfile>> result;
    for (const auto& [name, var] : variables_) {
        if (var->moduleName == moduleName) {
            result.push_back(var);
        }
    }
    return result;
}

std::vector<std::pair<std::string, std::string>> CrossModuleDatabase::getCoLocationPairs() const {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& [pair, count] : coAccessMatrix_) {
        if (count > 0) {
            result.push_back(pair);
        }
    }
    // Sort by co-access count (descending)
    std::sort(result.begin(), result.end(),
        [this](const auto& a, const auto& b) {
            return coAccessMatrix_.at(a) > coAccessMatrix_.at(b);
        });
    return result;
}

int CrossModuleDatabase::getCoAccessCount(const std::string& var1, const std::string& var2) const {
    auto key = std::make_pair(
        var1 < var2 ? var1 : var2,
        var1 < var2 ? var2 : var1
    );
    auto it = coAccessMatrix_.find(key);
    return (it != coAccessMatrix_.end()) ? it->second : 0;
}

size_t CrossModuleDatabase::getTotalFarMemoryNeeded() const {
    size_t total = 0;
    for (const auto& [name, var] : variables_) {
        total += var->sizeBytes;
    }
    return total;
}

double CrossModuleDatabase::getAverageBankUtilization() const {
    const size_t BANK_SIZE = 65536;
    if (variables_.empty()) return 0.0;

    std::set<int> banksUsed;
    size_t totalUsed = 0;

    for (const auto& [name, var] : variables_) {
        if (var->suggestedBank >= 0) {
            banksUsed.insert(var->suggestedBank);
            totalUsed += var->sizeBytes;
        }
    }

    if (banksUsed.empty()) return 0.0;

    size_t totalCapacity = banksUsed.size() * BANK_SIZE;
    return (double)totalUsed / totalCapacity;
}

std::shared_ptr<FarVariableProfile> CrossModuleDatabase::getOrCreateVariable(
    const std::string& varName,
    const std::string& moduleName) {
    auto it = variables_.find(varName);
    if (it != variables_.end()) {
        return it->second;
    }

    auto var = std::make_shared<FarVariableProfile>();
    var->name = varName;
    var->moduleName = moduleName;
    variables_[varName] = var;
    return var;
}

} // namespace phase99

