#pragma once
#include "Stage.hpp"
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

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
                 bool zpCallMode = false,
                 bool saveTemps = false,
                 bool allowImplicitFuncDecl = false,
                 bool objectOnly = false,
                 uint32_t prgBase = 0x2000)
        : ast_(ast),
          analyzer_(analyzer),
          optimizationLevel_(optimizationLevel),
          verboseLevel_(verboseLevel),
          inlineFunctions_(inlineFunctions),
          staticAlloc_(staticAlloc),
          zpCallMode_(zpCallMode),
          saveTemps_(saveTemps),
          allowImplicitFuncDecl_(allowImplicitFuncDecl),
          objectOnly_(objectOnly),
          prgBase_(prgBase) {}

    Result execute() override;
    std::string getName() const override { return "CodeGen (IR)"; }

    // Get generated IR module
    std::shared_ptr<ir::Module> getIRModule() const { return irModule_; }

    // Get IR output (assembly-ready)
    std::string getIROutput() const { return irOutput_; }

    // Phase 102: Set typedef information from parser
    // Map: typedef_name → struct/union base type name
    void setMaxInlineIRSize(int n) { maxInlineIRSize_ = n; }
    void setMaxUnrollBytes(int n) { maxUnrollBytes_ = n; }

    void setSourceFile(const std::string& f) { sourceFile_ = f; }
    void setStackTop(int top) { stackTop_ = top; }
    // -f<opt>/-fno-<opt> (catalog name -> on/off), --future, -Roptir
    void setOptOverrides(const std::map<std::string, bool>& o) { optOverrides_ = o; }
    void setFutureWarnings(bool f) { futureWarnings_ = f; }
    void setTraceIROpt(bool t) { traceIROpt_ = t; }
    void setTypedefMappings(const std::map<std::string, std::string>& typedefMappings) {
        typedefMappings_ = typedefMappings;
    }

private:
    std::shared_ptr<TranslationUnit> ast_;
    std::shared_ptr<FunctionAnalyzer> analyzer_;
    int optimizationLevel_;
    int verboseLevel_;
    bool inlineFunctions_;
    int maxInlineIRSize_ = 11;
    int maxUnrollBytes_ = 128;
    int stackTop_ = -1;
    bool staticAlloc_;
    bool zpCallMode_;
    bool saveTemps_;
    bool allowImplicitFuncDecl_;
    bool objectOnly_;
    uint32_t prgBase_;
    std::shared_ptr<ir::Module> irModule_;
    std::string irOutput_;

    // Phase 102: Typedef mappings from parser (typedef_name → baseType)
    std::map<std::string, std::string> typedefMappings_;
    std::string sourceFile_;  // input file (module name for static symbols)
    std::map<std::string, bool> optOverrides_;
    bool futureWarnings_ = false;
    bool traceIROpt_ = false;

    // Is the catalog optimization `name` on: its -f/-fno- override, else the -O level's default
    bool optOn(const std::string& name) const;

    // Helper methods for codegen phases
    void configureIRBuilder();
    void generateIR();
    void applyIPOSpecialization();
    void optimizeIR();
    void applyInlining();
    void eliminateDeadCode();
    void generateAssembly();
};
