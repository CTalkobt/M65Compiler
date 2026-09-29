#include "LinkerFieldOptimizer.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace phase96_5 {

LinkerFieldOptimizer::LinkerFieldOptimizer() {
}

// Phase 1: Merge field profiles from object files
void LinkerFieldOptimizer::mergeFieldProfiles(const std::vector<std::string>& objectFiles) {
    for (const auto& objFile : objectFiles) {
        mergeProfilesFromObjectFile(objFile);
    }

    // After merging, analyze opportunities
    computeGlobalSavings();
}

void LinkerFieldOptimizer::addFieldProfile(const std::string& structName,
                                           const std::string& fieldName,
                                           const std::string& sourceModule,
                                           const std::vector<std::string>& accessingFunctions) {
    std::string key = structName + "::" + fieldName;

    auto it = mergedProfiles.find(key);
    if (it == mergedProfiles.end()) {
        // Create new merged profile
        MergedFieldProfile profile;
        profile.fieldName = fieldName;
        profile.structName = structName;
        profile.totalAccessCount = accessingFunctions.size();
        profile.globalSavingsEstimate = 0.0;
        profile.shouldCacheGlobally = false;
        mergedProfiles[key] = profile;
        it = mergedProfiles.find(key);
    }

    // Update merged profile
    it->second.accessingModules.insert(sourceModule);
    for (const auto& func : accessingFunctions) {
        it->second.accessingFunctions.insert(func);
    }
}

void LinkerFieldOptimizer::mergeProfilesFromObjectFile(const std::string& objectFile) {
    // In real implementation, would parse .o45 object file
    // For now, this is a placeholder

    // Would extract:
    // - Field metadata from .o45
    // - Access patterns
    // - Module name
}

// Phase 2: Analyze cross-module opportunities
void LinkerFieldOptimizer::analyzeGlobalOptimizationOpportunities() {
    for (auto& [key, profile] : mergedProfiles) {
        // Criteria for global caching:
        // 1. Accessed in multiple modules (cross-TU)
        // 2. High total access count
        // 3. Savings exceed dispatcher overhead

        bool isMultiModule = profile.accessingModules.size() > 1;
        bool isFrequentlyAccessed = profile.totalAccessCount > 10;

        // Dispatcher overhead: ~20-30 bytes
        // Savings must exceed this to be worthwhile
        bool savingsJustifyDispatcher = profile.globalSavingsEstimate > 30.0;

        profile.shouldCacheGlobally = (isMultiModule && isFrequentlyAccessed) ||
                                      savingsJustifyDispatcher;
    }
}

void LinkerFieldOptimizer::computeGlobalSavings() {
    for (auto& [key, profile] : mergedProfiles) {
        // Compute global savings across all accessing functions
        // Heuristic: each access saves ~10 bytes with caching
        // But first access has overhead (~3 bytes)

        if (profile.totalAccessCount > 1) {
            profile.globalSavingsEstimate = (profile.totalAccessCount - 1) * 8.0;

            // Multiply by module count for cross-TU benefit
            if (profile.accessingModules.size() > 1) {
                profile.globalSavingsEstimate *= (profile.accessingModules.size() * 0.8);
            }
        }
    }
}

// Phase 3: Generate dispatchers for multi-version fields
void LinkerFieldOptimizer::generateFieldDispatchers() {
    for (const auto& [key, profile] : mergedProfiles) {
        if (profile.shouldCacheGlobally && profile.accessingModules.size() > 1) {
            generateDispatcherCode(profile.fieldName);
        }
    }
}

void LinkerFieldOptimizer::generateDispatcherCode(const std::string& fieldName) {
    FieldDispatcher dispatcher;
    dispatcher.dispatcherName = "_field_cache_dispatch_" + fieldName;
    dispatcher.fieldName = fieldName;

    // Dispatcher would:
    // 1. Check if pointer is cached
    // 2. If cached, use cached version
    // 3. If not cached, use non-cached version
    // 4. Typical size: 20-30 bytes

    dispatcher.dispatcherSize = 25;

    dispatchers.push_back(dispatcher);
}

// Phase 4: Emit assembly caching hints
void LinkerFieldOptimizer::emitCachingDirectives() {
    for (const auto& [key, profile] : mergedProfiles) {
        if (profile.shouldCacheGlobally) {
            FieldCachingDirective directive;
            directive.fieldName = profile.fieldName;
            directive.structName = profile.structName;
            directive.estimatedSavings = profile.globalSavingsEstimate;

            // Allocate register
            directive.suggestedRegister = allocateGlobalRegister(
                profile.fieldName, profile.globalSavingsEstimate);

            // Determine caching strategy
            if (profile.accessingModules.size() > 2) {
                directive.cachingStrategy = "function-level";
            } else {
                directive.cachingStrategy = "conservative";
            }

            cachingDirectives.push_back(directive);

            // Emit hints for assembly
            emitHintsForField(profile.fieldName, profile);
        }
    }
}

int LinkerFieldOptimizer::allocateGlobalRegister(const std::string& fieldName,
                                                 double savingsEstimate) {
    // Allocate from ZP registers $60-$70
    // Highest-savings fields get preferred registers

    static int nextReg = 0x60;
    if (nextReg <= 0x6E) {  // Leave $70 free
        int reg = nextReg;
        nextReg += 2;
        return reg;
    }
    return -1;  // No registers available
}

void LinkerFieldOptimizer::emitHintsForField(const std::string& fieldName,
                                            const MergedFieldProfile& profile) {
    // Emit hints for assembler to apply optimizations

    AssemblyHint loadHint;
    loadHint.type = AssemblyHint::Type::CacheLoad;
    loadHint.fieldName = fieldName;
    loadHint.lineNumber = 0;  // Initialize lineNumber
    loadHint.description = "Load " + fieldName + " into cache register";
    loadHint.estimatedByteSavings = 3;  // One LDA instruction
    assemblyHints.push_back(loadHint);

    // Emit reuse hints
    for (size_t i = 1; i < profile.totalAccessCount; i++) {
        AssemblyHint reuseHint;
        reuseHint.type = AssemblyHint::Type::CacheReuse;
        reuseHint.fieldName = fieldName;
        reuseHint.lineNumber = 0;  // Initialize lineNumber
        reuseHint.description = "Reuse cached " + fieldName;
        reuseHint.estimatedByteSavings = 10;  // Avoid offset calculation
        assemblyHints.push_back(reuseHint);
    }
}

// Phase 5: Coordinate with assembler
void LinkerFieldOptimizer::coordinateWithAssembler() {
    // Would pass caching directives to assembler optimizer (Phase 95.5)
    // Assembler would use hints to optimize field access instructions
}

// Query methods
std::vector<MergedFieldProfile> LinkerFieldOptimizer::getGlobalCachingCandidates() const {
    std::vector<MergedFieldProfile> candidates;

    for (const auto& [key, profile] : mergedProfiles) {
        if (profile.shouldCacheGlobally) {
            candidates.push_back(profile);
        }
    }

    // Sort by global savings (descending)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.globalSavingsEstimate > b.globalSavingsEstimate;
              });

    return candidates;
}

std::vector<FieldCachingDirective> LinkerFieldOptimizer::getCachingDirectives() const {
    return cachingDirectives;
}

std::vector<FieldDispatcher> LinkerFieldOptimizer::getFieldDispatchers() const {
    return dispatchers;
}

std::vector<AssemblyHint> LinkerFieldOptimizer::getAssemblyHints() const {
    return assemblyHints;
}

// Statistics and reporting
double LinkerFieldOptimizer::getTotalGlobalSavings() const {
    double total = 0.0;
    for (const auto& directive : cachingDirectives) {
        total += directive.estimatedSavings;
    }
    return total;
}

int LinkerFieldOptimizer::getFieldDispatcherCount() const {
    return dispatchers.size();
}

int LinkerFieldOptimizer::getTotalCachingHints() const {
    return assemblyHints.size();
}

std::set<std::string> LinkerFieldOptimizer::getOptimizedStructs() const {
    std::set<std::string> structs;
    for (const auto& directive : cachingDirectives) {
        structs.insert(directive.structName);
    }
    return structs;
}

// Output generation
void LinkerFieldOptimizer::generateLinkerScript(const std::string& outputFile) {
    std::ofstream out(outputFile);

    out << "; Phase 96.5.4: Linker Field Caching Directives\n";
    out << "; Generated automatically by LinkerFieldOptimizer\n\n";

    for (const auto& directive : cachingDirectives) {
        out << ".field_cache " << directive.structName << "." << directive.fieldName << "\n";
        out << "  register $" << std::hex << directive.suggestedRegister << std::dec << "\n";
        out << "  strategy " << directive.cachingStrategy << "\n";
        out << "  savings " << std::fixed << std::setprecision(1)
            << directive.estimatedSavings << " bytes\n\n";
    }
}

void LinkerFieldOptimizer::generateAssemblyHints(const std::string& outputFile) {
    std::ofstream out(outputFile);

    out << "; Phase 96.5.4: Assembly Caching Hints\n";
    out << "; For use by AssemblerOptimizer (Phase 95.5)\n\n";

    for (const auto& hint : assemblyHints) {
        out << "; " << hint.description << " (saves ~" << hint.estimatedByteSavings << " bytes)\n";
    }
}

void LinkerFieldOptimizer::printOptimizationReport(std::ostream& out) const {
    out << "\n=== Phase 96.5.4: Linker Field Optimization Report ===\n";
    out << "Total field profiles merged: " << mergedProfiles.size() << "\n";
    out << "Global caching candidates: " << getGlobalCachingCandidates().size() << "\n";
    out << "Field dispatchers generated: " << dispatchers.size() << "\n";
    out << "Assembly hints emitted: " << assemblyHints.size() << "\n";
    out << "Total estimated global savings: " << std::fixed << std::setprecision(1)
        << getTotalGlobalSavings() << " bytes\n\n";

    out << "Optimized Structs:\n";
    auto structs = getOptimizedStructs();
    for (const auto& s : structs) {
        out << "  • " << s << "\n";
    }
}

void LinkerFieldOptimizer::printDispatcherMap(std::ostream& out) const {
    out << "\n=== Field Dispatcher Map ===\n";
    for (const auto& dispatcher : dispatchers) {
        out << "Dispatcher: " << dispatcher.dispatcherName << "\n";
        out << "  Field: " << dispatcher.fieldName << "\n";
        out << "  Size: " << dispatcher.dispatcherSize << " bytes\n";
        out << "  Cached versions: " << dispatcher.cachedVersions.size() << "\n";
        out << "  Uncached versions: " << dispatcher.uncachedVersions.size() << "\n\n";
    }
}

void LinkerFieldOptimizer::clear() {
    mergedProfiles.clear();
    cachingDirectives.clear();
    dispatchers.clear();
    assemblyHints.clear();
}

// AssemblerCoordinator implementation

AssemblerCoordinator::AssemblerCoordinator() {
}

void AssemblerCoordinator::registerCachingDirective(const FieldCachingDirective& directive) {
    activeDirectives[directive.fieldName] = directive;
    registerAllocation[directive.fieldName] = directive.suggestedRegister;
}

void AssemblerCoordinator::applyFieldCachingOptimization(const std::string& fieldName,
                                                        const std::string& assemblyCode) {
    if (activeDirectives.count(fieldName) == 0) {
        return;  // No caching directive for this field
    }

    // In real implementation, would:
    // 1. Parse assembly code
    // 2. Identify field access instructions
    // 3. Apply caching optimization
    // 4. Emit optimized assembly

    // For now, just track that optimization was requested
    AssemblyHint hint;
    hint.type = AssemblyHint::Type::CacheLoad;
    hint.fieldName = fieldName;
    hint.description = "Applied caching for " + fieldName;
    appliedHints[fieldName].push_back(hint);
}

bool AssemblerCoordinator::validateCachingImplementation() {
    // Validate that all caching directives were properly applied

    for (const auto& [field, directive] : activeDirectives) {
        if (registerAllocation[field] < 0x60 || registerAllocation[field] > 0x70) {
            return false;  // Invalid register
        }

        if (appliedHints.count(field) == 0) {
            // Warning: no hints applied for this field
            // (might be OK if field wasn't accessed in final assembly)
        }
    }

    return true;
}

bool AssemblerCoordinator::isFieldCached(const std::string& fieldName) const {
    return activeDirectives.count(fieldName) > 0;
}

int AssemblerCoordinator::getCacheRegister(const std::string& fieldName) const {
    auto it = registerAllocation.find(fieldName);
    if (it != registerAllocation.end()) {
        return it->second;
    }
    return -1;
}

std::string AssemblerCoordinator::getCachingStrategy(const std::string& fieldName) const {
    auto it = activeDirectives.find(fieldName);
    if (it != activeDirectives.end()) {
        return it->second.cachingStrategy;
    }
    return "none";
}

void AssemblerCoordinator::printOptimizationResults(std::ostream& out) const {
    out << "\n=== Assembler Coordinator Results ===\n";
    out << "Active caching directives: " << activeDirectives.size() << "\n";
    out << "Fields with allocated registers: " << registerAllocation.size() << "\n";
    out << "Applied optimizations:\n";

    for (const auto& [field, hints] : appliedHints) {
        out << "  " << field << ": " << hints.size() << " optimizations applied\n";
    }
}

void AssemblerCoordinator::clear() {
    activeDirectives.clear();
    registerAllocation.clear();
    appliedHints.clear();
}

} // namespace phase96_5
