#include "CompilationPipeline.hpp"
#include "Stage.hpp"
#include "PreprocessStage.hpp"
#include "ParseStage.hpp"
#include "Parser.hpp"  // Phase 102: For TypeAlias definition
#include "OptimizeStage.hpp"
#include "CodegenStage.hpp"
#include "AssemblyStage.hpp"
#include "LinkingStage.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>

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
            config_.outputFile = base + ".s45";
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

Stage::Result CompilationPipeline::runStage(Stage& stage) {
    if (config_.verboseLevel >= 1) {
        std::cout << "  " << stage.getName() << "..." << std::endl;
    }
    return stage.execute();
}

CompilationResult CompilationPipeline::compile() {
    CompilationResult result;

    try {
        // Derive output files if not specified
        deriveOutputFiles();
        result.outputFile = config_.outputFile;

        // Load source file
        std::string sourceRaw = loadSourceFile(config_.inputFile);

        if (config_.verboseLevel >= 1) {
            std::cout << "Compiling " << config_.inputFile << " (O"
                     << config_.optimizationLevel << ")..." << std::endl;
        }

        // Stage 1: Preprocess
        PreprocessStage preprocessStage(config_.inputFile, sourceRaw,
                                       config_.verboseLevel,
                                       config_.includePaths, config_.symbols);
        auto preprocessResult = runStage(preprocessStage);
        if (!preprocessResult.success) {
            result.success = false;
            result.exitCode = preprocessResult.exitCode;
            result.error = preprocessResult.error;
            return result;
        }
        std::string preprocessedSource = preprocessStage.getPreprocessedSource();

        if (config_.preprocessOnly) {
            std::ofstream out(config_.outputFile);
            out << preprocessedSource;
            result.success = true;
            return result;
        }

        // Stage 2: Parse
        ParseStage parseStage(preprocessedSource, config_.verboseLevel, config_.inputFile);
        auto parseResult = runStage(parseStage);
        if (!parseResult.success) {
            result.success = false;
            result.exitCode = parseResult.exitCode;
            result.error = parseResult.error;
            return result;
        }
        auto ast = parseStage.getAST();

        // Phase 102: Extract typedef information for IR generation
        // Build typedef mappings for struct/union types from parseStage
        std::map<std::string, std::string> typedefMappings;
        const auto& typedefInfo = parseStage.getTypedefInfo();
        for (const auto& [typedefName, typeInfo] : typedefInfo) {
            const auto& [baseType, pointerLevel] = typeInfo;
            // Only register struct/union typedefs
            if (baseType.find("struct ") == 0 || baseType.find("union ") == 0) {
                typedefMappings[typedefName] = baseType;
            }
        }

        // Stage 3: Optimize
        OptimizeStage optimizeStage(ast, config_.optimizationLevel,
                                   config_.verboseLevel, config_.inlineSmallFunctions);
        auto optimizeResult = runStage(optimizeStage);
        if (!optimizeResult.success) {
            result.success = false;
            result.exitCode = optimizeResult.exitCode;
            result.error = optimizeResult.error;
            return result;
        }
        auto analyzer = optimizeStage.getFunctionAnalyzer();

        // Stage 4: Codegen
        std::string tempAsmFile = "temp_" + std::to_string(getpid()) + ".s45";
        CodegenStage codegenStage(ast, analyzer, config_.optimizationLevel,
                                 config_.verboseLevel, config_.inlineSmallFunctions,
                                 config_.staticAllocMode, config_.saveTemps,
                                 config_.objectOnly);

        // Phase 102: Pass typedef mappings to codegen stage
        codegenStage.setTypedefMappings(typedefMappings);

        auto codegenResult = runStage(codegenStage);
        if (!codegenResult.success) {
            result.success = false;
            result.exitCode = codegenResult.exitCode;
            result.error = codegenResult.error;
            remove(tempAsmFile.c_str());
            return result;
        }
        std::string irOutput = codegenStage.getIROutput();

        // Write assembly to temp file
        std::ofstream asmFile(tempAsmFile);
        asmFile << irOutput;
        asmFile.close();

        if (config_.assemblyOnly) {
            std::rename(tempAsmFile.c_str(), config_.outputFile.c_str());
            result.success = true;
            return result;
        }

        // Stage 5: Assembly
        std::string objectFile = config_.objectOnly ? config_.outputFile :
                                (std::string("temp_") + std::to_string(getpid()) + ".o45");
        AssemblyStage asmStage(irOutput, objectFile, config_.verboseLevel,
                              config_.objectOnly, config_.toolDir);
        auto asmResult = runStage(asmStage);
        remove(tempAsmFile.c_str());

        if (!asmResult.success) {
            result.success = false;
            result.exitCode = asmResult.exitCode;
            result.error = asmResult.error;
            return result;
        }

        if (config_.objectOnly) {
            result.success = true;
            return result;
        }

        // Stage 6: Linking
        LinkingStage linkStage(objectFile, config_.outputFile, config_.verboseLevel,
                              std::to_string(config_.prgBase), config_.libraryPaths,
                              config_.toolDir);
        auto linkResult = runStage(linkStage);
        remove(objectFile.c_str());

        if (!linkResult.success) {
            result.success = false;
            result.exitCode = linkResult.exitCode;
            result.error = linkResult.error;
            return result;
        }

        if (config_.verboseLevel >= 1) {
            std::cout << "Compilation successful: " << config_.outputFile << std::endl;
        }

        result.success = true;
        result.exitCode = 0;
        return result;

    } catch (const std::exception& e) {
        result.success = false;
        result.exitCode = 1;
        result.error = std::string("Pipeline error: ") + e.what();
        return result;
    }
}
