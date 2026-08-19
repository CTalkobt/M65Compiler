#pragma once

#include "OptimizationCatalog.hpp"
#include <map>
#include <set>
#include <string>

// Controls which optimizations are enabled based on -O level and individual flags
// Supports -O0 through -O9, per-optimization enabling/disabling, and pragma directives

class OptimizationController {
public:
    // Set optimization level (-O0 through -O9)
    void setLevel(int level);
    int getLevel() const { return level_; }

    // Enable/disable specific optimization
    void enableOptimization(const std::string& name);
    void disableOptimization(const std::string& name);

    // Check if optimization is enabled
    bool isEnabled(const std::string& name) const;

    // Get all currently enabled optimizations
    std::set<std::string> getEnabledOptimizations() const;

    // Parse command-line options (e.g., "-O3", "-PNoLICM", "-OConstantFolding")
    bool parseOption(const std::string& option);

    // Apply level-based defaults
    void applyLevelDefaults();

    // Per-function optimization hints
    enum FunctionOptimizationMode {
        DEFAULT,  // Use controller settings
        NONE,     // Disable all optimizations
        SPEED,    // Enable only speed optimizations
        SPACE,    // Enable only space optimizations
    };

    // Set function-level optimization mode (via pragma)
    void setFunctionMode(const std::string& funcName, FunctionOptimizationMode mode);
    FunctionOptimizationMode getFunctionMode(const std::string& funcName) const;

    // Individual per-function optimization control (via pragma)
    // Examples: constant_folding, loop_unrolling, no_branch_inversion
    void setFunctionOptimization(const std::string& funcName, const std::string& optName, bool enable);
    bool isFunctionOptimizationEnabled(const std::string& funcName, const std::string& optName) const;
    bool hasFunctionOptimizationOverride(const std::string& funcName, const std::string& optName) const;

    // Get documentation
    std::string getDocumentation() const;

private:
    int level_ = 2;  // Default: -O2
    std::set<std::string> enabledOptimizations_;
    std::set<std::string> disabledOptimizations_;
    std::map<std::string, FunctionOptimizationMode> functionModes_;
    // Per-function per-optimization control: funcName -> (optName -> enabled)
    std::map<std::string, std::map<std::string, bool>> functionOptimizations_;

    // Convert snake_case pragma names to CamelCase internal names
    std::string pragmaNameToInternal(const std::string& pragmaName) const;
    // Convert CamelCase internal names to snake_case pragma names
    std::string internalNameToPragma(const std::string& internalName) const;
};
