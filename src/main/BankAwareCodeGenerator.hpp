// Phase 99.4.3: CodeGenerator Integration Adapter
// Connects bank layout optimization to code generation

#pragma once

#include "BankSetupOptimizer.hpp"
#include "BankLayoutGenerator.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase99 {

// Code generation decisions for far variables
struct FarVarCodeGenDecision {
    std::string variableName;
    int targetBank = -1;
    bool hoistBankSetup = false;
    bool useCaching = false;
    int cacheRegister = -1;
    std::string optimizationHint;
};

// Adapter connecting bank optimization to CodeGenerator
class BankAwareCodeGenerator {
public:
    explicit BankAwareCodeGenerator(const BankLayout& layout,
                                   const LinkerIntegrator& integrator)
        : layout_(layout), integrator_(integrator) {}

    // Initialize adapter with bank layout
    void initializeBankLayout();

    // Analyze far variables for code generation
    std::map<std::string, FarVarCodeGenDecision> analyzeFarVariableCodeGen();

    // Get bank setup code for variable access
    std::string generateBankSetupCode(const std::string& varName);

    // Get optimized code for loop context
    std::string generateLoopOptimizedCode(const std::string& varName,
                                         bool inLoop);

    // Cache management for loop optimization
    void enterLoop(const std::string& loopLabel);
    void exitLoop();

    // Get optimization hints for specific variable
    std::string getOptimizationHint(const std::string& varName) const;

    // Record variable access pattern
    void recordVariableAccess(const std::string& varName,
                            int accessCount,
                            bool inLoop);

    // Query: current bank setup decisions
    const std::map<std::string, FarVarCodeGenDecision>& getDecisions() const {
        return decisions_;
    }

private:
    const BankLayout& layout_;
    const LinkerIntegrator& integrator_;
    BankSetupOptimizer optimizer_{layout_};
    BankCacheManager cacheManager_;
    std::map<std::string, FarVarCodeGenDecision> decisions_;

    // Helper methods
    std::string generateBankImmediate(int bank) const;
    std::string generateBankStore() const;
    bool isCached(const std::string& varName) const;
};

} // namespace phase99

