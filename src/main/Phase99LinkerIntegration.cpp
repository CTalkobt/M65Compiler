// Phase 99.3: Linker Integration Implementation
#include "Phase99LinkerIntegration.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace phase99 {

std::vector<LinkerLayoutDirective> LinkerIntegrator::generateLinkerDirectives() {
    std::vector<LinkerLayoutDirective> directives;

    for (const auto& assignment : layout_.assignments) {
        auto var = database_.getVariable(assignment.variableName);
        if (!var) continue;

        LinkerLayoutDirective directive;
        directive.variableName = assignment.variableName;
        directive.targetBank = assignment.assignedBank;
        directive.offsetInBank = assignment.offsetInBank;
        directive.sizeBytes = var->sizeBytes;

        directives.push_back(directive);
    }

    // Sort by bank, then by offset within bank
    std::sort(directives.begin(), directives.end(),
        [](const auto& a, const auto& b) {
            if (a.targetBank != b.targetBank) {
                return a.targetBank < b.targetBank;
            }
            return a.offsetInBank < b.offsetInBank;
        });

    return directives;
}

std::vector<BankMapEntry> LinkerIntegrator::generateBankMap() {
    std::vector<BankMapEntry> bankMap;

    for (const auto& assignment : layout_.assignments) {
        auto var = database_.getVariable(assignment.variableName);
        if (!var) continue;

        BankMapEntry entry;
        entry.variableName = assignment.variableName;
        entry.bank = assignment.assignedBank;
        entry.offsetInBank = assignment.offsetInBank;
        entry.sizeBytes = var->sizeBytes;

        bankMap.push_back(entry);
    }

    // Sort by variable name for alphabetical reference
    std::sort(bankMap.begin(), bankMap.end(),
        [](const auto& a, const auto& b) {
            return a.variableName < b.variableName;
        });

    return bankMap;
}

bool LinkerIntegrator::validateLayout(std::string& errorMessage) {
    // Check bank capacity constraints
    if (!checkBankCapacityConstraints(errorMessage)) {
        return false;
    }

    // Check alignment constraints
    if (!checkAlignmentConstraints(errorMessage)) {
        return false;
    }

    // Check for address overlaps
    if (!checkAddressOverlaps(errorMessage)) {
        return false;
    }

    return true;
}

std::string LinkerIntegrator::generateOptimizationHints() {
    std::ostringstream hints;

    hints << "; Phase 99.3 Linker Optimization Hints\n";
    hints << "; Bank Layout for Cross-Module Optimization\n\n";

    // Group by bank
    std::map<int, std::vector<std::string>> bankVars;
    for (const auto& assignment : layout_.assignments) {
        bankVars[assignment.assignedBank].push_back(assignment.variableName);
    }

    // Generate hints per bank
    for (const auto& [bank, vars] : bankVars) {
        hints << "; Bank " << bank << " (" << layout_.bankUtilization[bank]
              << "/" << 65536 << " bytes)\n";

        for (const auto& varName : vars) {
            auto var = database_.getVariable(varName);
            if (!var) continue;

            // Find assignment for offset
            size_t offset = 0;
            for (const auto& assign : layout_.assignments) {
                if (assign.variableName == varName && assign.assignedBank == bank) {
                    offset = assign.offsetInBank;
                    break;
                }
            }

            hints << ";   " << varName << " @ offset $"
                  << std::hex << offset << std::dec
                  << " (size: " << var->sizeBytes << ")\n";
        }
        hints << "\n";
    }

    // Bank switch optimization hints
    hints << "; Bank Switch Optimization\n";
    hints << "; Total benefit from co-location: "
          << std::fixed << std::setprecision(0) << layout_.totalBenefit
          << " cycles\n";
    hints << "; Average bank utilization: "
          << std::fixed << std::setprecision(1)
          << (layout_.averageUtilization * 100.0) << "%\n";
    hints << "; Banks required: " << layout_.totalBanksRequired << "\n";

    return hints.str();
}

int LinkerIntegrator::getBankForVariable(const std::string& varName) const {
    for (const auto& assignment : layout_.assignments) {
        if (assignment.variableName == varName) {
            return assignment.assignedBank;
        }
    }
    return -1;
}

std::vector<LinkerLayoutDirective> LinkerIntegrator::computeRelocationOrder() {
    return generateLinkerDirectives();
}

bool LinkerIntegrator::checkBankCapacityConstraints(std::string& error) const {
    const size_t BANK_SIZE = 65536;

    for (size_t bank = 0; bank < layout_.bankUtilization.size(); bank++) {
        if ((size_t)layout_.bankUtilization[bank] > BANK_SIZE) {
            std::ostringstream oss;
            oss << "Bank " << bank << " exceeds 64KB capacity: "
                << layout_.bankUtilization[bank] << " bytes";
            error = oss.str();
            return false;
        }
    }

    return true;
}

bool LinkerIntegrator::checkAlignmentConstraints(std::string& error) const {
    for (const auto& assignment : layout_.assignments) {
        auto var = database_.getVariable(assignment.variableName);
        if (!var || var->alignment <= 1) continue;

        // Check if offset respects alignment
        if (assignment.offsetInBank % var->alignment != 0) {
            std::ostringstream oss;
            oss << "Variable " << assignment.variableName
                << " alignment violated: offset " << assignment.offsetInBank
                << " not aligned to " << var->alignment;
            error = oss.str();
            return false;
        }
    }

    return true;
}

bool LinkerIntegrator::checkAddressOverlaps(std::string& error) const {
    // Build address ranges per bank
    std::map<int, std::vector<std::pair<size_t, size_t>>> bankRanges;

    for (const auto& assignment : layout_.assignments) {
        auto var = database_.getVariable(assignment.variableName);
        if (!var) continue;

        int bank = assignment.assignedBank;
        size_t start = assignment.offsetInBank;
        size_t end = start + var->sizeBytes;

        // Check overlap with existing ranges in this bank
        if (bankRanges.count(bank)) {
            for (const auto& [existStart, existEnd] : bankRanges[bank]) {
                // Check for overlap
                if ((start < existEnd && start >= existStart) ||
                    (end > existStart && end <= existEnd) ||
                    (start <= existStart && end >= existEnd)) {
                    std::ostringstream oss;
                    oss << "Address overlap in bank " << bank
                        << ": " << assignment.variableName
                        << " [$" << std::hex << start << "-" << end << std::dec
                        << "]";
                    error = oss.str();
                    return false;
                }
            }
        }

        bankRanges[bank].push_back({start, end});
    }

    return true;
}

} // namespace phase99

