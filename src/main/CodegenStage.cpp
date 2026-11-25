#include "SacSlotShare.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include "IR.hpp"
#include "CodegenStage.hpp"
#include "AST.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "FunctionAnalyzer.hpp"
#include "IROptimizer.hpp"
#include "OptimizationCatalog.hpp"
#include "CLint.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <algorithm>
#include <fstream>
#include <set>

Stage::Result CodegenStage::execute() {
    if (verboseLevel_ >= 1) {
        Logger::stream(LogLevel::INFO) << "Generating IR (level " << optimizationLevel_ << ")..." << std::endl;
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
        if (lint_ && irModule_) clint::runFrames(*lint_, *irModule_, frames_);

        if (verboseLevel_ >= 1) {
            Logger::stream(LogLevel::INFO) << "IR generation complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what(), 127};  // Exit code 127-168: code generation error
    }
}

void CodegenStage::configureIRBuilder() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Configuring IRBuilder..." << std::endl;
    }

    // IRBuilder configuration will be done within generate phase
    // This phase prepares any builder-level settings
}

void CodegenStage::generateIR() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Generating IR from AST..." << std::endl;
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
    builder.foldStringBuiltins = optOn("Builtin");
    // (a byte compared inline is 8-12 bytes of code, more for an array in a
    // stack frame: at -O2 / -Os only s[0] == 0, no larger than the call;
    // -O3 and up, for speed, up to 8 bytes, and strcpy of a literal as memcpy)
    builder.stringCompareInlineBytes = optimizationLevel_ >= 3 ? 8 : 1;
    builder.strcpyLiteralAsMemcpy = optimizationLevel_ >= 3;
    builder.loopIdioms = optOn("LoopIdioms");
    builder.futureWarnings = futureWarnings_;
    builder.maxInlineIRSize = maxInlineIRSize_;
    builder.maxUnrollBytes = maxUnrollBytes_;
    builder.allowImplicitFunctionDecl = allowImplicitFuncDecl_;

    // Phase 102: Pass typedef mappings from parser
    builder.setTypedefMappings(typedefMappings_);
    if (!sourceFile_.empty()) builder.setSourceInfo(sourceFile_);

    if (lint_)
        builder.lintReport = [this](const std::string& id, const std::string& file, int line, int col,
                                    const std::string& msg) { lint_->report(id, file, line, col, msg); };

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
    if (fastFloat_) irModule_->fastFloat = true;
    if (irModule_->stackSize < 0) irModule_->stackSize = stackSize_;
    if (irModule_->overlay.empty()) irModule_->overlay = overlay_;
    for (const auto& s : unpackSites_) irModule_->unpackSites.push_back({s.format, s.file, s.line, s.column, s.what});
    // cl45: the rules on the IR as written (before any pass)
    if (lint_) {
        lint_->staticAlloc = staticAlloc_;
        clint::runIR(*lint_, *irModule_, *ast_);
    }

    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "IR generated successfully" << std::endl;
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
        Logger::stream(LogLevel::DEBUG) << "Optimizing IR..." << std::endl;
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
        // (after CSE: x / 3 and x % 3 of one x, one divide)
        {"StrengthReduction", ir::optimizeShareDivMod},
        {"LICM",              ir::optimizeLICM},
        // after LICM: g[r] hoisted out of an inner loop is reduced in the outer one
        {"LoopStrengthReduction", ir::optimizeLoopStrengthReduction},
        {"CopyPropagation",   ir::optimizeCopyChains},
        // again: copies merged (a ?: result's definitions are one vreg now),
        // and the copies its narrowing leaves
        {"TypeNarrowing",     ir::optimizeTypeNarrowing},
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
        if (verboseLevel_ >= 2) Logger::stream(LogLevel::DEBUG) << p.name << std::endl;
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
        ir::foldFarOffset(*irModule_);
        ir::foldNegOffset(*irModule_);
        ir::removeDeadTemps(*irModule_);
        ir::orderCommutativeOperands(*irModule_);
    }
    // last: the blocks __unlikely / __likely say are cold at the end (loop
    // rotation follows in generateAssembly)
    ir::optimizeBlockReorder(*irModule_);
    ir::passEnabled = nullptr;
    ir::optTrace = nullptr;
}

void CodegenStage::applyInlining() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Applying inlining decisions..." << std::endl;
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
        Logger::stream(LogLevel::DEBUG) << "Found " << inlineCount << " inlining candidates." << std::endl;
    }
}

void CodegenStage::eliminateDeadCode() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Eliminating dead code..." << std::endl;
    }

    if (!irModule_) return;

    // Final DCE pass to remove unused functions and variables
    // This is separate from IR optimization DCE because it operates at a
    // different granularity (module-level functions vs IR instructions)

    if (verboseLevel_ >= 3) {
        Logger::stream(LogLevel::DEEP) << "Dead code elimination complete." << std::endl;
    }
}

void CodegenStage::generateAssembly() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Generating assembly..." << std::endl;
    }

    if (!irModule_) return;

    // CC45_DUMP_IR=1: print the optimized IR to stderr (debugging aid)
    if (const char* d = getenv("CC45_DUMP_IR"); d && *d && *d != '0') ir::Printer::print(std::cerr, *irModule_);

    // Generate assembly text for a module
    auto gen = [&](const ir::Module& mod) {
        // #607: a function whose code changes a register or flag the
        // estimate of it did not have (what its .reg_clobbers / .flag_clobbers
        // declare, and what cc45 keeps in registers across a call of it) is
        // generated again with what was measured, until nothing is missing
        std::string out;
        std::map<std::string, std::pair<uint8_t, uint8_t>> clobberOverrides;
        for (int round = 0; round < 4; round++) {
            std::ostringstream ss;
            IRCodeGen codegen(ss);
            codegen.setFunctionSections(functionSections_);
            codegen.setF32PtrCall(objectOnly_ && optOn("FloatCopyCall"));
            if (round == 3)   // (it did not settle: everything, for the functions in question)
                for (auto& o : clobberOverrides) o.second = {0x0F, 0x0F};
            codegen.setClobberOverrides(clobberOverrides);
            codegen.setLintMode(lint_ != nullptr);
            codegen.setAllocReport(allocReport_ && !lint_);
            // Parameters: module, zpStart, relocMode, zpCallMode, emitReasons, staticAllocMode, sacDebugMode, prgBase
            codegen.generate(mod,
                             0x08,              // zpStart
                             objectOnly_,       // relocMode (true for -c/.o45, false for direct PRG)
                             zpCallMode_,       // zpCallMode
                             emitReasons_,      // emitReasons (-Rcodegen: why each instruction)
                             staticAlloc_,      // staticAllocMode
                             false,             // sacDebugMode (code of its own: not by -v)
                             prgBase_);
            out = ss.str();
            if (lint_) frames_ = codegen.frameSizes();
            auto fixes = codegen.clobberFixes();
            if (fixes.empty()) break;
            if (getenv("CC45_CLOBBER_TRACE"))
                for (const auto& f : fixes)
                    std::cerr << "clobbers of " << f.first << ": the code changes more than estimated (regs "
                              << (int)f.second.first << ", flags " << (int)f.second.second << ")\n";
            for (const auto& f : fixes) {
                auto& o = clobberOverrides[f.first];
                o.first |= f.second.first;
                o.second |= f.second.second;
            }
        }
        // SAC locals never live at the same time share their storage (#324)
        if (staticAlloc_ && optOn("SacSlotSharing")) out = shareSacSlots(out);
        // many float copies as calls of __f32cpy (#485)
        if (objectOnly_ && optOn("FloatCopyCall")) out = IRCodeGen::callF32Copies(out);
        return out;
    };

    // Parameter copy lists (#359): a pragma, then -fzp-args / -fno-zp-args,
    // then the level (off from -O3 / -Ospeed: the copies inline, faster)
    for (auto& fn : irModule_->functions) {
        auto it = fn.optOverrides.find("ZpArgs");
        fn.argLists = it != fn.optOverrides.end() ? it->second : optOn("ZpArgs");
    }

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
    if (!trial.empty() && sizeTrial_ && !lint_) {
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

    if (allocReport_) irOutput_ = finishAllocReport(irOutput_, std::cerr);

    // research/pcode: CC45_PCODE_STATS=<file> appends, per function, the
    // bytes its final IR would take in a byte-oriented p-code beside the
    // bytes of its 45GS02 code (the assembled text, as the size trial
    // finds each function's start)
    if (const char* statsPath = getenv("CC45_PCODE_STATS"); statsPath && *statsPath && sizeTrial_ && !lint_)
        writePcodeStats(statsPath);

    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Generated " << irOutput_.size()
                 << " bytes of assembly." << std::endl;
    }
}

// -Ralloc (#248): IRCodeGen's "; @alloc" lines in the final assembly, with
// what SacSlotShare made of the static slots (the labels of a group on one
// .res: a slot sharing its bytes; cc45.sacsize.<f>: the block's size after
// it), written to err and kept in the assembly as "; alloc:" comments
std::string CodegenStage::finishAllocReport(const std::string& text, std::ostream& err) {
    std::vector<std::string> lines;
    {
        std::istringstream in(text);
        for (std::string l; std::getline(in, l);) lines.push_back(l);
    }
    auto trim = [](const std::string& t) {
        size_t a = t.find_first_not_of(" \t"), b = t.find_last_not_of(" \t");
        return a == std::string::npos ? std::string() : t.substr(a, b - a + 1);
    };
    std::map<std::string, std::vector<std::string>> shares;   // a slot: the others on its .res
    std::map<std::string, int> sacSize;
    std::vector<std::string> pending;
    for (const auto& l : lines) {
        std::string c = trim(l);
        if (c.empty() || c[0] == ';') continue;
        if (c.back() == ':' && c.find_first_of(" \t") == std::string::npos) {
            // (a SAC slot: SacSlotShare groups only those; the block's own
            // label, _f__ar, sits before its first slot)
            if (c.find("__local_") != std::string::npos) pending.push_back(c.substr(0, c.size() - 1));
            continue;
        }
        if (c.rfind(".res ", 0) == 0 && pending.size() > 1)
            for (const auto& a : pending)
                for (const auto& b : pending) if (a != b) shares[a].push_back(b);
        if (c.rfind("cc45.sacsize.", 0) == 0) {
            size_t eq = c.find('=');
            if (eq != std::string::npos) sacSize[trim(c.substr(13, eq - 13))] = std::atoi(c.c_str() + eq + 1);
        }
        pending.clear();
    }
    const std::string tag = "; @alloc ";
    // a slot's variable as the report names it (its line: "  <who> <kind> <n> <slot> ...")
    std::map<std::string, std::string> whoOf;
    for (const auto& l : lines) {
        if (l.rfind(tag + "  ", 0) != 0) continue;
        std::istringstream f(l.substr(tag.size()));
        std::string who, kind, n, where;
        if (f >> who >> kind >> n >> where) whoOf[where] = who;
    }
    std::ostringstream o;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string l = lines[i];
        if (l.rfind(tag, 0) == 0) {
            std::string t = l.substr(tag.size());
            if (t.rfind("  ", 0) != 0) {
                // the function's line: the static block's size after sharing
                std::string fn = t.substr(0, t.find(':'));
                size_t at = t.find(", static ");
                auto z = sacSize.find(fn);
                if (at != std::string::npos && z != sacSize.end()) {
                    size_t num = at + 9, end = t.find(' ', num);
                    int before = std::atoi(t.c_str() + num);
                    if (z->second != before)
                        t = t.substr(0, num) + std::to_string(z->second) + " bytes (" + std::to_string(before) +
                            " before sharing)" + t.substr(t.find(" bytes", end) + 6);
                }
            } else {
                for (const auto& [slot, others] : shares) {
                    size_t at = t.find(slot);
                    if (at == std::string::npos) continue;
                    size_t e = at + slot.size();
                    if (e < t.size() && (std::isalnum((unsigned char)t[e]) || t[e] == '_')) continue;
                    std::string with;
                    for (const auto& w : others) {
                        auto nm = whoOf.find(w);
                        with += (with.empty() ? "" : ", ") + (nm != whoOf.end() ? nm->second : w);
                    }
                    t += "; its bytes shared with " + with + " (never live together, #324)";
                    break;
                }
            }
            err << t << "\n";
            l = "; alloc: " + t;
        }
        o << l;
        if (i + 1 < lines.size() || (!text.empty() && text.back() == '\n')) o << "\n";
    }
    return o.str();
}


// ---------------------------------------------------------------------------
// research/pcode: what the IR would take as p-code
//
// The encoding measured (scheme A, a register p-code mirroring the IR):
//   opcode     1 byte: the operation and its operand type
//   vreg       1 byte (2 when the function has more than 256 vregs)
//   immediate  1 byte when it fits in -128..255, else its type's size (>= 2)
//   global     2 bytes (an address); elemOffset != 0: +2
//   label      2 bytes; a BR to the next block is nothing, a BR_COND one of
//              whose targets is the next block names the other only
//   call       target 2 (indirect: its vreg), an argument count byte, the
//              arguments as operands
//   switch     value, default label, a count byte, each case (value + label)
//   asm        not encodable: counted apart (native escape)
// Per function: a 3-byte header (frame size, parameter bytes, flags) and the
// native entry that enters the interpreter (jsr __pcode: 3 bytes).
namespace {
int pcImm(const ir::Operand& o) {
    if (o.immVal >= -128 && o.immVal <= 255) return 1;
    int n = ir::typeSize(o.type);
    if (o.type == ir::Type::I_N) n = o.byteWidth;
    return n < 2 ? 2 : n;
}
int pcOperand(const ir::Operand& o, int vb) {
    switch (o.kind) {
        case ir::OperandKind::NONE: return 0;
        case ir::OperandKind::VREG: return vb;
        case ir::OperandKind::IMM: return pcImm(o);
        case ir::OperandKind::GLOBAL: return 2;
        case ir::OperandKind::LABEL: return 2;
        case ir::OperandKind::FRAME_RELATIVE: return 2;
    }
    return 0;
}
}  // namespace

// Scheme B: scheme A with the combined instructions of the interpreter's
// version 2 (research/pcode/pcode_vm.s45), where a temporary is used only
// by the next instruction:
//   const t; store t -> x            SET x, #imm            1 + v + imm
//   t = a op b; store t -> x         x == a (or b, commuted): op2 x, b (in
//                                    place, 1 + 2v; INC / DEC for +-1, 1 + v);
//                                    else op x, a, b (1 + 3v)
//   t = cmp a, b; br_cond t          CMPBR a, b, target     1 + 2 operands + 2
//   t = addr_elem b, i, n; load/store [t]   LDE / STE       1 + 3v + 1
//   t = add p, #off; load/store [t]  LDO / STO [p + off]    1 + 2v + 1-2
//   call with 0-3 arguments          the count in the opcode: -1
// and an ADV (1 byte) for every 240 bytes.
namespace {
bool isCmp(ir::Op o) { return o >= ir::Op::CMP_EQ && o <= ir::Op::CMP_GEU; }
bool isAlu(ir::Op o) {
    switch (o) {
        case ir::Op::ADD: case ir::Op::SUB: case ir::Op::AND: case ir::Op::OR: case ir::Op::XOR:
        case ir::Op::SHL: case ir::Op::SHR: case ir::Op::LSL: case ir::Op::LSR: case ir::Op::ASR:
        case ir::Op::MUL: case ir::Op::MUL_U: case ir::Op::DIV: case ir::Op::DIV_U: case ir::Op::MOD:
        case ir::Op::MOD_U: return true;
        default: return false;
    }
}
bool commutes(ir::Op o) {
    return o == ir::Op::ADD || o == ir::Op::AND || o == ir::Op::OR || o == ir::Op::XOR ||
           o == ir::Op::MUL || o == ir::Op::MUL_U;
}
bool sameVreg(const ir::Operand& a, const ir::Operand& b) { return a.isVreg() && b.isVreg() && a.vregId == b.vregId; }
}  // namespace

static long pcodeSchemeB(const ir::Function& fn, long& instsOut) {
    const int vb = fn.nextVreg > 256 ? 2 : 1;
    std::map<uint32_t, int> uses;
    auto use = [&](const ir::Operand& o) { if (o.isVreg()) uses[o.vregId]++; };
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts) {
            use(in.src1); use(in.src2);
            if (in.op == ir::Op::STORE || in.op == ir::Op::BR_COND) {} // (dest is no use)
            for (const auto& a : in.args) use(a);
            for (const auto& ph : in.phiIncoming) use(ph.first);
        }
    long bytes = 6, insts = 0;
    for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
        const auto& insts_ = fn.blocks[bi].insts;
        const std::string next = bi + 1 < fn.blocks.size() ? fn.blocks[bi + 1].label : "";
        // the next instruction that is not a NOP
        auto nextOf = [&](size_t k) { size_t j = k + 1; while (j < insts_.size() && insts_[j].op == ir::Op::NOP) j++; return j; };
        for (size_t k = 0; k < insts_.size(); k++) {
            const auto& in = insts_[k];
            if (in.op == ir::Op::NOP || in.op == ir::Op::ASM_INLINE) continue;
            size_t j = nextOf(k);
            const ir::Inst* nx = j < insts_.size() ? &insts_[j] : nullptr;
            bool once = in.dest.isVreg() && uses[in.dest.vregId] == 1;
            // const + store
            if (in.op == ir::Op::CONST && once && nx && nx->op == ir::Op::STORE && sameVreg(nx->src1, in.dest)) {
                bytes += 1 + pcOperand(nx->src2, vb) + pcImm(in.src1); insts++; k = j; continue;
            }
            // op + store
            if (isAlu(in.op) && once && nx && nx->op == ir::Op::STORE && sameVreg(nx->src1, in.dest)) {
                const ir::Operand& x = nx->src2;
                const ir::Operand* other = nullptr;
                if (sameVreg(in.src1, x)) other = &in.src2;
                else if (commutes(in.op) && sameVreg(in.src2, x)) other = &in.src1;
                if (other && other->isImm() && other->immVal == 1 && (in.op == ir::Op::ADD || in.op == ir::Op::SUB))
                    bytes += 1 + vb;
                else if (other)
                    bytes += 1 + pcOperand(x, vb) + pcOperand(*other, vb);
                else
                    bytes += 1 + pcOperand(x, vb) + pcOperand(in.src1, vb) + pcOperand(in.src2, vb);
                insts++; k = j; continue;
            }
            // compare + br_cond
            if (isCmp(in.op) && once && nx && nx->op == ir::Op::BR_COND && sameVreg(nx->src1, in.dest)) {
                bool t = nx->src2.kind == ir::OperandKind::LABEL && nx->src2.name == next;
                bool f = nx->dest.kind == ir::OperandKind::LABEL && nx->dest.name == next;
                bytes += 1 + pcOperand(in.src1, vb) + pcOperand(in.src2, vb) + 2 + ((t || f) ? 0 : 3);
                insts += (t || f) ? 1 : 2; k = j; continue;
            }
            // addr_elem + load / store
            if (in.op == ir::Op::ADDR_ELEM && once && nx &&
                ((nx->op == ir::Op::LOAD && sameVreg(nx->src1, in.dest)) ||
                 (nx->op == ir::Op::STORE && sameVreg(nx->src2, in.dest)))) {
                const ir::Operand& v = nx->op == ir::Op::LOAD ? nx->dest : nx->src1;
                bytes += 1 + pcOperand(v, vb) + pcOperand(in.src1, vb) + pcOperand(in.src2, vb) + 1 + (in.elemOffset ? 2 : 0);
                insts++; k = j; continue;
            }
            // pointer + constant offset, then load / store
            if (in.op == ir::Op::ADD && in.resultType == ir::Type::PTR && once && in.src2.isImm() && nx &&
                ((nx->op == ir::Op::LOAD && sameVreg(nx->src1, in.dest)) ||
                 (nx->op == ir::Op::STORE && sameVreg(nx->src2, in.dest)))) {
                const ir::Operand& v = nx->op == ir::Op::LOAD ? nx->dest : nx->src1;
                bytes += 1 + pcOperand(v, vb) + pcOperand(in.src1, vb) + (in.src2.immVal >= 0 && in.src2.immVal < 256 ? 1 : 2);
                insts++; k = j; continue;
            }
            // otherwise as scheme A (calls with 0-3 arguments: the count in the opcode)
            long n = 1;
            switch (in.op) {
                case ir::Op::BR:
                    n = (in.src1.kind == ir::OperandKind::LABEL && in.src1.name == next) ? 0 : 3;
                    break;
                case ir::Op::BR_COND: {
                    bool t = in.src2.kind == ir::OperandKind::LABEL && in.src2.name == next;
                    bool f = in.dest.kind == ir::OperandKind::LABEL && in.dest.name == next;
                    n += pcOperand(in.src1, vb) + ((t || f) ? 2 : 4);
                    break;
                }
                case ir::Op::SWITCH:
                    n += pcOperand(in.src1, vb) + 3;
                    for (const auto& c : in.switchCases) n += pcImm(ir::Operand::imm(c.first, in.src1.type)) + 2;
                    break;
                case ir::Op::CALL: case ir::Op::CALL_VOID: case ir::Op::CALL_INDIRECT:
                    n += pcOperand(in.dest, vb) + (in.op == ir::Op::CALL_INDIRECT ? pcOperand(in.src1, vb) : 2) +
                         (in.args.size() <= 3 ? 0 : 1);
                    for (const auto& a : in.args) n += pcOperand(a, vb);
                    break;
                case ir::Op::PHI:
                    n += pcOperand(in.dest, vb) + 1;
                    for (const auto& [o, l] : in.phiIncoming) n += pcOperand(o, vb) + 2;
                    break;
                case ir::Op::FCONST: n += pcOperand(in.dest, vb) + 5; break;
                default:
                    n += pcOperand(in.dest, vb) + pcOperand(in.src1, vb) + pcOperand(in.src2, vb);
                    for (const auto& a : in.args) n += pcOperand(a, vb);
                    if (in.elemOffset) n += 2;
                    break;
            }
            if (n) insts++;
            bytes += n;
        }
    }
    bytes += bytes / 240;   // ADV
    instsOut = insts;
    return bytes;
}

void CodegenStage::writePcodeStats(const char* path) {
    std::vector<TrialResult> res;
    std::map<std::string, uint32_t> native;
    if (sizeTrial_({irOutput_}, res) && res.size() == 1) {
        std::map<std::string, uint32_t> start;
        std::set<uint32_t> starts;
        for (const auto& fn : irModule_->functions) {
            uint32_t at = UINT32_MAX;
            for (const std::string& n : {fn.name, fn.name + "__sac"}) {
                auto it = res[0].starts.find(n);
                if (it != res[0].starts.end()) at = std::min(at, it->second);
            }
            if (at != UINT32_MAX) { start[fn.name] = at; starts.insert(at); }
        }
        // (bounded by the next function's start only: a function's own
        // __sac entry and its exported labels are inside it)
        for (const auto& [n, at] : start) {
            auto next = starts.upper_bound(at);
            native[n] = (next == starts.end() ? res[0].textSize : *next) - at;
        }
    }
    std::ofstream out(path, std::ios::app);
    if (!out) return;
    for (const auto& fn : irModule_->functions) {
        if (fn.blocks.empty()) continue;
        const int vb = fn.nextVreg > 256 ? 2 : 1;
        long bytes = 6, insts = 0, asmCount = 0, labels = 0;
        std::map<std::string, std::pair<long, long>> perOp;   // "op/type" -> (count, bytes)
        std::map<std::string, size_t> blockAt;
        for (size_t b = 0; b < fn.blocks.size(); b++) blockAt[fn.blocks[b].label] = b;
        for (size_t b = 0; b < fn.blocks.size(); b++) {
            const std::string next = b + 1 < fn.blocks.size() ? fn.blocks[b + 1].label : "";
            for (const auto& in : fn.blocks[b].insts) {
                if (in.op == ir::Op::NOP) continue;
                insts++;
                long n = 1;
                switch (in.op) {
                    case ir::Op::ASM_INLINE: asmCount++; n = 0; break;
                    case ir::Op::BR:
                        n = (in.src1.kind == ir::OperandKind::LABEL && in.src1.name == next) ? 0 : 1 + 2;
                        if (n) labels++;
                        break;
                    case ir::Op::BR_COND: {
                        bool t = in.src2.kind == ir::OperandKind::LABEL && in.src2.name == next;
                        bool f = in.dest.kind == ir::OperandKind::LABEL && in.dest.name == next;
                        n += pcOperand(in.src1, vb) + ((t || f) ? 2 : 4);
                        labels += (t || f) ? 1 : 2;
                        break;
                    }
                    case ir::Op::SWITCH:
                        n += pcOperand(in.src1, vb) + 2 + 1;
                        for (const auto& c : in.switchCases) {
                            ir::Operand v = ir::Operand::imm(c.first, in.src1.type);
                            n += pcImm(v) + 2;
                        }
                        break;
                    case ir::Op::CALL: case ir::Op::CALL_VOID: case ir::Op::CALL_INDIRECT:
                        n += pcOperand(in.dest, vb) + (in.op == ir::Op::CALL_INDIRECT ? pcOperand(in.src1, vb) : 2) + 1;
                        for (const auto& a : in.args) n += pcOperand(a, vb);
                        break;
                    case ir::Op::PHI:
                        n += pcOperand(in.dest, vb) + 1;
                        for (const auto& [o, l] : in.phiIncoming) n += pcOperand(o, vb) + 2;
                        break;
                    case ir::Op::FCONST: n += pcOperand(in.dest, vb) + 5; break;
                    default:
                        n += pcOperand(in.dest, vb) + pcOperand(in.src1, vb) + pcOperand(in.src2, vb);
                        for (const auto& a : in.args) n += pcOperand(a, vb);
                        if (in.elemOffset) n += 2;
                        break;
                }
                bytes += n;
                std::ostringstream k;
                k << ir::opName(in.op);
                k << "/" << (int)in.resultType;
                auto& po = perOp[k.str()];
                po.first++;
                po.second += n;
            }
        }
        auto nat = native.find(fn.name);
        long instsB = 0, bytesB = pcodeSchemeB(fn, instsB);
        out << "F " << fn.name << " " << (nat != native.end() ? (long)nat->second : -1L) << " " << bytes << " "
            << insts << " " << fn.blocks.size() << " " << fn.nextVreg << " " << fn.paramTypes.size() << " "
            << asmCount << " " << labels << " " << bytesB << " " << instsB << "\n";
        for (const auto& [k, v] : perOp) out << "O " << k << " " << v.first << " " << v.second << "\n";
    }
}
