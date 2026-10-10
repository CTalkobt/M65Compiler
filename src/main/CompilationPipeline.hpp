#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include "Stage.hpp"

// Compilation configuration
struct CompilationConfig {
    // Input/output
    std::string inputFile;
    std::string outputFile;

    // Compilation flags
    bool assemblyOnly = false;  // -S: stop after codegen
    bool objectOnly = false;    // -c: stop after assembly
    bool preprocessOnly = false;  // -E: stop after preprocessing
    bool saveTemps = false;     // --save-temps: keep intermediates

    // Optimization
    int optimizationLevel = 2;
    bool emitIR = false;        // --emit-ir: write IR text dump
    bool emitReasons = false;   // -Rcodegen: annotate assembly
    int maxInlineIRSize = 11;   // -fmax-inline-ir-size: max IR instructions for inlining
    bool signedDefault = false; // -fsigned-default: plain char/short/int/long are signed (standard C)
    bool unsignedChar = false;  // -funsigned-char: plain char stays unsigned even with -fsigned-default

    // Calling conventions
    bool zpCallMode = true;     // ZP parameter convention (default); -fno-zpcall selects stack parameters
    bool staticAllocMode = true;
    int stackTop = -1;          // -fno-stack16 (0) / -fstack-top=ADDR; -1 = 16-bit stack at $BFFF
    bool inlineSmallFunctions = false;

    // Language conformance
    bool allowImplicitFunctionDecl = false;  // -fimplicit-function-declaration: warn instead of error

    // Verbose output
    int verboseLevel = 0;

    // Includes and defines
    std::vector<std::string> includePaths;
    std::map<std::string, std::string> symbols;

    // Linking
    std::vector<std::string> libraryPaths;
    uint32_t prgBase = 0x2000;

    // Pragmas from CLI
    std::vector<std::string> cliPragmas;

    // Tool directory (directory containing cc45/ca45/ln45 binaries)
    std::string toolDir;
};

// Compilation result
struct CompilationResult {
    bool success = false;
    int exitCode = 0;
    std::string error;
    std::string outputFile;
};

// Main compilation pipeline orchestrator
class CompilationPipeline {
public:
    explicit CompilationPipeline(const CompilationConfig& config)
        : config_(config) {}

    // Execute the full compilation pipeline
    CompilationResult compile();

private:
    CompilationConfig config_;

    // Determine which stages to run based on config
    std::vector<std::unique_ptr<Stage>> buildPipelineStages();

    // Determine output file if not set
    void deriveOutputFiles();

    // Load source file
    std::string loadSourceFile(const std::string& filename);

    // Helper to run a stage
    Stage::Result runStage(Stage& stage);
};
