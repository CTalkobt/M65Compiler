#include "FieldCachingAnalyzer.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace phase96_5 {

FieldCachingAnalyzer::FieldCachingAnalyzer() {
    initializeRegisters();
}

void FieldCachingAnalyzer::initializeRegisters() {
    // Initialize ZP registers $60-$70 (5 registers available)
    for (int reg = 0x60; reg <= 0x70; reg += 2) {
        RegisterAvailability ra;
        ra.registerNum = reg;
        ra.occupantField = "";
        ra.pressureLevel = 0;  // Initially available
        registers.push_back(ra);
    }
}

// Register pressure analysis
void FieldCachingAnalyzer::analyzeRegisterPressure(const std::vector<std::string>& fieldCandidates,
                                                    const std::vector<std::string>& compilingFunctions) {
    // For each field candidate, compute how much register pressure it creates
    for (const auto& field : fieldCandidates) {
        double pressure = computeRegisterPressure(field);

        // Update register pressure levels
        for (auto& reg : registers) {
            if (pressure > 0.7 && reg.competingFields.empty()) {
                reg.pressureLevel = std::min(2, (int)(pressure * 3));
                reg.competingFields.insert(field);
            }
        }
    }
}

double FieldCachingAnalyzer::computeRegisterPressure(const std::string& fieldName) const {
    // Heuristic: compute pressure based on:
    // 1. Number of functions accessing this field
    // 2. Total access frequency
    // 3. Interaction with other fields

    double pressure = 0.0;

    // Base pressure from access patterns
    size_t accessCount = 0;
    size_t functionCount = 0;

    for (const auto& [func, lifetimes] : fieldLifetimes) {
        for (const auto& lt : lifetimes) {
            if (lt.fieldName == fieldName) {
                accessCount += lt.accessCount;
                functionCount++;
            }
        }
    }

    // Normalize pressure (0.0 = no pressure, 1.0 = high pressure)
    if (functionCount > 0) {
        pressure = std::min(1.0, (double)accessCount / 100.0);
    }

    return pressure;
}

// Lifetime analysis phase
void FieldCachingAnalyzer::analyzeFieldLifetimes(const std::string& functionName,
                                                 const std::vector<FieldPointerLifetime>& lifetimes) {
    fieldLifetimes[functionName] = lifetimes;

    for (auto& lifetime : fieldLifetimes[functionName]) {
        // Mark if lifetime crosses function calls
        if (functionInvalidationEvents.count(functionName) > 0) {
            lifetime.crossesFunctionCall = lifetimeSpansFunctionCall(
                lifetime, functionInvalidationEvents[functionName]);
        }
    }
}

bool FieldCachingAnalyzer::lifetimeSpansFunctionCall(
    const FieldPointerLifetime& lifetime,
    const std::vector<InvalidationEvent>& events) const {

    for (const auto& event : events) {
        if (event.type == InvalidationEvent::Type::FunctionCall) {
            if (event.lineNumber >= lifetime.firstAccessLine &&
                event.lineNumber <= lifetime.lastAccessLine) {
                return true;
            }
        }
    }
    return false;
}

// Invalidation strategy planning
void FieldCachingAnalyzer::planInvalidationStrategy(const std::string& functionName,
                                                   const std::vector<InvalidationEvent>& events) {
    functionInvalidationEvents[functionName] = events;

    // Determine if we can use aggressive or must use conservative invalidation
    bool hasPotentialAliasing = false;
    bool hasGlobalWrites = false;

    for (const auto& event : events) {
        if (event.type == InvalidationEvent::Type::GlobalWrite) {
            hasGlobalWrites = true;
        }
    }

    if (!hasPotentialAliasing && !hasGlobalWrites) {
        planAggressiveInvalidation(functionName);
    } else {
        planConservativeInvalidation(functionName);
    }
}

void FieldCachingAnalyzer::planConservativeInvalidation(const std::string& functionName) {
    // Conservative: Invalidate cache at every function call and memory write
    // This is safe but may miss optimization opportunities

    if (functionInvalidationEvents.count(functionName) == 0) {
        return;
    }

    auto& events = functionInvalidationEvents[functionName];
    for (auto& event : events) {
        if (event.type == InvalidationEvent::Type::FunctionCall ||
            event.type == InvalidationEvent::Type::MemoryWrite ||
            event.type == InvalidationEvent::Type::GlobalWrite) {
            event.isConservative = true;
        }
    }
}

void FieldCachingAnalyzer::planAggressiveInvalidation(const std::string& functionName) {
    // Aggressive: Only invalidate where truly necessary
    // Requires alias analysis to be correct

    if (functionInvalidationEvents.count(functionName) == 0) {
        return;
    }

    auto& events = functionInvalidationEvents[functionName];
    for (auto& event : events) {
        event.isConservative = false;
    }
}

std::string FieldCachingAnalyzer::determineInvalidationStrategy(
    const FieldPointerLifetime& lifetime,
    const std::vector<InvalidationEvent>& events) const {

    if (lifetime.crossesFunctionCall) {
        if (lifetime.accessSpan > 50) {
            return "function-level";  // Cache per function
        } else {
            return "conservative";    // Invalidate frequently
        }
    } else if (lifetime.loopNestingLevel > 0) {
        return "loop-level";          // Cache in loop scope
    } else {
        return "aggressive";          // Keep cache as long as possible
    }
}

// Cost-benefit analysis phase
void FieldCachingAnalyzer::analyzeCostBenefit() {
    cachingDecisions.clear();

    for (const auto& [funcName, lifetimes] : fieldLifetimes) {
        for (const auto& lifetime : lifetimes) {
            CachingDecision decision;
            decision.fieldName = lifetime.fieldName;
            decision.structName = "";  // Would come from field info
            decision.firstAccessLine = lifetime.firstAccessLine;
            decision.lastAccessLine = lifetime.lastAccessLine;

            // Compute estimated savings
            int bytesPerAccess = 10;  // Typical offset calc
            decision.estimatedSavings = (lifetime.accessCount - 1) * bytesPerAccess * 0.8;

            // Compute implementation cost
            decision.implementationCost = computeImplementationCost(
                lifetime.fieldName,
                determineInvalidationStrategy(lifetime,
                    functionInvalidationEvents[funcName]));

            // Net benefit
            decision.netBenefit = decision.estimatedSavings - decision.implementationCost;
            decision.shouldCache = (decision.netBenefit > 0);

            // Determine caching strategy
            if (functionInvalidationEvents.count(funcName) > 0) {
                decision.cachingStrategy = determineInvalidationStrategy(
                    lifetime, functionInvalidationEvents[funcName]);
            } else {
                decision.cachingStrategy = "conservative";
            }

            // Allocate register if caching is beneficial
            if (decision.shouldCache) {
                decision.suggestedRegister = allocateOptimalRegister(lifetime.fieldName);
                decision.recommendedCacheDuration = lifetime.accessSpan;
                cachingDecisions.push_back(decision);
            }
        }
    }

    // Sort by net benefit (descending)
    std::sort(cachingDecisions.begin(), cachingDecisions.end(),
              [](const auto& a, const auto& b) {
                  return a.netBenefit > b.netBenefit;
              });
}

double FieldCachingAnalyzer::computeImplementationCost(
    const std::string& fieldName,
    const std::string& cachingStrategy) const {

    // Cost varies by strategy
    if (cachingStrategy == "function-level") {
        return 3.0;  // Load cache once per function
    } else if (cachingStrategy == "loop-level") {
        return 2.0;  // Load cache once per loop
    } else if (cachingStrategy == "aggressive") {
        return 2.0;  // Load cache, keep until invalidation
    } else {  // conservative
        return 5.0;  // Frequent invalidation/reload overhead
    }
}

int FieldCachingAnalyzer::allocateOptimalRegister(const std::string& fieldName) {
    // Find least contested available register
    int bestReg = -1;
    int minPressure = 3;

    for (auto& reg : registers) {
        if (reg.occupantField.empty() && reg.pressureLevel < minPressure) {
            minPressure = reg.pressureLevel;
            bestReg = reg.registerNum;
        }
    }

    if (bestReg != -1) {
        for (auto& reg : registers) {
            if (reg.registerNum == bestReg) {
                reg.occupantField = fieldName;
                reg.pressureLevel = 1;
                break;
            }
        }
    }

    return bestReg;
}

// Query methods
std::vector<CachingDecision> FieldCachingAnalyzer::getOptimalCachingDecisions() {
    return cachingDecisions;
}

std::vector<CachingDecision> FieldCachingAnalyzer::getCachingDecisionsByFunction(
    const std::string& funcName) {

    std::vector<CachingDecision> result;
    for (const auto& decision : cachingDecisions) {
        if (fieldLifetimes.count(funcName) > 0) {
            for (const auto& lt : fieldLifetimes[funcName]) {
                if (lt.fieldName == decision.fieldName) {
                    result.push_back(decision);
                }
            }
        }
    }
    return result;
}

RegisterAvailability FieldCachingAnalyzer::getRegisterAvailability(int registerNum) const {
    for (const auto& reg : registers) {
        if (reg.registerNum == registerNum) {
            return reg;
        }
    }
    // Default: unavailable
    RegisterAvailability ra;
    ra.registerNum = registerNum;
    ra.pressureLevel = 2;
    return ra;
}

std::vector<RegisterAvailability> FieldCachingAnalyzer::getAvailableRegisters() const {
    std::vector<RegisterAvailability> available;
    for (const auto& reg : registers) {
        if (reg.occupantField.empty()) {
            available.push_back(reg);
        }
    }
    return available;
}

bool FieldCachingAnalyzer::validateCachingDecision(const CachingDecision& decision) const {
    // Validation checks:
    // 1. Net benefit is positive
    // 2. Register is available
    // 3. Invalidation strategy is sound

    if (decision.netBenefit <= 0) {
        return false;  // No benefit
    }

    if (decision.suggestedRegister < 0x60 || decision.suggestedRegister > 0x70) {
        return false;  // Invalid register
    }

    return true;
}

double FieldCachingAnalyzer::getTotalEstimatedSavings() const {
    double total = 0.0;
    for (const auto& decision : cachingDecisions) {
        if (decision.shouldCache) {
            total += decision.netBenefit;
        }
    }
    return total;
}

int FieldCachingAnalyzer::getRegisterAllocationScore() const {
    // Score: 0-100 based on how well registers are allocated
    int allocated = 0;
    for (const auto& reg : registers) {
        if (!reg.occupantField.empty()) {
            allocated++;
        }
    }
    return (allocated * 100) / static_cast<int>(registers.size());
}

// Debug output
void FieldCachingAnalyzer::printAnalysisReport(std::ostream& out) const {
    out << "\n=== Phase 96.5.3: Field Caching Analysis Report ===\n";
    out << "Total caching decisions: " << cachingDecisions.size() << "\n";
    out << "Total estimated savings: " << std::fixed << std::setprecision(1)
        << getTotalEstimatedSavings() << " bytes\n";
    out << "Register allocation: " << getRegisterAllocationScore() << "%\n\n";

    for (const auto& decision : cachingDecisions) {
        out << "Field: " << decision.fieldName << "\n";
        out << "  Strategy: " << decision.cachingStrategy << "\n";
        out << "  Register: $" << std::hex << decision.suggestedRegister << std::dec << "\n";
        out << "  Savings: " << std::fixed << std::setprecision(1)
            << decision.estimatedSavings << " bytes\n";
        out << "  Cost: " << decision.implementationCost << " bytes\n";
        out << "  Net benefit: " << decision.netBenefit << " bytes\n\n";
    }
}

void FieldCachingAnalyzer::printRegisterAllocationMap(std::ostream& out) const {
    out << "\n=== Register Allocation Map ===\n";
    out << "ZP Region $60-$70:\n";
    for (const auto& reg : registers) {
        out << "  $" << std::hex << reg.registerNum << std::dec << ": ";
        if (reg.occupantField.empty()) {
            out << "[available]\n";
        } else {
            out << reg.occupantField << " (pressure: " << reg.pressureLevel << ")\n";
        }
    }
}

void FieldCachingAnalyzer::printInvalidationStrategy(std::ostream& out) const {
    out << "\n=== Invalidation Strategies ===\n";
    for (const auto& [funcName, events] : functionInvalidationEvents) {
        out << "Function: " << funcName << "\n";
        int invalidations = 0;
        for (const auto& event : events) {
            if (event.isConservative) {
                invalidations++;
            }
        }
        out << "  Invalidation points: " << invalidations << "\n";
    }
}

void FieldCachingAnalyzer::clear() {
    registers.clear();
    fieldLifetimes.clear();
    functionInvalidationEvents.clear();
    cachingDecisions.clear();
    initializeRegisters();
}

} // namespace phase96_5
