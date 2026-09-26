#pragma once
#include <string>
#include <vector>
#include <map>
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

    // Calling conventions
    bool zpCallMode = false;
    bool staticAllocMode = true;
    bool inlineSmallFunctions = false;

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
