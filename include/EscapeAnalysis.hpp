#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <set>
#include <map>

// Phase C7.7: Escape Analysis & Pointer Analysis at IR level
// Track pointer aliasing relationships and identify stack-allocated values
// - Stack-allocated values that don't escape
// - Enable safe stack-to-register promotion
// - Identify safe pointer operations

class EscapeAnalysis : public OptimizationPassBase {
public:
    EscapeAnalysis();
    ~EscapeAnalysis() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct AllocationInfo {
        std::string vreg;           // Virtual register
        bool escapesFunction;       // True if escapes to caller
        bool escapesBlock;          // True if escapes current block
        int escapeReason;           // Why it escapes (stored, passed, etc.)
        int allocationCount;
    };

    struct AliasGroup {
        std::set<std::string> vregs; // Vregs in this alias group
        bool mayAlias;
    };

    // Analyze which allocations escape the function
    void analyzeEscapes(ir::Function& func, std::map<std::string, AllocationInfo>& info);

    // Check if a value escapes via function call
    bool escapesViaCall(const std::string& vreg, const ir::Function& func) const;

    // Check if a value escapes via store operation
    bool escapesViaStore(const std::string& vreg, const ir::Inst& inst) const;

    // Build alias groups
    void buildAliasGroups(ir::Module& irModule, std::vector<AliasGroup>& groups);

    // Metrics
    int valuesPromoted_ = 0;
    int allocationsAnalyzed_ = 0;
    int escapePointsFound_ = 0;
};
