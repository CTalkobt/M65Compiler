#pragma once
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "AssemblerTypes.hpp"

class AssemblerParser;
struct O45File;

// Phase 5: External function attributes for inter-TU optimization
struct ExternalFuncInfo {
    uint8_t regMask;
    bool isLeaf;
};

// Named optimization flags: per-pass control via -P<Name> / -PNo<Name>
struct OptimizationFlags {
    bool jsrRelocate = true;  // JSR → BSR relocation for position-independent code

    // Constructor to reset all flags based on optimization level
    static OptimizationFlags fromLevel(int level) {
        OptimizationFlags flags;
        // All passes enabled at level 1+
        if (level <= 0) {
            flags.jsrRelocate = false;
        }
        return flags;
    }
};

class AssemblerOptimizer {
public:
    static bool optimize(AssemblerParser* parser, bool verbose = false, bool traceMachState = false);

    // Phase 5: Load clobber info from external .o45 files for inter-TU optimization
    static bool optimizeWithExternalObjects(
        AssemblerParser* parser,
        const std::vector<std::string>& objectFiles,
        bool verbose = false,
        bool traceMachState = false
    );

private:
    // Internal optimization with optional external function info
    static bool optimizeInternal(
        AssemblerParser* parser,
        const std::map<std::string, ExternalFuncInfo>* externalFuncs,
        bool verbose,
        bool traceMachState,
        int optimizationLevel = 2
    );
};
