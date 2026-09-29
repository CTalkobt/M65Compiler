#include "GlobalFunctionDatabase.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

GlobalFunctionDatabase& GlobalFunctionDatabase::instance() {
    static GlobalFunctionDatabase db;
    return db;
}

void GlobalFunctionDatabase::addFunctionProfile(const GlobalFunctionProfile& profile) {
    profiles_[profile.name] = profile;
}

GlobalFunctionProfile* GlobalFunctionDatabase::getFunctionProfile(const std::string& name) {
    auto it = profiles_.find(name);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

const GlobalFunctionProfile* GlobalFunctionDatabase::getFunctionProfile(const std::string& name) const {
    auto it = profiles_.find(name);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<SpecializationInfo> GlobalFunctionDatabase::getSpecializationCandidates() const {
    std::vector<SpecializationInfo> candidates;

    for (const auto& [name, profile] : profiles_) {
        // Skip if no external callers (already dead code)
        if (!profile.hasExternalCaller) continue;

        // Analyze call patterns to find specialization opportunities
        std::unordered_map<std::string, std::vector<ArgumentPattern>> patternsByKey;

        for (const auto& callSite : profile.callSites) {
            // Group constant argument patterns
            std::string key;
            std::vector<ArgumentPattern> constants;

            for (const auto& arg : callSite.arguments) {
                if (arg.isConstant) {
                    if (!key.empty()) key += "_";
                    key += std::to_string(arg.constantValue);
                    constants.push_back(arg);
                }
            }

            if (!key.empty()) {
                patternsByKey[key].insert(
                    patternsByKey[key].end(),
                    constants.begin(),
                    constants.end()
                );
            }
        }

        // For each constant pattern that appears multiple times, consider specialization
        for (const auto& [key, patterns] : patternsByKey) {
            // Count how many call sites use this pattern
            int count = 0;
            for (const auto& callSite : profile.callSites) {
                bool matches = true;
                for (const auto& arg : callSite.arguments) {
                    auto found = std::find_if(patterns.begin(), patterns.end(),
                        [&arg](const ArgumentPattern& p) {
                            return p.paramIndex == arg.paramIndex && p.isConstant;
                        });
                    if (found == patterns.end()) {
                        matches = false;
                        break;
                    }
                }
                if (matches) count++;
            }

            // Only specialize if used in 2+ call sites
            if (count >= 2) {
                SpecializationInfo info;
                info.baseFunctionName = name;
                info.constantArgs = patterns;
                // Estimate savings: branch elimination + constant folding
                // Conservative estimate: 5-15 bytes per specialized constant
                info.estimatedSavings = patterns.size() * 7;
                info.roi = calculateROI(profile, patterns);

                if (info.roi > 1.0f) {  // Only include if ROI > 1.0
                    candidates.push_back(info);
                }
            }
        }
    }

    // Sort by ROI descending
    std::sort(candidates.begin(), candidates.end(),
        [](const SpecializationInfo& a, const SpecializationInfo& b) {
            return a.roi > b.roi;
        });

    return candidates;
}

std::vector<std::string> GlobalFunctionDatabase::getDeadFunctions() const {
    std::vector<std::string> deadFuncs;

    for (const auto& [name, profile] : profiles_) {
        if (profile.isDeadCode || !profile.hasExternalCaller) {
            deadFuncs.push_back(name);
        }
    }

    return deadFuncs;
}

std::vector<GlobalFunctionProfile*> GlobalFunctionDatabase::getExternalFunctions() {
    std::vector<GlobalFunctionProfile*> external;

    for (auto& [name, profile] : profiles_) {
        if (profile.hasExternalCaller) {
            external.push_back(&profile);
        }
    }

    return external;
}

float GlobalFunctionDatabase::calculateROI(const GlobalFunctionProfile& /* profile */,
                                          const std::vector<ArgumentPattern>& constants) const {
    // ROI = savings / (overhead of specialization)
    // Savings: branch elimination + condition checking (~5 bytes per constant arg)
    // Overhead: extra function definition (~10-20 bytes)

    int savings = constants.size() * 5;
    int overhead = 15;  // Cost of creating specialized version

    if (overhead == 0) return 1.0f;

    float roi = static_cast<float>(savings) / static_cast<float>(overhead);
    return roi;
}

void GlobalFunctionDatabase::printStatistics() const {
    std::cerr << "\n=== Global Function Database Statistics ===\n";
    std::cerr << "Total functions: " << profiles_.size() << "\n";

    int totalCallSites = 0;
    int externalFunctions = 0;
    int deadFunctions = 0;
    int leafFunctions = 0;

    for (const auto& [name, profile] : profiles_) {
        totalCallSites += profile.callSites.size();
        if (profile.hasExternalCaller) externalFunctions++;
        if (profile.isDeadCode) deadFunctions++;
        if (profile.isLeaf) leafFunctions++;
    }

    std::cerr << "Total call sites: " << totalCallSites << "\n";
    std::cerr << "External functions: " << externalFunctions << "\n";
    std::cerr << "Dead functions: " << deadFunctions << "\n";
    std::cerr << "Leaf functions: " << leafFunctions << "\n";

    auto candidates = getSpecializationCandidates();
    std::cerr << "Specialization candidates: " << candidates.size() << "\n";

    if (!candidates.empty()) {
        std::cerr << "\nTop specialization opportunities:\n";
        for (size_t i = 0; i < std::min(size_t(5), candidates.size()); ++i) {
            std::cerr << "  " << candidates[i].baseFunctionName
                     << ": " << candidates[i].estimatedSavings << " bytes"
                     << " (ROI: " << candidates[i].roi << ")\n";
        }
    }

    std::cerr << "\n";
}

void GlobalFunctionDatabase::clear() {
    profiles_.clear();
}
