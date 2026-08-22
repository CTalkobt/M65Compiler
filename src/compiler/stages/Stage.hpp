#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Base interface for compilation pipeline stages
class Stage {
public:
    virtual ~Stage() = default;

    // Stage result (success/failure)
    struct Result {
        bool success = false;
        std::string error;
        int exitCode = 0;
    };

    // Execute the stage
    virtual Result execute() = 0;

    // Stage name for logging
    virtual std::string getName() const = 0;
};

// Preprocessing stage
class PreprocessStage : public Stage {
public:
    PreprocessStage(const std::string& input, const std::string& output,
                   int verboseLevel, const std::vector<std::string>& includePaths,
                   const std::map<std::string, std::string>& symbols)
        : inputFile_(input), outputFile_(output), verboseLevel_(verboseLevel),
          includePaths_(includePaths), symbols_(symbols) {}

    Result execute() override;
    std::string getName() const override { return "Preprocess"; }

private:
    std::string inputFile_;
    std::string outputFile_;
    int verboseLevel_;
    std::vector<std::string> includePaths_;
    std::map<std::string, std::string> symbols_;
};

// Lexing/Parsing stage
class ParseStage : public Stage {
public:
    ParseStage(const std::string& source, int verboseLevel, bool emitAST = false)
        : source_(source), verboseLevel_(verboseLevel), emitAST_(emitAST) {}

    Result execute() override;
    std::string getName() const override { return "Parse"; }

    // Get parsed AST
    std::shared_ptr<TranslationUnit> getAST() const { return ast_; }
    std::vector<std::pair<int, std::pair<std::string, int>>> getLineMap() const { return lineMap_; }

private:
    std::string source_;
    int verboseLevel_;
    bool emitAST_;
    std::shared_ptr<TranslationUnit> ast_;
    std::vector<std::pair<int, std::pair<std::string, int>>> lineMap_;
};

// Optimization analysis stage
class OptimizeStage : public Stage {
public:
    OptimizeStage(std::shared_ptr<TranslationUnit> ast, int optimizationLevel, int verboseLevel)
        : ast_(ast), optimizationLevel_(optimizationLevel), verboseLevel_(verboseLevel) {}

    Result execute() override;
    std::string getName() const override { return "Optimize (AST)"; }

    std::shared_ptr<TranslationUnit> getAST() const { return ast_; }

private:
    std::shared_ptr<TranslationUnit> ast_;
    int optimizationLevel_;
    int verboseLevel_;
};

// IR Building and Optimization stage
class CodegenStage : public Stage {
public:
    CodegenStage(std::shared_ptr<TranslationUnit> ast,
                const std::string& outputAsm,
                int optimizationLevel, int verboseLevel,
                bool emitIR = false, bool zpCallMode = false, bool staticAllocMode = true)
        : ast_(ast), outputAsm_(outputAsm), optimizationLevel_(optimizationLevel),
          verboseLevel_(verboseLevel), emitIR_(emitIR), zpCallMode_(zpCallMode),
          staticAllocMode_(staticAllocMode) {}

    Result execute() override;
    std::string getName() const override { return "Codegen (IR + Assembly)"; }

    std::shared_ptr<ir::Module> getIRModule() const { return irModule_; }
    std::string getAssemblyFile() const { return outputAsm_; }

private:
    std::shared_ptr<TranslationUnit> ast_;
    std::string outputAsm_;
    int optimizationLevel_;
    int verboseLevel_;
    bool emitIR_;
    bool zpCallMode_;
    bool staticAllocMode_;
    std::shared_ptr<ir::Module> irModule_;
};

// Assembly stage (ca45 subprocess)
class AssemblyStage : public Stage {
public:
    AssemblyStage(const std::string& asmFile, const std::string& outputObj,
                 int optimizationLevel, int verboseLevel, bool emitReasons = false)
        : asmFile_(asmFile), outputObj_(outputObj), optimizationLevel_(optimizationLevel),
          verboseLevel_(verboseLevel), emitReasons_(emitReasons) {}

    Result execute() override;
    std::string getName() const override { return "Assemble (ca45)"; }

private:
    std::string asmFile_;
    std::string outputObj_;
    int optimizationLevel_;
    int verboseLevel_;
    bool emitReasons_;
};

// Linking stage (ln45 subprocess)
class LinkingStage : public Stage {
public:
    LinkingStage(const std::string& objectFile, const std::vector<std::string>& libPaths,
                const std::string& output, int verboseLevel, uint32_t prgBase = 0x2000)
        : objectFile_(objectFile), libPaths_(libPaths), output_(output),
          verboseLevel_(verboseLevel), prgBase_(prgBase) {}

    Result execute() override;
    std::string getName() const override { return "Link (ln45)"; }

private:
    std::string objectFile_;
    std::vector<std::string> libPaths_;
    std::string output_;
    int verboseLevel_;
    uint32_t prgBase_;
};
