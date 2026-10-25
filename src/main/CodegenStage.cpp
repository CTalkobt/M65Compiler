#include "SacSlotShare.hpp"
#include <cstdlib>
#include "IR.hpp"
#include "CodegenStage.hpp"
#include "AST.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "FunctionAnalyzer.hpp"
#include "IROptimizer.hpp"
#include "OptimizationCatalog.hpp"
#include <iostream>
#include <sstream>
#include <memory>

Stage::Result CodegenStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Generating IR (level " << optimizationLevel_ << ")..." << std::endl;
    }

    try {
        // Phase 1: Configure IRBuilder with compilation settings
        configureIRBuilder();

        // Phase 2: Generate IR from AST
        generateIR();

        // Phase 4: IR-level optimizations (8 passes)
        optimizeIR();

        // Phase 5: Apply inlining decisions
        if (inlineFunctions_ || optimizationLevel_ >= 2) {
            applyInlining();
        }

        // Phase 6: Dead code elimination
        eliminateDeadCode();

        // Phase 7: Generate final assembly IR
        generateAssembly();

        if (verboseLevel_ >= 1) {
            std::cout << "IR generation complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what(), 127};  // Exit code 127-168: code generation error
    }
}

void CodegenStage::configureIRBuilder() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Configuring IRBuilder..." << std::endl;
    }

    // IRBuilder configuration will be done within generate phase
    // This phase prepares any builder-level settings
}

void CodegenStage::generateIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Generating IR from AST..." << std::endl;
    }

    if (!ast_) return;

    // Create IRBuilder and generate IR from the AST
    IRBuilder builder;

    // Configure builder for this compilation
    builder.zpCallMode = zpCallMode_;
    builder.staticAllocMode = staticAlloc_;
    // -finline-functions forces inlining; -f[no-]inline-small-functions or the
    // -O level (2 and up) decide otherwise
    builder.inlineFunctions = optOverrides_.count("InlineSmallFunctions")
        ? optOverrides_.at("InlineSmallFunctions")
        : (inlineFunctions_ || optOn("InlineSmallFunctions"));
    builder.devirtualize = optOn("Devirtualization");
    builder.loopIdioms = optOn("LoopIdioms");
    builder.futureWarnings = futureWarnings_;
    builder.maxInlineIRSize = maxInlineIRSize_;
    builder.maxUnrollBytes = maxUnrollBytes_;
    builder.allowImplicitFunctionDecl = allowImplicitFuncDecl_;

    // Phase 102: Pass typedef mappings from parser
    builder.setTypedefMappings(typedefMappings_);
    if (!sourceFile_.empty()) builder.setSourceInfo(sourceFile_);

    // Generate IR from AST
    builder.generate(*ast_);

    // Print warnings
    const auto& warnings = builder.getWarnings();
    for (size_t i = 0; i < warnings.size(); i++) {
        if (!builder.warningAlreadyPrinted(i)) std::cerr << warnings[i] << std::endl;
    }

    if (builder.hasErrors()) {
        for (const auto& err : builder.getErrors()) {
            std::cerr << err << std::endl;
        }
        errorsPrinted_ = true;
        throw std::runtime_error(builder.getErrors().front());
    }

    // Get the generated module
    irModule_ = std::make_unique<ir::Module>(builder.getModule());
    // A #pragma cc45 no_stack16 / stack_top in the source wins over the flag
    if (irModule_->stackTop < 0) irModule_->stackTop = stackTop_;
    if (irModule_->bssFill < 0) irModule_->bssFill = bssFill_;
    if (irModule_->stackSize < 0) irModule_->stackSize = stackSize_;
    if (irModule_->overlay.empty()) irModule_->overlay = overlay_;

    if (verboseLevel_ >= 2) {
        std::cout << "    IR generated successfully" << std::endl;
    }
}

bool CodegenStage::optOn(const std::string& name) const {
    auto it = optOverrides_.find(name);
    if (it != optOverrides_.end()) return it->second;
    return OptimizationCatalog::isEnabledAtLevel(name, optimizationLevel_);
}

// The IR passes of IROptimizer.cpp, in order. Each runs on a function when its
// #pragma cc45 optimize(...) override says so, else when -f/-fno-/the -O level
// enables it (all are on from -O1).
void CodegenStage::optimizeIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Optimizing IR..." << std::endl;
    }
    if (!irModule_) return;

    struct Pass { const char* name; void (*run)(ir::Module&); };
    static const Pass passes[] = {
        {"LoopUnrolling",     ir::optimizeLoopUnroll},   // first: its constants feed the others
        // before TypeNarrowing, which then computes on the narrowed bytes
        {"ParamNarrowing",    ir::optimizeParamNarrowing},
        {"StrengthReduction", ir::optimizeStrengthReduction},
        {"AlgebraicSimplify", ir::optimizeAlgebraic},
        {"TypeNarrowing",     ir::optimizeTypeNarrowing},
        {"BranchFolding",     ir::optimizeBranchFold},
        {"CommonSubexprElim", ir::optimizeCSE},
        {"LICM",              ir::optimizeLICM},
        {"CopyPropagation",   ir::optimizeCopyChains},
        {"AddrElemFusion",    ir::optimizeAddrElemFusion},
        // again: constants the passes above made visible (a char divided by
        // a constant: the promoted divisor became a constant only now)
        {"StrengthReduction", ir::optimizeStrengthReduction},
        // and again: a char's x & x is (zext x) & (zext x), the same operand
        // only after TypeNarrowing / CSE
        {"AlgebraicSimplify", ir::optimizeAlgebraic},
    };

    ir::optTrace = traceIROpt_ ? &std::cerr : nullptr;
    ir::unrollMaxBytes = irModule_->maxUnrollBytes >= 0 ? irModule_->maxUnrollBytes : maxUnrollBytes_;
    ir::passEnabled = [this](const ir::Function& fn, const char* name) {
        auto it = fn.optOverrides.find(name);
        if (it != fn.optOverrides.end()) return it->second;
        return optOn(name);
    };
    // first: which functions are const / pure (the passes use it)
    if (optOn("PurityInference") || std::any_of(irModule_->functions.begin(), irModule_->functions.end(),
            [](const ir::Function& f) { auto it = f.optOverrides.find("PurityInference");
                                        return it != f.optOverrides.end() && it->second; }))
        ir::inferPurity(*irModule_);
    bool ranAny = false;
    for (const auto& p : passes) {
        bool any = optOn(p.name);
        for (const auto& fn : irModule_->functions) {
            auto it = fn.optOverrides.find(p.name);
            if (it != fn.optOverrides.end() && it->second) any = true;
        }
        if (!any) continue;
        if (verboseLevel_ >= 2) std::cout << "    " << p.name << std::endl;
        p.run(*irModule_);
        ranAny = true;
    }
    // What the passes left unused (part of dead-code elimination, always on)
    if (ranAny) ir::removeDeadTemps(*irModule_);
    // a leaf function's pointer parameters in ZP registers (allocation, always on)
    ir::leafPointerParamsToZp(*irModule_);
    // after it, so *p++ on such a parameter uses its ZP register
    if (optOn("PostIncrementSink") || std::any_of(irModule_->functions.begin(), irModule_->functions.end(),
            [](const ir::Function& f) { auto it = f.optOverrides.find("PostIncrementSink");
                                        return it != f.optOverrides.end() && it->second; }))
        ir::optimizePostIncrementSink(*irModule_);
    // what the sinking left unused (the old value of x++ as a statement)
    if (ranAny) ir::removeDeadTemps(*irModule_);
    // a[x + c] as element x of a shifted base (after the passes: CSE does not
    // see the offset), then the add it leaves unused
    if (ranAny && !getenv("CC45_NO_ELEM_OFFSET")) {
        ir::foldElemIndexOffset(*irModule_);
        ir::removeDeadTemps(*irModule_);
    }
    // g.y and a[3] at their addresses (after the passes: they see the add)
    if (ranAny) {
        ir::foldGlobalOffsetAccess(*irModule_);
        ir::removeDeadTemps(*irModule_);
    }
    // last: the blocks __unlikely / __likely say are cold at the end (loop
    // rotation follows in generateAssembly)
    ir::optimizeBlockReorder(*irModule_);
    ir::passEnabled = nullptr;
    ir::optTrace = nullptr;
}

void CodegenStage::applyInlining() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Applying inlining decisions..." << std::endl;
    }

    if (!irModule_ || !analyzer_) return;

    // Apply inlining decisions based on function characteristics
    const auto& characteristics = analyzer_->getAllCharacteristics();

    // Count inlining candidates
    int inlineCount = 0;
    for (const auto& [funcName, chars] : characteristics) {
        // Leaf functions < 10 bytes are primary inlining candidates
        if (chars.isLeaf && chars.estimatedCodeSize < 10) {
            inlineCount++;
        }
        // Small functions < 20 bytes with low complexity are secondary candidates
        else if (chars.estimatedCodeSize < 20 &&
                 chars.loopCount == 0 && chars.branchCount <= 1) {
            inlineCount++;
        }
    }

    if (verboseLevel_ >= 2 && inlineCount > 0) {
        std::cout << "    Found " << inlineCount << " inlining candidates." << std::endl;
    }
}

void CodegenStage::eliminateDeadCode() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Eliminating dead code..." << std::endl;
    }

    if (!irModule_) return;

    // Final DCE pass to remove unused functions and variables
    // This is separate from IR optimization DCE because it operates at a
    // different granularity (module-level functions vs IR instructions)

    if (verboseLevel_ >= 3) {
        std::cout << "    Dead code elimination complete." << std::endl;
    }
}

void CodegenStage::generateAssembly() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Generating assembly..." << std::endl;
    }

    if (!irModule_) return;

    // CC45_DUMP_IR=1: print the optimized IR to stderr (debugging aid)
    if (const char* d = getenv("CC45_DUMP_IR"); d && *d && *d != '0') ir::Printer::print(std::cerr, *irModule_);

    // Generate assembly text for a module
    auto gen = [&](const ir::Module& mod) {
        std::ostringstream ss;
        IRCodeGen codegen(ss);
        codegen.setFunctionSections(functionSections_);
        // Parameters: module, zpStart, relocMode, zpCallMode, emitReasons, staticAllocMode, sacDebugMode, prgBase
        codegen.generate(mod,
                         0x08,              // zpStart
                         objectOnly_,       // relocMode (true for -c/.o45, false for direct PRG)
                         zpCallMode_,       // zpCallMode
                         verboseLevel_ >= 3, // emitReasons
                         staticAlloc_,      // staticAllocMode
                         verboseLevel_ >= 3, // sacDebugMode
                         prgBase_);
        std::string out = ss.str();
        // SAC locals never live at the same time share their storage (#324)
        if (staticAlloc_ && optOn("SacSlotSharing")) out = shareSacSlots(out);
        return out;
    };

    // Loop rotation (#138): forced for the functions -frotate-loops / a
    // pragma names; otherwise (on from -O1, not -Os) kept for a function only
    // when its code is no larger, both ways assembled by ca45 (and the file's
    // code no larger as a whole)
    std::set<std::string> forced, trial;
    for (const auto& fn : irModule_->functions) {
        if (fn.noOptimize) continue;    // (#pragma cc45 optimize(off))
        // (a pragma, then -f / -fno-, decide outright; otherwise the level)
        auto it = fn.optOverrides.find("LoopRotation");
        auto g = optOverrides_.find("LoopRotation");
        if (it != fn.optOverrides.end()) {
            if (it->second) forced.insert(fn.name);
        } else if (g != optOverrides_.end()) {
            if (g->second) forced.insert(fn.name);
        } else if (optOn("LoopRotation")) {
            trial.insert(fn.name);
        }
    }
    ir::optTrace = traceIROpt_ ? &std::cerr : nullptr;
    if (!forced.empty())
        ir::optimizeLoopRotation(*irModule_, [&](const ir::Function& f) { return forced.count(f.name) > 0; });
    ir::optTrace = nullptr;
    std::string base = gen(*irModule_);
    irOutput_ = base;
    if (!trial.empty() && sizeTrial_) {
        ir::Module rot = *irModule_;
        auto rotated = ir::optimizeLoopRotation(rot, [&](const ir::Function& f) { return trial.count(f.name) > 0; });
        // each function's code bytes: from its start (its name, or its SAC
        // entry) to the next function's, the last to the end of the code
        auto sizesOf = [&](const TrialResult& r) {
            std::map<std::string, uint32_t> start, size;
            std::set<uint32_t> starts;
            for (const auto& fn : irModule_->functions) {
                uint32_t at = UINT32_MAX;
                for (const std::string& n : {fn.name, fn.name + "__sac"}) {
                    auto it = r.starts.find(n);
                    if (it != r.starts.end()) at = std::min(at, it->second);
                }
                if (at != UINT32_MAX) { start[fn.name] = at; starts.insert(at); }
            }
            for (const auto& [n, at] : start) {
                auto next = starts.upper_bound(at);
                size[n] = (next == starts.end() ? r.textSize : *next) - at;
            }
            return size;
        };
        std::vector<TrialResult> res;
        std::string text1 = rotated.empty() ? std::string() : gen(rot);
        if (!rotated.empty() && sizeTrial_({base, text1}, res) && res.size() == 2) {
            auto s0 = sizesOf(res[0]), s1 = sizesOf(res[1]);
            std::set<std::string> keep;
            for (const auto& f : rotated) {
                bool k = s0.count(f) && s1.count(f) && s1[f] <= s0[f];
                if (k) keep.insert(f);
                if (traceIROpt_)
                    std::cerr << "[rotate] " << f << ": " << (k ? "kept" : "undone") << " ("
                              << s0[f] << " -> " << s1[f] << " bytes)" << std::endl;
            }
            std::string cand;
            uint32_t candSize = 0;
            if (keep.size() == rotated.size()) {
                cand = text1;
                candSize = res[1].textSize;
            } else if (!keep.empty()) {
                ir::Module mix = *irModule_;
                for (auto& fn : mix.functions)
                    if (keep.count(fn.name))
                        for (const auto& rf : rot.functions)
                            if (rf.name == fn.name) { fn = rf; break; }
                cand = gen(mix);
                std::vector<TrialResult> rc;
                if (sizeTrial_({cand}, rc) && rc.size() == 1) candSize = rc[0].textSize;
                else cand.clear();
            }
            // (and the file's code no larger as a whole)
            if (!cand.empty() && candSize <= res[0].textSize) irOutput_ = cand;
            else if (!cand.empty() && traceIROpt_)
                std::cerr << "[rotate] the file: undone (" << res[0].textSize << " -> " << candSize << " bytes)" << std::endl;
        }
    }

    if (verboseLevel_ >= 2) {
        std::cout << "    Generated " << irOutput_.size()
                 << " bytes of assembly." << std::endl;
    }
}
