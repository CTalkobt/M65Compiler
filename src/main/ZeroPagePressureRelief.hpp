#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// Zero-Page Pressure Relief Optimization
// Identifies and migrates variables to avoid zero-page pressure/conflicts
class ZeroPagePressureRelief : public TemplateOptimizationPass {
public:
    ZeroPagePressureRelief();
    ~ZeroPagePressureRelief() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

    OptimizationMetrics getMetrics() const override { return metrics_; }
    std::string getName() const override { return "Zero-Page Pressure Relief"; }
    OptimizationType getType() const override { return OptimizationType::ZERO_PAGE_PRESSURE_RELIEF; }

private:
    struct ZPPressureProfile {
        std::string variableName;
        int accessFrequency;        // Number of accesses
        bool isHotVariable;         // Accessed in loops
        int estimatedSize;          // Bytes consumed
        bool canMigrate;            // Safe to move to main memory
        int bytesReduced;           // Space freed in zero-page
    };

    int variablesMigrated_;
    int bytesFreed_;

    // Profiling
    void profileZPUsage(TranslationUnit& ast);
    void profileZPUsageIR(ir::Module& irModule);

    // Decision logic
    bool shouldMigrate(const ZPPressureProfile& profile) const;
    int estimateRelief(int zpUsage) const;

    // Code generation
    void applyMigration(TranslationUnit& ast);
    void applyMigrationIR(ir::Module& irModule);
};
