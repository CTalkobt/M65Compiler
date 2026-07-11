#pragma once
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "AssemblerTypes.hpp"

class AssemblerParser;
struct O45File;

// Phase 5: External function attributes for inter-TU optimization
struct ExternalFuncInfo {
    uint8_t regMask;
    bool isLeaf;
};

// Named optimization flags: per-pass control via -P<Name> / -PNo<Name>
struct OptimizationFlags {
    // IR-level optimizations (compiler)
    bool strengthReduction = true;    // Replace MUL/DIV by pow2 with bit shifts
    bool algebraicSimplify = true;    // Eliminate identity/annihilator patterns
    bool typeNarrowing = true;        // Replace wide ops with I8 when truncated
    bool branchFold = true;           // Branch folding and dead block elimination
    bool cse = true;                  // Common Subexpression Elimination
    bool licm = true;                 // Loop-Invariant Code Motion
    bool copyChains = true;           // Resolve transitive copy chains
    bool addrElemFusion = true;       // Merge ADDR_ELEM into LOAD/STORE

    // Assembler-level optimizations (assembler)
    bool jsrRelocate = true;          // JSR → BSR relocation for position-independent code
    bool tailCall = true;             // JSR + RTS → JMP
    bool branchInvert = true;         // Invert branch + BRA → single inverted branch
    bool jmpBra = true;               // JMP → BRA for backward branches
    bool noOpBra = true;              // Eliminate BRA to next instruction
    bool cmpElimination = true;       // Eliminate redundant compare instructions
    bool redundantLoad = true;        // Eliminate redundant loads (reverse store-forwarding)
    bool deadStore = true;            // Eliminate dead stores
    bool tailDedup = true;            // Deduplicate tail code sequences
    bool preserveXSP = true;          // Preserve X register across calls (optimize TSX)
    bool seqExtract = true;           // Sequential extract optimization
    bool storeLoadPair = true;        // Optimize store-load pairs
    bool fcmpOpt = true;              // Floating-point compare optimization
    bool tsxRedundant = true;         // Eliminate redundant TSX instructions

    // Constructor to reset all flags based on optimization level
    static OptimizationFlags fromLevel(int level) {
        OptimizationFlags flags;
        // Level 0: disable all optimizations
        if (level <= 0) {
            flags.strengthReduction = false;
            flags.algebraicSimplify = false;
            flags.typeNarrowing = false;
            flags.branchFold = false;
            flags.cse = false;
            flags.licm = false;
            flags.copyChains = false;
            flags.addrElemFusion = false;
            flags.jsrRelocate = false;
            flags.tailCall = false;
            flags.branchInvert = false;
            flags.jmpBra = false;
            flags.noOpBra = false;
            flags.cmpElimination = false;
            flags.redundantLoad = false;
            flags.deadStore = false;
            flags.tailDedup = false;
            flags.preserveXSP = false;
            flags.seqExtract = false;
            flags.storeLoadPair = false;
            flags.fcmpOpt = false;
            flags.tsxRedundant = false;
        }
        // Level 1+: basic optimizations (all enabled by default)
        // Level 2+: default (all enabled)
        // Level 3: aggressive (all enabled, may be combined with -P flags)
        return flags;
    }
};

class AssemblerOptimizer {
public:
    static bool optimize(AssemblerParser* parser, bool verbose = false, bool traceMachState = false);

    // Phase 5: Load clobber info from external .o45 files for inter-TU optimization
    static bool optimizeWithExternalObjects(
        AssemblerParser* parser,
        const std::vector<std::string>& objectFiles,
        bool verbose = false,
        bool traceMachState = false
    );

private:
    // Internal optimization with optional external function info
    static bool optimizeInternal(
        AssemblerParser* parser,
        const std::map<std::string, ExternalFuncInfo>* externalFuncs,
        bool verbose,
        bool traceMachState,
        int optimizationLevel = 2
    );
};
