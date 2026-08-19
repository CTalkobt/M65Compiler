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

    // Get documentation
    std::string getDocumentation() const;

private:
    int level_ = 2;  // Default: -O2
    std::set<std::string> enabledOptimizations_;
    std::set<std::string> disabledOptimizations_;
    std::map<std::string, FunctionOptimizationMode> functionModes_;
};
