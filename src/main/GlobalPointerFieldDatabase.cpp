#include "GlobalPointerFieldDatabase.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace phase96_5 {

// Singleton instance
GlobalPointerFieldDatabase& GlobalPointerFieldDatabase::instance() {
    static GlobalPointerFieldDatabase db;
    return db;
}

// Recording phase: Record field accesses
void GlobalPointerFieldDatabase::recordFieldAccess(const std::string& functionName,
                                                   const std::string& structName,
                                                   const std::string& fieldName,
                                                   size_t fieldOffset,
                                                   AccessType accessType) {
    auto profile = getOrCreateFieldProfile(structName, fieldName, fieldOffset);

    // Track accessing functions
    profile->accessingFunctions.insert(functionName);

    // Update access pattern
    auto& pattern = profile->patterns[functionName];
    switch (accessType) {
        case AccessType::Read:
            pattern.readCount++;
            break;
        case AccessType::Write:
            pattern.writeCount++;
            break;
        case AccessType::Address:
            pattern.readCount++;  // Address-of counts as a read for caching purposes
            break;
    }

    profile->totalAccesses++;
}

void GlobalPointerFieldDatabase::recordFieldInLoop(const std::string& functionName,
                                                   const std::string& structName,
                                                   const std::string& fieldName,
                                                   int loopDepth) {
    auto profile = getOrCreateFieldProfile(structName, fieldName, 0);
    profile->accessingFunctions.insert(functionName);

    auto& pattern = profile->patterns[functionName];
    pattern.isNested = (loopDepth > 0);
    pattern.loopDepth = std::max(pattern.loopDepth, loopDepth);

    // Loop-nested accesses are high-value caching candidates
    if (loopDepth > 0) {
        pattern.cacheOpportunities += (loopDepth * 10);  // Heuristic: loop depth * 10
    }
}

void GlobalPointerFieldDatabase::recordSequentialAccess(const std::string& functionName,
                                                        const std::string& structName,
                                                        const std::string& field1,
                                                        const std::string& field2) {
    auto profile = getOrCreateFieldProfile(structName, field1, 0);
    profile->accessingFunctions.insert(functionName);

    auto& pattern = profile->patterns[functionName];
    pattern.isSequential = true;
    pattern.commonSuccessor = 1;  // field2 follows field1

    // Record aliasing relationship
    profile->aliasingFields.insert(field2);
}

// Helper to get or create a field profile
std::shared_ptr<PointerFieldProfile> GlobalPointerFieldDatabase::getOrCreateFieldProfile(
    const std::string& structName,
    const std::string& fieldName,
    size_t fieldOffset) {

    std::string key = structName + "::" + fieldName;

    auto it = fieldProfiles.find(key);
    if (it != fieldProfiles.end()) {
        return it->second;
    }

    // Create new profile
    auto profile = std::make_shared<PointerFieldProfile>();
    profile->fieldName = fieldName;
    profile->structName = structName;
    profile->fieldOffset = fieldOffset;
    profile->fieldSize = 2;  // All pointer fields are 2 bytes

    fieldProfiles[key] = profile;

    // Add to struct's field set
    if (structFields.find(structName) == structFields.end()) {
        structFields[structName] = std::make_shared<StructPointerFieldSet>();
        structFields[structName]->structName = structName;
    }
    structFields[structName]->fields.push_back(profile);

    return profile;
}

// Analysis phase: Analyze patterns and compute optimization decisions
void GlobalPointerFieldDatabase::analyzePatterns() {
    // Step 1: Compute cache candidates
    computeCacheCandidates();

    // Step 2: Compute aliasing patterns
    computeAliasingPatterns();

    // Step 3: Assign cache registers
    for (auto& [key, profile] : fieldProfiles) {
        if (profile->isCacheCandidate) {
            profile->estimatedSavings = computeSavingsEstimate(*profile);

            // Simple heuristic: assign ZP registers based on fields that benefit most
            if (profile->estimatedSavings > 10.0) {
                // High-value fields get preferred cache registers
                static int nextCacheReg = 0x60;  // Start at $60 (safe ZP area)
                if (nextCacheReg < 0x70) {
                    profile->suggestedCacheRegister = nextCacheReg;
                    nextCacheReg += 2;
                }
            }
        }
    }

    // Debug output
    if (false) {  // Set to true for debug output
        fprintf(stderr, "[Phase 96.5] Analyzed %zu pointer fields across structs\n",
                fieldProfiles.size());
        for (const auto& [key, profile] : fieldProfiles) {
            if (profile->isCacheCandidate) {
                fprintf(stderr, "  Candidate: %s::%s (savings: %.1f bytes)\n",
                        profile->structName.c_str(), profile->fieldName.c_str(),
                        profile->estimatedSavings);
            }
        }
    }
}

// Identify fields that are good caching candidates
void GlobalPointerFieldDatabase::computeCacheCandidates() {
    for (auto& [key, profile] : fieldProfiles) {
        // Criteria for caching:
        // 1. Accessed in multiple functions (cross-module)
        // 2. High access frequency (>3 accesses)
        // 3. Accessed in loops (nested)
        // 4. Sequential pattern with other fields

        size_t accessCount = 0;
        for (const auto& [func, pattern] : profile->patterns) {
            accessCount += pattern.readCount + pattern.writeCount;
        }

        bool isMultiFunction = profile->accessingFunctions.size() > 1;
        bool isFrequentlyAccessed = accessCount > 3;
        bool isLoopNested = std::any_of(profile->patterns.begin(), profile->patterns.end(),
                                        [](const auto& p) { return p.second.isNested; });

        profile->isCacheCandidate = (isMultiFunction || isFrequentlyAccessed || isLoopNested);
    }
}

// Identify field aliasing patterns (fields accessed together)
void GlobalPointerFieldDatabase::computeAliasingPatterns() {
    // For each struct, find fields that are frequently accessed together
    for (auto& [structName, fieldSet] : structFields) {
        for (size_t i = 0; i < fieldSet->fields.size(); i++) {
            for (size_t j = i + 1; j < fieldSet->fields.size(); j++) {
                auto& field1 = fieldSet->fields[i];
                auto& field2 = fieldSet->fields[j];

                // Check if they're accessed together in any function
                for (const auto& func : field1->accessingFunctions) {
                    if (field2->accessingFunctions.count(func) > 0) {
                        // Both accessed in same function - they alias
                        field1->aliasingFields.insert(field2->fieldName);
                        field2->aliasingFields.insert(field1->fieldName);
                    }
                }
            }
        }
    }
}

// Estimate code size savings from caching a field
double GlobalPointerFieldDatabase::computeSavingsEstimate(const PointerFieldProfile& profile) const {
    double savings = 0.0;

    for (const auto& [func, pattern] : profile.patterns) {
        // Base savings: if cached, avoid offset calculation on each access
        // Typical offset calculation: 10-15 bytes, cached: 2 bytes
        // But benefit only if multiple accesses

        size_t accessCount = pattern.readCount + pattern.writeCount;
        if (accessCount > 1) {
            // First access: load cache (2 bytes)
            // Subsequent accesses: reuse cache (0 bytes vs 10+ bytes)
            double benefit = (accessCount - 1) * 10.0;  // Heuristic: 10 bytes per access saved
            double cost = 2.0;  // Cost of caching the pointer

            savings += std::max(0.0, benefit - cost);
        }

        // Bonus for loop-nested accesses (higher cache hit rate)
        if (pattern.isNested && pattern.loopDepth > 0) {
            savings *= (1.0 + pattern.loopDepth * 0.5);  // 50% bonus per loop level
        }
    }

    return savings;
}

// Query phase: Get optimization candidates
std::vector<std::shared_ptr<PointerFieldProfile>> GlobalPointerFieldDatabase::getOptimizationCandidates() const {
    std::vector<std::shared_ptr<PointerFieldProfile>> candidates;

    for (const auto& [key, profile] : fieldProfiles) {
        if (profile->isCacheCandidate && profile->estimatedSavings > 0.0) {
            candidates.push_back(profile);
        }
    }

    // Sort by estimated savings (descending)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a->estimatedSavings > b->estimatedSavings;
              });

    return candidates;
}

std::vector<std::shared_ptr<PointerFieldProfile>> GlobalPointerFieldDatabase::getFieldsByStruct(
    const std::string& structName) const {

    auto it = structFields.find(structName);
    if (it != structFields.end()) {
        return it->second->fields;
    }
    return {};
}

std::shared_ptr<PointerFieldProfile> GlobalPointerFieldDatabase::getFieldProfile(
    const std::string& structName,
    const std::string& fieldName) const {

    std::string key = structName + "::" + fieldName;
    auto it = fieldProfiles.find(key);
    if (it != fieldProfiles.end()) {
        return it->second;
    }
    return nullptr;
}

// Cross-module analysis
std::vector<std::string> GlobalPointerFieldDatabase::getHotStructs() const {
    std::vector<std::string> hotStructs;

    for (const auto& [structName, fieldSet] : structFields) {
        std::set<std::string> uniqueModules;
        for (const auto& field : fieldSet->fields) {
            for (const auto& func : field->accessingFunctions) {
                // Extract module name from function name (if available)
                // This is a placeholder - actual implementation would need module tracking
                uniqueModules.insert("unknown");
            }
        }

        if (uniqueModules.size() > 3) {  // Accessed in >3 modules
            hotStructs.push_back(structName);
        }
    }

    return hotStructs;
}

std::vector<std::string> GlobalPointerFieldDatabase::getGlobalAliasChains() const {
    std::vector<std::string> chains;

    for (const auto& [key, profile] : fieldProfiles) {
        if (!profile->aliasingFields.empty()) {
            std::string chain = profile->structName + "::" + profile->fieldName;
            for (const auto& alias : profile->aliasingFields) {
                chain += " -> " + alias;
            }
            chains.push_back(chain);
        }
    }

    return chains;
}

// Clear database (for testing or multiple compilations)
void GlobalPointerFieldDatabase::clear() {
    fieldProfiles.clear();
    structFields.clear();
    fieldSequences.clear();
}

} // namespace phase96_5
