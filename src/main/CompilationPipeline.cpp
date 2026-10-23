#include "CompilationPipeline.hpp"
#include "Stage.hpp"
#include "PreprocessStage.hpp"
#include "ParseStage.hpp"
#include "Parser.hpp"  // Phase 102: For TypeAlias definition
#include "OptimizeStage.hpp"
#include "CodegenStage.hpp"
#include "AssemblyStage.hpp"
#include "LinkingStage.hpp"
#include "OptimizationCatalog.hpp"
#include "O45Reader.hpp"
#include <iterator>
#include <thread>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

// from to to (rename, or a copy across file systems)
static bool moveFile(const std::string& from, const std::string& to) {
    if (std::rename(from.c_str(), to.c_str()) == 0) return true;
    std::ifstream in(from, std::ios::binary);
    std::ofstream out(to, std::ios::binary);
    if (!in || !out) return false;
    out << in.rdbuf();
    out.close();
    if (!out) return false;
    std::remove(from.c_str());
    return true;
}

std::string CompilationPipeline::loadSourceFile(const std::string& filename) {
    // An ifstream opens a directory and reads it as an empty file
    struct stat st;
    if (stat(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        throw std::runtime_error("Input file is a directory: " + filename);
    }
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

        // Add config-dependent predefined macros
        auto symbols = config_.symbols;
        if (config_.staticAllocMode) {
            symbols["__CC45_STATIC_ALLOC__"] = "1";
        }

        // Stage 1: Preprocess
        PreprocessStage preprocessStage(config_.inputFile, sourceRaw,
                                       config_.verboseLevel,
                                       config_.includePaths, symbols);
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
        parseStage.setSignedDefault(config_.signedDefault);
        parseStage.setUnsignedChar(config_.unsignedChar);
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
        optimizeStage.setOptOverrides(config_.optOverrides);
        optimizeStage.setTraceIROpt(config_.traceIROpt);
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
                                 config_.staticAllocMode, config_.zpCallMode, config_.saveTemps,
                                 config_.allowImplicitFunctionDecl,
                                 // relocatable for an object, and for a program (ln45
                                 // links it with crt0); standalone only for -S
                                 !config_.assemblyOnly, config_.prgBase);

        // Phase 102: Pass typedef mappings to codegen stage
        codegenStage.setTypedefMappings(typedefMappings);
        codegenStage.setMaxInlineIRSize(config_.maxInlineIRSize);
        codegenStage.setMaxUnrollBytes(config_.maxUnrollBytes);
        codegenStage.setSourceFile(config_.inputFile);
        codegenStage.setStackTop(config_.stackTop);
        codegenStage.setFunctionSections(config_.functionSections);
        codegenStage.setBssFill(config_.bssFill);
        codegenStage.setStackSize(config_.stackSize);
        codegenStage.setOverlay(config_.overlayName);
        codegenStage.setOptOverrides(config_.optOverrides);
        codegenStage.setFutureWarnings(config_.futureWarnings);
        codegenStage.setTraceIROpt(config_.traceIROpt);

        // -f/-fno- of the optimizations ca45 implements: forwarded as -P<name>/-PNo<name>
        bool needRelocatable = config_.objectOnly || !config_.assemblyOnly;
        std::string asmArgs;
        for (const auto& [name, on] : config_.optOverrides) {
            const OptimizationInfo* o = OptimizationCatalog::getOptimization(name);
            if (o && o->impl == OptimizationImpl::ASM_PASS)
                asmArgs += std::string(asmArgs.empty() ? "" : " ") + (on ? "-P" : "-PNo") + o->asmFlag;
        }
        // -ffunction-sections: a section per function (ln45 drops unused ones)
        if (config_.functionSections && needRelocatable)
            asmArgs += std::string(asmArgs.empty() ? "" : " ") + "-ffunction-sections";
        // Trial assemblies (loop rotation, #138), at once, as the assembly
        // stage would run ca45: where each text's exported code symbols start;
        // the objects are kept, and the one of the text codegen keeps is the
        // output (no assembly of it again)
        std::vector<std::pair<std::string, std::string>> trialObjects;   // text, object
        int trialCount = 0;
        codegenStage.setSizeTrial([&](const std::vector<std::string>& texts,
                                      std::vector<CodegenStage::TrialResult>& out) {
            std::vector<std::string> stems;
            std::vector<std::thread> runs;
            std::vector<int> rcs(texts.size(), -1);
            for (size_t k = 0; k < texts.size(); k++) {
                std::string stem = "temp_" + std::to_string(getpid()) + "_trial" + std::to_string(trialCount++);
                stems.push_back(stem);
                std::ofstream f(stem + ".s45");
                f << texts[k];
                if (!f) return false;
            }
            for (size_t k = 0; k < texts.size(); k++)
                runs.emplace_back([&, k] {
                    std::string cmd = config_.toolDir + "ca45 " + stems[k] + ".s45 -c -o " + stems[k] + ".o45";
                    if (!asmArgs.empty()) cmd += " " + asmArgs;
                    cmd += " >/dev/null 2>&1";
                    rcs[k] = system(cmd.c_str());
                });
            for (auto& t : runs) t.join();
            bool ok = true;
            out.clear();
            for (size_t k = 0; k < texts.size(); k++) {
                remove((stems[k] + ".s45").c_str());
                std::ifstream in(stems[k] + ".o45", std::ios::binary);
                std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                O45File obj;
                std::string err;
                if (rcs[k] != 0 || data.empty() || !O45Reader::read(data, obj, err)) {
                    remove((stems[k] + ".o45").c_str());
                    ok = false;
                    continue;
                }
                CodegenStage::TrialResult r;
                for (const auto& e : obj.exports)
                    if ((e.segment & 0x7F) == SEG_TEXT) r.starts[e.name] = e.offset;
                r.textSize = (uint32_t)obj.textBody.size();
                out.push_back(std::move(r));
                trialObjects.push_back({texts[k], stems[k] + ".o45"});
            }
            return ok;
        });
        auto dropTrials = [&](const std::string& keep) {
            for (const auto& [t, o] : trialObjects)
                if (o != keep) remove(o.c_str());
        };

        auto codegenResult = runStage(codegenStage);
        if (!codegenResult.success) {
            result.success = false;
            result.exitCode = codegenResult.exitCode;
            result.error = codegenResult.error;
            result.errorPrinted = codegenStage.errorsPrinted();
            remove(tempAsmFile.c_str());
            dropTrials("");
            return result;
        }
        std::string irOutput = codegenStage.getIROutput();
        // a trial assembled this very text (with the same options): its object
        std::string trialObject;
        if (needRelocatable && !config_.saveTemps)
            for (const auto& [t, o] : trialObjects)
                if (t == irOutput) { trialObject = o; break; }
        dropTrials(trialObject);

        // Write assembly to temp file
        std::ofstream asmFile(tempAsmFile);
        asmFile << irOutput;
        asmFile.close();

        if (config_.assemblyOnly) {
            // -S: the assembly is the output. Write it directly (a rename of the
            // temp file fails across filesystems and for /dev/null)
            remove(tempAsmFile.c_str());
            std::ofstream out(config_.outputFile);
            out << irOutput;
            if (!out) {
                result.success = false;
                result.exitCode = 253;
                result.error = "Cannot write " + config_.outputFile;
                return result;
            }
            result.success = true;
            return result;
        }

        // Stage 5: Assembly
        std::string objectFile = config_.objectOnly ? config_.outputFile :
                                (std::string("temp_") + std::to_string(getpid()) + ".o45");
        // Always use relocatable mode (-c) when producing .o45 for linker
        AssemblyStage asmStage(irOutput, objectFile, config_.verboseLevel,
                              needRelocatable, config_.toolDir);
        asmStage.setExtraArgs(asmArgs);
        asmStage.setKeepTemp(config_.saveTemps);
        Stage::Result asmResult{true};
        if (!trialObject.empty() && moveFile(trialObject, objectFile)) {
            if (config_.verboseLevel >= 1) std::cout << "Assembled (a trial's object)." << std::endl;
        } else {
            if (!trialObject.empty()) remove(trialObject.c_str());
            asmResult = runStage(asmStage);
        }
        if (!config_.saveTemps) remove(tempAsmFile.c_str());

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
        // Convert prgBase to hex string for linker's -t flag
        char prgBaseHex[16];
        snprintf(prgBaseHex, sizeof(prgBaseHex), "%x", config_.prgBase);
        LinkingStage linkStage(objectFile, config_.outputFile, config_.verboseLevel,
                              config_.prgBaseSet ? std::string(prgBaseHex) : std::string(),
                              config_.libraryPaths, config_.toolDir, config_.zpCallMode);
        linkStage.setGcInputs(config_.functionSections);
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
        result.exitCode = 253;  // Exit code 253-255: internal/pipeline error
        result.error = std::string("Pipeline error: ") + e.what();
        return result;
    }
}
