#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <map>

// Phase C7.1: Copy Propagation at IR level
// Eliminates redundant copy operations (MOV instructions)
// - Track copy chains (a = b, b = c → a = c)
// - Replace uses of copies with original values
// - Substitute backward through copy chains

class CopyPropagation : public OptimizationPassBase {
public:
    CopyPropagation();
    ~CopyPropagation() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct CopyInfo {
        std::string source;  // Original value
        size_t instIndex;
        bool isValid;
    };

    // Track copy assignments
    std::map<std::string, CopyInfo> copyMap_;

    // Get ultimate source of a vreg (follow copy chains)
    std::string getUltimateSource(const std::string& vreg) const;

    // Check if operand is a copy target
    bool isCopyTarget(const ir::Operand& op) const;

    // Metrics
    int copiesEliminated_ = 0;
    int chainsFollowed_ = 0;
};
