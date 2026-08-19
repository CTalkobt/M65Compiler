#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

// Comprehensive optimization catalog with fine-grained control
// Supports -O0 through -O9 levels with per-optimization configuration

enum class OptimizationCategory {
    SPACE,      // Reduces code size
    SPEED,      // Improves execution speed
    BOTH,       // Benefits both size and speed
};

struct OptimizationInfo {
    std::string name;                    // Optimization identifier (e.g., "ConstantPropagation")
    std::string displayName;             // Human-readable name
    std::string description;             // What the optimization does
    OptimizationCategory category;       // space/speed/both
    int enabledAtLevel;                  // Minimum -O level to enable (0-9)
    int disabledAtLevel;                 // Level at which to disable (-1 = never)
    std::string flag;                    // Command-line flag (e.g., "-O<name>")
    bool requiresAnalysis;               // Needs flow analysis
    int estimatedBenefit;                // Percentage savings/speedup
};

class OptimizationCatalog {
public:
    // Get all registered optimizations
    static const std::vector<OptimizationInfo>& getAllOptimizations();

    // Get optimization by name
    static const OptimizationInfo* getOptimization(const std::string& name);

    // Get optimizations enabled at a specific level
    static std::vector<OptimizationInfo> getOptimizationsAtLevel(int level);

    // Get optimizations by category
    static std::vector<OptimizationInfo> getOptimizationsByCategory(OptimizationCategory category);

    // Check if optimization is enabled at level
    static bool isEnabledAtLevel(const std::string& name, int level);

    // Get documentation for all optimizations
    static std::string getDocumentation();

private:
    static void initializeCatalog();
    static std::vector<OptimizationInfo> catalog_;
    static bool initialized_;
};
