#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

// Phase C6.3: Redundant Load Elimination at IR level
// Eliminates duplicate LOAD operations when:
// - Same address loaded multiple times without intervening stores
// - Value already in a register (reverse store-forwarding)
// - Value unchanged across basic block boundaries (dataflow analysis)

class RedundantLoadElimination : public OptimizationPassBase {
public:
    RedundantLoadElimination();
    ~RedundantLoadElimination() override = default;

    // AST-level pass (deferred to IR)
    void apply(TranslationUnit& ast) override {}

    // IR-level optimization
    void apply(ir::Module& irModule) override;

private:
    struct MemoryValue {
        std::string address;  // Memory address (variable name or constant)
        std::string vreg;     // VReg holding the loaded value
        size_t instIndex;     // Instruction index of original load
        bool isValid;         // Still valid (no intervening stores)
    };

    struct BlockState {
        std::map<std::string, MemoryValue> loadedValues;  // address → (vreg, index)
        std::set<std::string> storedAddresses;            // Addresses written to
    };

    // Track loads and stores within a basic block
    void analyzeBlock(ir::Block& block, BlockState& state);

    // Check if address could be aliased by a store
    bool mayAlias(const std::string& loadAddr, const std::string& storeAddr) const;

    // Check if value in vreg matches last load from address
    bool isValueStillLoaded(const std::string& vreg, const std::string& address,
                           const BlockState& state) const;

    // Get all potential aliases for a memory address
    std::set<std::string> getAliases(const std::string& addr) const;

    // Metrics
    int redundantLoadsEliminated_ = 0;
    int reverseStoreForwards_ = 0;
    int aliasAnalysisBlockages_ = 0;
};
