#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

// Phase C6.4: Dead Store Elimination at IR level
// Eliminates STORE operations whose values are never read:
// - Store to variable followed by overwrite without read
// - Store to volatile variable (cannot eliminate)
// - Store at end of block with no successor reads
// - Liveness analysis: variable never read after store

class DeadStoreElimination : public OptimizationPassBase {
public:
    DeadStoreElimination();
    ~DeadStoreElimination() override = default;

    // AST-level pass (deferred to IR)
    void apply(TranslationUnit& ast) override {}

    // IR-level optimization
    void apply(ir::Module& irModule) override;

private:
    struct StoreRecord {
        std::string address;     // Memory address (variable, symbol)
        size_t instIndex;        // Instruction index of STORE
        bool isVolatile;         // Cannot eliminate volatile stores
        bool isLiveAfter;        // Is value read after store
    };

    struct BlockLiveness {
        std::set<std::string> liveIn;    // Live at block entry
        std::set<std::string> liveOut;   // Live at block exit
        std::map<std::string, size_t> lastStore;  // address → instruction index
    };

    // Analyze basic block for live variables (backward pass)
    void analyzeBlockLiveness(ir::Block& block, BlockLiveness& liveness);

    // Check if variable is live after given instruction
    bool isLiveAfter(const std::string& addr, size_t instIndex,
                    const BlockLiveness& liveness) const;

    // Check if store can be safely eliminated
    bool isDeadStore(const ir::Inst& inst, const BlockLiveness& liveness) const;

    // Compute which stores are dead
    std::set<size_t> findDeadStores(ir::Block& block);

    // Mark address as volatile (cannot eliminate stores)
    void markVolatile(const std::string& addr);

    // Metrics
    int deadStoresEliminated_ = 0;
    int volatileStoresPreserved_ = 0;
    std::set<std::string> volatileAddresses_;
};
