// Phase 103: Cross-Module Enhancement Implementation
#include "Phase103CrossModuleEnhancer.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace phase103 {

std::vector<InterModuleRelationship>
Phase103CrossModuleEnhancer::analyzeInterModuleAccess(
    const std::vector<std::string>& moduleNames) {
    std::vector<InterModuleRelationship> relationships;

    // Simulate inter-module access analysis
    for (size_t i = 0; i < moduleNames.size(); ++i) {
        for (size_t j = 0; j < moduleNames.size(); ++j) {
            if (i == j) continue;  // Skip intra-module

            // Create relationships between modules
            InterModuleRelationship rel;
            rel.sourceModule = moduleNames[i];
            rel.targetModule = moduleNames[j];
            rel.variableName = "var_" + moduleNames[j];
            rel.accessCount = 100 + (i * 10);  // Simulated
            rel.callFrequency = 50 + (j * 5);
            rel.isHotPath = rel.accessCount > 150;
            rel.benefit = calculateInterModuleBenefit(rel);

            relationships.push_back(rel);
            totalCrossModuleAccesses_ += rel.accessCount;
            relationshipsByModule_[rel.sourceModule].push_back(rel);
        }
    }

    return relationships;
}

std::vector<CoLocationGroup>
Phase103CrossModuleEnhancer::findCoLocationGroups(
    const std::vector<InterModuleRelationship>& relationships) {
    std::vector<CoLocationGroup> groups;

    // Group related variables
    std::map<std::string, std::vector<const InterModuleRelationship*>> varGroups;

    for (const auto& rel : relationships) {
        varGroups[rel.targetModule].push_back(&rel);
    }

    // Create co-location groups from relationships
    int groupCounter = 0;
    for (const auto& [targetModule, rels] : varGroups) {
        if (rels.empty()) continue;

        CoLocationGroup group;
        group.groupId = "cg_" + std::to_string(groupCounter++);

        for (const auto* rel : rels) {
            group.variables.push_back(rel->variableName);
            group.affectedModules.push_back(rel->sourceModule);
            group.expectedBenefit += rel->benefit;
        }

        // Remove duplicates
        std::sort(group.affectedModules.begin(), group.affectedModules.end());
        group.affectedModules.erase(
            std::unique(group.affectedModules.begin(),
                       group.affectedModules.end()),
            group.affectedModules.end());

        group.estimatedSize = estimateGroupSize(group);
        group.isSafe = validateCrossModuleConstraints(group);

        if (group.isSafe) {
            groups.push_back(group);
            identifiedGroups_++;
            totalBenefit_ += group.expectedBenefit;
        }
    }

    return groups;
}

std::vector<CoLocationGroup>
Phase103CrossModuleEnhancer::groupVariablesForBanking(
    const std::vector<CoLocationGroup>& groups) {
    std::vector<CoLocationGroup> bankedGroups = groups;

    // Assign banks (MEGA65 has multiple 64KB banks)
    int currentBank = 0;
    const int maxBanks = 16;  // Simplified

    for (auto& group : bankedGroups) {
        if (group.estimatedSize <= 65536) {  // Fits in one bank
            group.preferredBank = currentBank;
            currentBank = (currentBank + 1) % maxBanks;
        } else {
            // Need multiple banks - split group
            group.preferredBank = -1;  // Mark for further processing
            group.constraints.push_back("Spans multiple banks");
        }
    }

    return bankedGroups;
}

bool Phase103CrossModuleEnhancer::validateCrossModuleConstraints(
    const CoLocationGroup& group) {
    // Check constraints
    if (group.variables.empty()) {
        return false;
    }

    if (group.affectedModules.size() > 10) {
        return false;  // Too many modules
    }

    if (group.estimatedSize > 131072) {
        return false;  // Exceeds 2-bank capacity
    }

    return true;
}

double Phase103CrossModuleEnhancer::calculateInterModuleBenefit(
    const InterModuleRelationship& relationship) {
    // Benefit = access frequency * access cost reduction
    double baseBenefit = relationship.accessCount * 2.0;  // Cost per access

    if (relationship.isHotPath) {
        baseBenefit *= 1.5;  // 50% bonus for hot paths
    }

    return baseBenefit;
}

std::vector<std::shared_ptr<phase100::OptimizationHint>>
Phase103CrossModuleEnhancer::generateCrossModuleHints(
    const CoLocationGroup& group) {
    std::vector<std::shared_ptr<phase100::OptimizationHint>> hints;

    // Generate co-location hint
    auto hint = std::make_shared<phase100::OptimizationHint>();
    hint->phase = "103";
    hint->hintType = "cross_module_co_location";
    hint->targetVariable = group.groupId;
    hint->estimatedBenefit = (int)group.expectedBenefit;
    hint->priority = 9;

    // Add properties
    hint->properties["group_id"] = group.groupId;
    hint->properties["variable_count"] = std::to_string(group.variables.size());
    hint->properties["module_count"] = std::to_string(group.affectedModules.size());

    if (group.preferredBank >= 0) {
        hint->properties["bank"] = std::to_string(group.preferredBank);
    }

    hints.push_back(hint);

    return hints;
}

std::string Phase103CrossModuleEnhancer::generateEnhancementReport(
    const std::vector<CoLocationGroup>& groups) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 103: Cross-Module Enhancement Report                 ║\n";
    report << "║     Inter-Module Optimization Analysis                         ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "ENHANCEMENT SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Cross-Module Accesses: " << totalCrossModuleAccesses_ << "\n";
    report << "Identified Co-Location Groups: " << identifiedGroups_ << "\n";
    report << "Total Estimated Benefit:      " << std::fixed << std::setprecision(0)
           << totalBenefit_ << " bytes\n\n";

    report << "CO-LOCATION GROUPS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Group ID          Variables  Modules  Size    Benefit  Bank\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& group : groups) {
        report << formatGroupRow(group);
    }

    report << "\n";

    // High-value groups
    if (!groups.empty()) {
        report << "HIGH-VALUE CO-LOCATION OPPORTUNITIES\n";
        report << "──────────────────────────────────────────────────────────────────\n";

        std::vector<const CoLocationGroup*> sorted;
        for (const auto& group : groups) {
            sorted.push_back(&group);
        }

        std::sort(sorted.begin(), sorted.end(),
            [](const auto* a, const auto* b) {
                return a->expectedBenefit > b->expectedBenefit;
            });

        for (size_t i = 0; i < sorted.size() && i < 5; ++i) {
            const auto* group = sorted[i];
            report << "\n" << (i + 1) << ". Group " << group->groupId << "\n";
            report << "   Variables:  " << group->variables.size() << "\n";
            report << "   Modules:    " << group->affectedModules.size() << "\n";
            report << "   Size:       " << group->estimatedSize << " bytes\n";
            report << "   Benefit:    " << std::fixed << std::setprecision(0)
                   << group->expectedBenefit << " bytes\n";
        }
    }

    report << "\n";
    return report.str();
}

bool Phase103CrossModuleEnhancer::canCoLocate(
    const InterModuleRelationship& r1,
    const InterModuleRelationship& r2) {
    // Can colocate if same target module
    if (r1.targetModule != r2.targetModule) {
        return false;
    }

    // Can colocate if compatible access patterns
    if (r1.isHotPath != r2.isHotPath) {
        return true;  // Different hotness is OK
    }

    return true;
}

int Phase103CrossModuleEnhancer::estimateGroupSize(const CoLocationGroup& group) {
    // Estimate: ~50 bytes per variable + overhead
    return group.variables.size() * 50 + 100;
}

std::string Phase103CrossModuleEnhancer::formatGroupRow(
    const CoLocationGroup& group) {
    std::stringstream row;

    row << std::left << std::setw(17) << group.groupId
        << "  " << std::right << std::setw(9) << group.variables.size()
        << "  " << std::setw(7) << group.affectedModules.size()
        << "  " << std::setw(6) << group.estimatedSize
        << "  " << std::setw(7) << std::fixed << std::setprecision(0)
        << group.expectedBenefit
        << "  " << (group.preferredBank >= 0 ?
                    std::to_string(group.preferredBank) : "auto") << "\n";

    return row.str();
}

} // namespace phase103
