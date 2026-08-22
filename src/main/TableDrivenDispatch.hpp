#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <string>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Table-Driven Code Optimization
// Converts switch statements to jump tables for dense case ranges
// Eliminates cascading CMP/BRA comparisons
class TableDrivenDispatch : public TemplateOptimizationPass {
public:
    TableDrivenDispatch();
    ~TableDrivenDispatch() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

    OptimizationMetrics getMetrics() const override { return metrics_; }
    std::string getName() const override { return "Table-Driven Dispatch"; }
    OptimizationType getType() const override { return OptimizationType::TABLE_DRIVEN_DISPATCH; }

private:
    struct SwitchAnalysis {
        std::string switchVarName;      // Variable being switched on
        int minCase;                    // Minimum case value
        int maxCase;                    // Maximum case value
        int caseCount;                  // Number of cases
        int sparseGaps;                 // Gaps in range (sparse detection)
        bool isDense;                   // Dense enough for table
        int bytesReduced;              // Estimated savings
    };

    struct JumpTableEntry {
        int caseValue;
        std::string targetLabel;
    };

    int switchesOptimized_;
    int bytesReduced_;
    std::vector<SwitchAnalysis> switchCandidates_;

    // Analysis
    void analyzeSwitchStatements(TranslationUnit& ast);
    void analyzeSwitchStatementsIR(ir::Module& irModule);

    // Decision making
    bool shouldUseTable(const SwitchAnalysis& analysis) const;
    int calculateDensity(const SwitchAnalysis& analysis) const;
    int estimateTableBenefit(const SwitchAnalysis& analysis) const;

    // Code generation
    void generateJumpTables(TranslationUnit& ast);
    void generateJumpTablesIR(ir::Module& irModule);

    // Table management
    std::vector<JumpTableEntry> buildJumpTable(const SwitchAnalysis& analysis);
    int calculateTableSize(int minCase, int maxCase) const;
};
