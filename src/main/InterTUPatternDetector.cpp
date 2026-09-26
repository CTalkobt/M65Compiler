#include "InterTUPatternDetector.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace phase96_5 {

InterTUPatternDetector::InterTUPatternDetector() {
}

void InterTUPatternDetector::recordFieldAccess(const std::string& functionName,
                                               const std::string& structName,
                                               const std::string& fieldName,
                                               bool isLoad,
                                               bool isDereference) {
    std::string fieldKey = structName + "::" + fieldName;

    if (isLoad) {
        functionFieldAccesses[functionName].insert({fieldKey, "read"});
    } else {
        functionFieldAccesses[functionName].insert({fieldKey, "write"});
    }

    if (isDereference) {
        functionFieldAccesses[functionName].insert({fieldKey, "deref"});
    }
}

void InterTUPatternDetector::recordFunctionCall(const std::string& caller,
                                                const std::string& callee) {
    callGraph[caller].insert(callee);
}

void InterTUPatternDetector::recordFieldAliasing(const std::string& field1,
                                                  const std::string& field2) {
    // Find or create alias group
    bool foundGroup = false;
    for (auto& [group, fields] : aliasGroups) {
        if (fields.count(field1) > 0) {
            fields.insert(field2);
            foundGroup = true;
            break;
        }
        if (fields.count(field2) > 0) {
            fields.insert(field1);
            foundGroup = true;
            break;
        }
    }

    if (!foundGroup) {
        // Create new group
        std::string groupName = field1 + "_alias_group";
        aliasGroups[groupName].insert(field1);
        aliasGroups[groupName].insert(field2);
    }
}

void InterTUPatternDetector::detectPatterns() {
    // Clear previous results
    detectedPatterns.clear();

    // Run all pattern detectors
    detectSequentialFieldAccess();
    detectLoopNestedAccess();
    detectFieldReuse();
    detectPointerDereference();
    detectAliasChains();

    // Sort by savings estimate (descending)
    std::sort(detectedPatterns.begin(), detectedPatterns.end(),
              [](const auto& a, const auto& b) {
                  return a.savingsEstimate > b.savingsEstimate;
              });
}

void InterTUPatternDetector::detectSequentialFieldAccess() {
    // Pattern: Access field1 then field2 in same function
    // Example: mesh[i][j].vertices then mesh[i][j].vertices->x

    for (const auto& [funcName, accesses] : functionFieldAccesses) {
        // accesses is set<pair<string, string>> - extract field keys
        std::vector<std::string> accessList;
        for (const auto& [fieldKey, accessType] : accesses) {
            accessList.push_back(fieldKey);
        }

        for (size_t i = 0; i < accessList.size(); i++) {
            for (size_t j = i + 1; j < accessList.size(); j++) {
                const auto& field1 = accessList[i];
                const auto& field2 = accessList[j];

                // Check if they're in same struct
                auto colon1 = field1.find("::");
                auto colon2 = field2.find("::");
                if (colon1 == std::string::npos || colon2 == std::string::npos) continue;

                std::string struct1 = field1.substr(0, colon1);
                std::string struct2 = field2.substr(0, colon2);

                if (struct1 != struct2) continue;

                // This is a sequential access pattern
                DetectedPattern pattern;
                pattern.type = PatternType::SequentialFieldAccess;
                pattern.description = field1 + " -> " + field2;
                pattern.involvingFunctions.insert(funcName);
                pattern.involvingStructs.insert(struct1);
                pattern.involvingFields.insert(field1.substr(colon1 + 2));
                pattern.involvingFields.insert(field2.substr(colon2 + 2));
                pattern.costWithoutOptimization = 20.0;   // Two offset calculations
                pattern.costWithOptimization = 5.0;       // One cached pointer
                pattern.savingsEstimate = 15.0;           // Typical savings
                pattern.frequency = 1;

                detectedPatterns.push_back(pattern);
            }
        }
    }
}

void InterTUPatternDetector::detectLoopNestedAccess() {
    // Pattern: Field accessed in nested loops
    // This would require loop depth information recorded separately
    // Placeholder for now

    for (const auto& [funcName, accesses] : functionFieldAccesses) {
        if (accesses.size() > 2) {  // Heuristic: multiple accesses in same function
            for (const auto& access : accesses) {
                const auto& fieldKey = access;
                if (fieldKey.second == "read" || fieldKey.second == "deref") {
                    DetectedPattern pattern;
                    pattern.type = PatternType::LoopNestedFieldAccess;
                    pattern.description = "Loop-nested: " + fieldKey.first;
                    pattern.involvingFunctions.insert(funcName);
                    pattern.costWithoutOptimization = 30.0;   // Repeated calculations
                    pattern.costWithOptimization = 3.0;       // Cached in loop
                    pattern.savingsEstimate = 25.0;
                    pattern.frequency = 3;  // Typical loop iterations

                    detectedPatterns.push_back(pattern);
                }
            }
        }
    }
}

void InterTUPatternDetector::detectFieldReuse() {
    // Pattern: Same field accessed multiple times in function
    // Example: mesh[i][j].vertices, then again mesh[i][j].vertices

    for (const auto& [funcName, accesses] : functionFieldAccesses) {
        std::map<std::string, int> accessCounts;

        for (const auto& access : accesses) {
            accessCounts[access.first]++;
        }

        for (const auto& [field, count] : accessCounts) {
            if (count > 1) {
                DetectedPattern pattern;
                pattern.type = PatternType::FieldReuse;
                pattern.description = field + " accessed " + std::to_string(count) + " times";
                pattern.involvingFunctions.insert(funcName);
                pattern.costWithoutOptimization = count * 10.0;
                pattern.costWithOptimization = 2.0;  // Cache pointer once
                pattern.savingsEstimate = (count - 1) * 8.0;  // Savings per reuse
                pattern.frequency = count;

                detectedPatterns.push_back(pattern);
            }
        }
    }
}

void InterTUPatternDetector::detectPointerDereference() {
    // Pattern: Load pointer field then dereference in same function
    // Example: ptr = mesh[i][j].vertices; x = ptr->x

    for (const auto& [funcName, accesses] : functionFieldAccesses) {
        bool hasLoad = false;
        bool hasDeref = false;

        for (const auto& access : accesses) {
            if (access.second == "read") hasLoad = true;
            if (access.second == "deref") hasDeref = true;
        }

        if (hasLoad && hasDeref) {
            DetectedPattern pattern;
            pattern.type = PatternType::PointerDereference;
            pattern.description = "Pointer load + dereference in " + funcName;
            pattern.involvingFunctions.insert(funcName);
            pattern.costWithoutOptimization = 15.0;   // Load + deref separately
            pattern.costWithOptimization = 8.0;       // Combined operation
            pattern.savingsEstimate = 7.0;
            pattern.frequency = 1;

            detectedPatterns.push_back(pattern);
        }
    }
}

void InterTUPatternDetector::detectAliasChains() {
    // Pattern: Pointer field is part of an alias chain across TUs
    // This requires cross-module analysis

    for (const auto& [groupName, fields] : aliasGroups) {
        if (fields.size() > 2) {
            DetectedPattern pattern;
            pattern.type = PatternType::AliasChain;
            pattern.description = "Alias chain: " + std::to_string(fields.size()) + " fields";
            pattern.involvingFields = fields;
            pattern.costWithoutOptimization = fields.size() * 5.0;
            pattern.costWithOptimization = 2.0;  // Single cache location
            pattern.savingsEstimate = pattern.costWithoutOptimization - pattern.costWithOptimization;
            pattern.frequency = fields.size();

            detectedPatterns.push_back(pattern);
        }
    }
}

std::vector<DetectedPattern> InterTUPatternDetector::getDetectedPatterns() const {
    return detectedPatterns;
}

std::vector<DetectedPattern> InterTUPatternDetector::getPatternsByType(PatternType type) const {
    std::vector<DetectedPattern> filtered;

    for (const auto& pattern : detectedPatterns) {
        if (pattern.type == type) {
            filtered.push_back(pattern);
        }
    }

    return filtered;
}

std::vector<DetectedPattern> InterTUPatternDetector::getHighValuePatterns(double minSavings) const {
    std::vector<DetectedPattern> highValue;

    for (const auto& pattern : detectedPatterns) {
        if (pattern.savingsEstimate >= minSavings) {
            highValue.push_back(pattern);
        }
    }

    return highValue;
}

void InterTUPatternDetector::printReport(std::ostream& out) const {
    out << "\n=== Phase 96.5.2: Inter-TU Pattern Detection Report ===\n";
    out << "Total patterns detected: " << detectedPatterns.size() << "\n";
    out << "Total estimated savings: " << getTotalEstimatedSavings() << " bytes\n\n";

    // Group by type
    std::map<PatternType, std::vector<const DetectedPattern*>> byType;
    for (const auto& pattern : detectedPatterns) {
        byType[pattern.type].push_back(&pattern);
    }

    for (const auto& [type, patterns] : byType) {
        std::string typeName;
        switch (type) {
            case PatternType::SequentialFieldAccess: typeName = "Sequential Field Access"; break;
            case PatternType::LoopNestedFieldAccess: typeName = "Loop-Nested Access"; break;
            case PatternType::FieldReuse: typeName = "Field Reuse"; break;
            case PatternType::PointerDereference: typeName = "Pointer Dereference"; break;
            case PatternType::AliasChain: typeName = "Alias Chain"; break;
        }

        out << "\n--- " << typeName << " (" << patterns.size() << " patterns) ---\n";

        for (const auto* pattern : patterns) {
            out << "  " << pattern->description << "\n";
            out << "    Savings: " << std::fixed << std::setprecision(1)
                << pattern->savingsEstimate << " bytes\n";
        }
    }
}

double InterTUPatternDetector::getTotalEstimatedSavings() const {
    double total = 0.0;
    for (const auto& pattern : detectedPatterns) {
        total += pattern.savingsEstimate;
    }
    return total;
}

void InterTUPatternDetector::clear() {
    functionFieldAccesses.clear();
    fieldAccessSequences.clear();
    aliasGroups.clear();
    callGraph.clear();
    detectedPatterns.clear();
}

} // namespace phase96_5
