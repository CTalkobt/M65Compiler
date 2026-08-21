// Phase 99.2: Bank Assignment Engine Implementation
#include "Phase99BankAssignmentEngine.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace phase99 {

BankLayout BankAssignmentEngine::optimizeBankAssignment() {
    assignments_.clear();
    bankContents_.clear();

    // Stage 1: Build co-location graph
    buildCoLocationGraph();

    // Stage 2: Compute packing order (hot variables first)
    computePackingOrder();

    // Stage 3: Run bin-packing algorithm
    runBinPacking();

    // Stage 4: Optimize locality within banks
    optimizeLocalityWithinBanks();

    // Build result structure
    BankLayout layout;
    const size_t BANK_SIZE = 65536;

    for (const auto& [varName, assignment] : assignments_) {
        layout.assignments.push_back(assignment);
    }

    // Compute bank utilization stats
    for (int bank = 0; bank < maxBanks_; bank++) {
        if (bankContents_.count(bank) && !bankContents_[bank].empty()) {
            size_t usage = 0;
            for (const auto& varName : bankContents_[bank]) {
                if (assignments_.count(varName)) {
                    auto var = database_.getVariable(varName);
                    if (var) {
                        usage += var->sizeBytes;
                    }
                }
            }
            layout.bankUtilization.push_back(usage);
            layout.totalBenefit += assignments_[bankContents_[bank][0]].estimatedBenefit;
            layout.totalBanksRequired++;
        }
    }

    // Calculate average utilization
    if (layout.totalBanksRequired > 0) {
        size_t totalUsed = 0;
        for (size_t util : layout.bankUtilization) {
            totalUsed += util;
        }
        layout.averageUtilization = (double)totalUsed / (layout.totalBanksRequired * BANK_SIZE);
    }

    return layout;
}

int BankAssignmentEngine::getBankForVariable(const std::string& varName) const {
    auto it = assignments_.find(varName);
    return (it != assignments_.end()) ? it->second.assignedBank : -1;
}

size_t BankAssignmentEngine::getOffsetInBank(const std::string& varName) const {
    auto it = assignments_.find(varName);
    return (it != assignments_.end()) ? it->second.offsetInBank : 0;
}

void BankAssignmentEngine::buildCoLocationGraph() {
    // Identify co-location opportunities
    auto pairs = database_.getCoLocationPairs();

    for (const auto& [var1, var2] : pairs) {
        int coAccessCount = database_.getCoAccessCount(var1, var2);
        if (coAccessCount > 0) {
            auto v1 = database_.getVariable(var1);
            auto v2 = database_.getVariable(var2);
            if (v1 && v2) {
                v1->coLocationBenefit += coAccessCount * 4.0;  // 4 cycle bank switch
                v2->coLocationBenefit += coAccessCount * 4.0;
            }
        }
    }
}

void BankAssignmentEngine::computePackingOrder() {
    // Sort variables by:
    // 1. Co-location benefit (descending)
    // 2. Access frequency (descending)
    // 3. Size (descending)

    auto allVars = database_.getAllVariables();
    std::sort(allVars.begin(), allVars.end(),
        [](const auto& a, const auto& b) {
            if (a->coLocationBenefit != b->coLocationBenefit) {
                return a->coLocationBenefit > b->coLocationBenefit;
            }
            if (a->totalAccessCount != b->totalAccessCount) {
                return a->totalAccessCount > b->totalAccessCount;
            }
            return a->sizeBytes > b->sizeBytes;
        });

    // Clear and rebuild assignments in order
    for (auto& var : allVars) {
        BankAssignment assignment;
        assignment.variableName = var->name;
        assignment.estimatedBenefit = var->coLocationBenefit;
        assignments_[var->name] = assignment;
    }
}

void BankAssignmentEngine::runBinPacking() {
    const size_t BANK_SIZE = 65536;

    // Pack variables into banks using First-Fit Decreasing algorithm
    for (const auto& [varName, assignment] : assignments_) {
        auto var = database_.getVariable(varName);
        if (!var) continue;

        bool placed = false;

        // Try to fit in existing bank
        for (int bank = 0; bank < maxBanks_; bank++) {
            if (!bankContents_.count(bank)) continue;

            size_t currentUsage = 0;
            for (const auto& existingVar : bankContents_[bank]) {
                auto ev = database_.getVariable(existingVar);
                if (ev) {
                    currentUsage += ev->sizeBytes;
                }
            }

            if (canPlaceInBank(var, bank, currentUsage)) {
                // Prefer banks with co-location candidates
                int coLocationCount = 0;
                for (const auto& existingVar : bankContents_[bank]) {
                    if (var->coLocationCandidates.count(existingVar)) {
                        coLocationCount++;
                    }
                }

                if (coLocationCount > 0 || currentUsage == 0) {
                    size_t offset = computeAlignedOffset(currentUsage, var->alignment);
                    assignments_[varName].assignedBank = bank;
                    assignments_[varName].offsetInBank = offset;
                    bankContents_[bank].push_back(varName);
                    placed = true;
                    break;
                }
            }
        }

        // Create new bank if needed
        if (!placed) {
            int newBank = 0;
            while (newBank < maxBanks_ && bankContents_.count(newBank)) {
                newBank++;
            }

            if (newBank < maxBanks_) {
                size_t offset = computeAlignedOffset(0, var->alignment);
                assignments_[varName].assignedBank = newBank;
                assignments_[varName].offsetInBank = offset;
                bankContents_[newBank].push_back(varName);
            }
        }
    }
}

void BankAssignmentEngine::optimizeLocalityWithinBanks() {
    // Reorder variables within each bank for better cache locality
    for (auto& [bank, variables] : bankContents_) {
        // Sort by access frequency (descending)
        std::sort(variables.begin(), variables.end(),
            [this](const std::string& a, const std::string& b) {
                auto va = database_.getVariable(a);
                auto vb = database_.getVariable(b);
                if (!va || !vb) return false;
                return va->totalAccessCount > vb->totalAccessCount;
            });

        // Recompute offsets after reordering
        size_t offset = 0;
        for (const auto& varName : variables) {
            auto var = database_.getVariable(varName);
            if (var) {
                offset = computeAlignedOffset(offset, var->alignment);
                assignments_[varName].offsetInBank = offset;
                offset += var->sizeBytes;
            }
        }
    }
}

bool BankAssignmentEngine::canPlaceInBank(const std::shared_ptr<FarVariableProfile>& var,
                                         int bank, size_t currentUsage) const {
    const size_t BANK_SIZE = 65536;
    size_t alignedOffset = computeAlignedOffset(currentUsage, var->alignment);
    return (alignedOffset + var->sizeBytes) <= BANK_SIZE;
}

int BankAssignmentEngine::computeAlignedOffset(size_t currentOffset, int alignment) const {
    if (alignment <= 1) return currentOffset;

    // Align to next multiple of alignment
    size_t remainder = currentOffset % alignment;
    if (remainder == 0) return currentOffset;
    return currentOffset + (alignment - remainder);
}

double BankAssignmentEngine::scoreCoLocation(const std::string& var1,
                                            const std::string& var2) const {
    int coAccessCount = database_.getCoAccessCount(var1, var2);
    return coAccessCount * coLocationWeight_;
}

} // namespace phase99

