#pragma once
#include <string>
#include <chrono>
#include <memory>

// Hook point identifiers in the compilation pipeline
enum class HookPoint {
    PreParse,          // Before parsing begins
    PostLex,           // After lexing completes
    PostParse,         // After parsing completes
    PostConstFold,     // After constant folding
    PostFuncAnalysis,  // After function analysis
    PreOptSelect,      // Before optimization selection (decision point)
    PostInlineSelect,  // After inline selection
    PostCallGraph,     // After call graph analysis
    PostIRBuild,       // After IR building
    PreIROpt,          // Before IR optimization (decision point)
    PostCodeGen,       // After code generation
    PostAsmOpt         // After assembler optimization
};

// Compilation signal structure captures metrics at each hook point
struct CompilationSignal {
    HookPoint phase;
    double compileTimeSoFar;      // ms elapsed since compilation start

    // Code metrics
    int tokenCount;               // Lexer output
    int astNodeCount;             // AST size
    int functionCount;            // Total functions
    int localVariableCount;       // Local variables in scope
    int nestingDepth;             // Max nesting level

    // Optimization metrics
    int constantsFolded;          // Constants eliminated by folding
    int deadCodeRemoved;          // Dead code eliminated
    int functionsAnalyzed;        // Functions analyzed
    int inlineCandidates;         // Inline opportunities found

    // IR metrics
    int irNodeCount;              // IR instruction count
    int irBlockCount;             // Basic block count
    int branchDensity;            // Branches per 100 instructions
    int registerPressure;         // Estimated register usage

    // Assembly metrics
    int assemblySize;             // Generated assembly size in bytes
    int optimizationsPassed;      // Passes completed

    // Compilation state
    bool isOptimizing;            // Current optimization state
    bool hasErrors;               // Error encountered
    std::string currentPhase;     // Human-readable phase name
};

// Hook decision structure - output of hook invocation
struct HookDecision {
    bool changed;                 // Whether decision differs from previous
    bool enableOptimizations;     // Should we enable optimizations?
    bool enableIROpts;            // Should we enable IR optimizations?
    bool enableInlining;          // Should we enable inlining?
    bool skipExpensivePasses;     // Skip expensive optimization passes
    double estimatedTimeRemaining; // Remaining compilation budget in ms
    std::string rationale;        // Reason for decision
};

// Hook statistics for profiling
struct HookStatistics {
    HookPoint phase;
    int invocationCount;
    int decisionsChanged;
    double averageDecisionTimeMs;
    double totalTimeMs;

    double getAverageTime() const {
        return (invocationCount > 0) ? totalTimeMs / invocationCount : 0.0;
    }
};

// Helper to convert HookPoint enum to string
inline std::string hookPointToString(HookPoint hp) {
    switch (hp) {
        case HookPoint::PreParse:        return "PreParse";
        case HookPoint::PostLex:         return "PostLex";
        case HookPoint::PostParse:       return "PostParse";
        case HookPoint::PostConstFold:   return "PostConstFold";
        case HookPoint::PostFuncAnalysis: return "PostFuncAnalysis";
        case HookPoint::PreOptSelect:    return "PreOptSelect";
        case HookPoint::PostInlineSelect: return "PostInlineSelect";
        case HookPoint::PostCallGraph:   return "PostCallGraph";
        case HookPoint::PostIRBuild:     return "PostIRBuild";
        case HookPoint::PreIROpt:        return "PreIROpt";
        case HookPoint::PostCodeGen:     return "PostCodeGen";
        case HookPoint::PostAsmOpt:      return "PostAsmOpt";
        default:                         return "Unknown";
    }
}

// Helper to create default decision (no change, continue as-is)
inline HookDecision makeDefaultDecision(const CompilationSignal& sig) {
    return HookDecision{
        false,                      // changed
        sig.isOptimizing,          // enableOptimizations
        sig.isOptimizing,          // enableIROpts
        sig.isOptimizing,          // enableInlining
        false,                      // skipExpensivePasses
        -1.0,                       // estimatedTimeRemaining (unknown)
        "default (no change)"       // rationale
    };
}
