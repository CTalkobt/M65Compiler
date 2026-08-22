#include "CompilationPipeline.hpp"
#include "Stage.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string CompilationPipeline::loadSourceFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void CompilationPipeline::deriveOutputFiles() {
    if (config_.outputFile.empty()) {
        std::string base = config_.inputFile;
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);

        if (config_.objectOnly) {
            config_.outputFile = base + ".o45";
        } else if (config_.assemblyOnly) {
            config_.outputFile = base + ".s";
        } else {
            config_.outputFile = base + ".prg";
        }
    } else {
        // Infer output type from extension when -o is used
        size_t dot = config_.outputFile.rfind('.');
        if (dot != std::string::npos) {
            std::string ext = config_.outputFile.substr(dot);
            if (ext == ".s" || ext == ".s45") {
                config_.assemblyOnly = true;
            } else if (ext == ".o45") {
                config_.objectOnly = true;
            }
        }
    }
}

std::vector<std::unique_ptr<Stage>> CompilationPipeline::buildPipelineStages() {
    std::vector<std::unique_ptr<Stage>> stages;

    // TODO: Implement stage builders
    // This will be filled in as we refactor cc45_main.cpp

    return stages;
}

Stage::Result CompilationPipeline::runStage(Stage& stage) {
    if (config_.verboseLevel >= 1) {
        std::cout << stage.getName() << "..." << std::endl;
    }

    auto result = stage.execute();

    if (!result.success && config_.verboseLevel >= 1) {
        std::cerr << "Error in " << stage.getName() << ": " << result.error << std::endl;
    }

    return result;
}

CompilationResult CompilationPipeline::compile() {
    CompilationResult result;
    result.outputFile = config_.outputFile;

    try {
        // Derive output files if not specified
        deriveOutputFiles();

        // Load source file
        std::string sourceRaw = loadSourceFile(config_.inputFile);

        // TODO: Build and execute pipeline stages
        // 1. Preprocess
        // 2. Parse (Lex + Parse)
        // 3. Optimize (AST-level)
        // 4. Codegen (IR build + IR opt + assembly gen)
        // 5. Assembly (ca45 subprocess) - unless -S
        // 6. Link (ln45 subprocess) - unless -c or -S

        result.success = true;
        result.exitCode = 0;
        return result;

    } catch (const std::exception& e) {
        result.success = false;
        result.exitCode = 1;
        result.error = e.what();
        return result;
    }
}
