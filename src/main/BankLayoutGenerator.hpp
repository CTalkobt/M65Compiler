// Phase 99.3: Linker Integration for Cross-Module Bank Optimization
// Applies optimized bank layout to linker output

#pragma once

#include "CrossModuleVariableDatabase.hpp"
#include "MemoryBankAssigner.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase99 {

// Layout descriptor for linker coordination
struct LinkerLayoutDirective {
    std::string variableName;
    int targetBank = -1;
    size_t offsetInBank = 0;
    size_t sizeBytes = 0;
};

// Bank map for runtime reference
struct BankMapEntry {
    std::string variableName;
    int bank = -1;
    size_t offsetInBank = 0;
    size_t sizeBytes = 0;
};

// Linker integration coordinator
class LinkerIntegrator {
public:
    explicit LinkerIntegrator(const CrossModuleDatabase& db,
                             const BankLayout& layout)
        : database_(db), layout_(layout) {}

    // Main entry point: generate linker directives
    std::vector<LinkerLayoutDirective> generateLinkerDirectives();

    // Generate runtime bank map (for loader/debug)
    std::vector<BankMapEntry> generateBankMap();

    // Validate layout against linker constraints
    bool validateLayout(std::string& errorMessage);

    // Generate optimization hints for code generation
    std::string generateOptimizationHints();

    // Query: get bank for variable
    int getBankForVariable(const std::string& varName) const;

private:
    const CrossModuleDatabase& database_;
    const BankLayout& layout_;

    // Helper methods
    std::vector<LinkerLayoutDirective> computeRelocationOrder();
    bool checkBankCapacityConstraints(std::string& error) const;
    bool checkAlignmentConstraints(std::string& error) const;
    bool checkAddressOverlaps(std::string& error) const;
};

} // namespace phase99

