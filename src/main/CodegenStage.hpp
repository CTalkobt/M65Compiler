#pragma once
#include "Stage.hpp"
#include <memory>
#include <string>
#include <vector>

class TranslationUnit;
class FunctionAnalyzer;
class IRBuilder;
class IROptimizer;
namespace ir { struct Module; }

// IR code generation stage
// Generates intermediate representation from AST with optimization
class CodegenStage : public Stage {
public:
    CodegenStage(std::shared_ptr<TranslationUnit> ast,
                 std::shared_ptr<FunctionAnalyzer> analyzer,
                 int optimizationLevel = 2,
                 int verboseLevel = 0,
                 bool inlineFunctions = false,
                 bool staticAlloc = false,
                 bool saveTemps = false)
        : ast_(ast),
          analyzer_(analyzer),
          optimizationLevel_(optimizationLevel),
          verboseLevel_(verboseLevel),
          inlineFunctions_(inlineFunctions),
          staticAlloc_(staticAlloc),
          saveTemps_(saveTemps) {}

    Result execute() override;
    std::string getName() const override { return "CodeGen (IR)"; }

    // Get generated IR module
    std::shared_ptr<ir::Module> getIRModule() const { return irModule_; }

    // Get IR output (assembly-ready)
    std::string getIROutput() const { return irOutput_; }

private:
    std::shared_ptr<TranslationUnit> ast_;
    std::shared_ptr<FunctionAnalyzer> analyzer_;
    int optimizationLevel_;
    int verboseLevel_;
    bool inlineFunctions_;
    bool staticAlloc_;
    bool saveTemps_;
    std::shared_ptr<ir::Module> irModule_;
    std::string irOutput_;

    // Helper methods for codegen phases
    void configureIRBuilder();
    void generateIR();
    void applyIPOSpecialization();
    void optimizeIR();
    void applyInlining();
    void eliminateDeadCode();
    void generateAssembly();
};
