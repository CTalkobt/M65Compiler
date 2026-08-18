#pragma once
#include <vector>
#include <string>
#include <set>
#include <map>
#include <cstdint>
#include <iostream>
#include "O45Types.hpp"
#include "O45Reader.hpp"
#include "O45Writer.hpp"
#include "O45Archive.hpp"
#include "O45IRSerializer.hpp"

// Decodes a raw relocation byte stream (as stored in O45File::textRelocs/dataRelocs)
// back into a list of high-level O45Reloc entries.
class O45RelocDecoder {
public:
    static std::vector<O45Reloc> decode(const std::vector<uint8_t>& raw);
};

// Linker for .o45 relocatable object files.
//
// Usage:
//   O45Linker linker;
//   linker.addObject("main.o45", mainObj);
//   linker.addObject("math.o45", mathObj);
//   linker.setTextBase(0x2000);
//   std::string err;
//   auto binary = linker.link(err);
//
class O45Linker {
public:
    // Parameter specialization: tracks parameters that are ALWAYS constant
    struct SpecializedParam {
        bool isConstant = false;
        int64_t value = 0;
    };

    // Add an object file to the link. Order matters for segment layout.
    void addObject(const std::string& filename, const O45File& obj);

    // Add a library archive. Members are selectively pulled in to resolve
    // undefined symbols. May be called multiple times; resolution iterates
    // until no new symbols are satisfied.
    void addLibrary(const std::string& filename, const Ar45Archive& lib);

    // Set the base addresses for merged segments.
    // If not set, text starts at 0x0000.
    void setTextBase(uint32_t addr) { textBase_ = addr; }
    void setDataBase(uint32_t addr) { dataBase_ = addr; dataBaseSet_ = true; }
    void setBssBase(uint32_t addr)  { bssBase_ = addr; bssBaseSet_ = true; }
    void setZpBase(uint32_t addr)   { zpBase_ = addr; zpBaseSet_ = true; }

    // Link all objects and return the final binary.
    // Returns empty vector on error (with errorMsg set).
    // If isPrg is true, prepends a 2-byte load address header.
    std::vector<uint8_t> link(std::string& errorMsg, bool isPrg = false);

    // After a successful link, retrieve the global symbol map (name -> final address).
    const std::map<std::string, uint32_t>& getSymbolMap() const { return globalSymbols_; }

    // After a successful link, retrieve function attributes (name -> O45FuncAttr).
    const std::map<std::string, O45FuncAttr>& getFuncAttrs() const { return funcAttrs_; }

    // After a successful link, retrieve the call graph (caller -> set of callees).
    const std::map<std::string, std::set<std::string>>& getCallGraph() const { return callGraph_; }

    // After a successful link, retrieve transitive clobber sets (name -> merged O45FuncAttr).
    const std::map<std::string, O45FuncAttr>& getTransitiveClobbers() const { return transitiveClobbers_; }

    // After a successful link, retrieve parameter specialization info.
    // Maps function name → parameter index → (isConstant, value)
    const std::map<std::string, std::map<int, SpecializedParam>>& getSpecializedParams() const {
        return specializedParams_;
    }

    // Phase 52: After a successful link, retrieve function specialization analysis
    const std::map<std::string, SpecializationAnalysis>& getSpecializationAnalysis() const {
        return specializationAnalysis_;
    }

    // Phase 53: Get specializations recommended for generation
    // Returns map of function name → patterns to specialize
    std::map<std::string, std::vector<SpecializationPattern>> getRecommendedSpecializations() const;

    // Phase 53: Write specialization report for compiler/debugging
    void writeSpecializationReport(std::ostream& out) const;

    // Phase 54: Get call routing analysis for all functions
    const std::map<std::string, CallRoutingAnalysis>& getCallRoutingAnalysis() const {
        return callRoutingAnalysis_;
    }

    // Phase 54: Write call routing report for debugging
    void writeCallRoutingReport(std::ostream& out) const;

    // Phase 55: Get inlining candidates for a function
    std::vector<InliningCandidate> getInliningCandidates(const std::string& funcName) const;

    // Phase 55: Write inlining report for debugging
    void writeInliningReport(std::ostream& out) const;

    // Phase 56: Generate dispatcher stubs for multi-specialization cases
    void generateDispatchers();

    // Phase 56: Get dispatcher analysis for a function
    const std::map<std::string, DispatcherAnalysis>& getDispatcherAnalysis() const {
        return dispatcherAnalysis_;
    }

    // Phase 56: Write dispatcher report for debugging
    void writeDispatcherReport(std::ostream& out) const;

    // Phase 57: Emit dispatcher assembly code
    std::string emitDispatcherAssembly() const;

    // Phase 57: Generate assembly for a specific dispatcher stub
    std::string emitDispatcherStub(const DispatcherStub& stub) const;

    // Phase 58: Integrate dispatcher assembly into link output
    void integrateDispatcherAssembly();

    // Phase 58: Get dispatcher assembly output for emission
    const std::string& getDispatcherAssemblyOutput() const {
        return dispatcherAssemblyOutput_;
    }

    // Phase 59: Get count of dispatcher stubs emitted
    int getDispatcherStubsEmitted() const {
        return dispatcherStubsEmitted_;
    }

    // Phase 59: Emit dispatcher assembly to output
    void emitDispatcherAssemblyOutput();

    // Phase 60: Write dispatcher assembly to file
    bool writeDispatcherAssemblyFile(const std::string& filepath, std::string& errorMsg);

    // Phase 60: Get dispatcher assembly file path
    const std::string& getDispatcherAssemblyFilePath() const {
        return dispatcherAssemblyFilePath_;
    }

    // Phase 61: Assemble dispatcher assembly file with ca45
    bool assembleDispatcherFile(const std::string& ca45Path, std::string& errorMsg);

    // Phase 61: Get dispatcher object file path
    const std::string& getDispatcherObjectFilePath() const {
        return dispatcherObjectFilePath_;
    }

    // Phase 61: Get count of dispatchers assembled
    int getDispatchersAssembled() const {
        return dispatchersAssembled_;
    }

    // Phase 62: Re-link with dispatcher object
    bool relinkWithDispatcher(std::string& errorMsg, bool isPrg = false);

    // Phase 62: Get dispatcher linking status
    bool isDispatcherLinked() const {
        return dispatcherLinked_;
    }

    // Phase 62: Get dispatcher binary output
    const std::vector<uint8_t>& getDispatcherBinary() const {
        return dispatcherBinary_;
    }

    // Phase 63: Verify dispatcher symbol resolution
    bool verifyDispatcherSymbols(std::string& report);

    // Phase 63: Get dispatcher symbol verification count
    int getDispatcherSymbolsVerified() const {
        return dispatcherSymbolsVerified_;
    }

    // Phase 63: Check if all dispatcher symbols resolved
    bool areAllDispatcherSymbolsResolved() const {
        return allDispatcherSymbolsResolved_;
    }

    // Phase 65: Generate dispatcher execution report
    std::string generateDispatcherReport();

    // Phase 66: Calculate dispatcher optimization metrics
    struct OptimizationMetrics {
        int estimatedCodeSavings = 0;        // Bytes saved by specialization
        float compressionRatio = 0.0f;       // (original - optimized) / original
        int dispatcherOverhead = 0;          // Dispatcher stub code size
        float netSavings = 0.0f;             // Total - overhead
        int callOptimizationCount = 0;       // Calls that benefit from routing
        float optimizedCallsPercent = 0.0f;  // Percentage of calls optimized
    };
    OptimizationMetrics calculateOptimizationMetrics();

    // Phase 67: Dispatcher performance benchmarking
    struct BenchmarkResult {
        std::string programName;             // Program being benchmarked
        int baselineSize = 0;                // Size without optimizations
        int optimizedSize = 0;               // Size with dispatcher + specializations
        int codeSizeReduction = 0;           // Bytes saved
        float compressionPercent = 0.0f;     // Percentage reduction
        int dispatcherCodeSize = 0;          // Bytes for dispatcher stubs
        int specializedVersionsCount = 0;    // Count of specialized versions
        int routableCallsCount = 0;          // Calls that use specialization
        float overheadRatio = 0.0f;          // Dispatcher overhead / savings ratio
        bool worthOptimizing = false;        // Recommendation (overhead < 50% of savings)
    };
    BenchmarkResult generateBenchmarkResult(const std::string& programName);

    // Phase 68: Generate benchmark report with recommendations
    std::string generateBenchmarkReport(const BenchmarkResult& result);

    // Phase 69: Multi-program benchmark comparison
    struct BenchmarkComparison {
        std::vector<BenchmarkResult> results;     // All benchmark results
        float avgCompressionPercent = 0.0f;       // Average compression across programs
        float avgOverheadRatio = 0.0f;            // Average overhead ratio
        int totalBaselineSize = 0;                // Total baseline bytes
        int totalOptimizedSize = 0;               // Total optimized bytes
        int totalSavings = 0;                     // Total bytes saved
        int bestProgram = -1;                     // Index of best compression
        int worstProgram = -1;                    // Index of worst compression
        int recommendedCount = 0;                 // Programs recommended for optimization
        float recommendationRatio = 0.0f;         // Percentage recommended
    };
    BenchmarkComparison compareBenchmarks(const std::vector<BenchmarkResult>& results);
    std::string generateComparisonReport(const BenchmarkComparison& comparison);

    // Phase 70: Export benchmark results to files
    bool exportBenchmarkResult(const BenchmarkResult& result, const std::string& filepath);
    bool exportComparisonReport(const BenchmarkComparison& comparison, const std::string& filepath);
    std::string exportBenchmarkJSON(const BenchmarkResult& result);
    std::string exportComparisonJSON(const BenchmarkComparison& comparison);

    // Phase 71: Dispatcher pipeline final status report
    std::string generateDispatcherPipelineSummary();

    // Phase 72: Performance regression detection
    struct RegressionAnalysis {
        std::string programName;             // Program name
        float baselineCompression = 0.0f;    // Previous compression %
        float currentCompression = 0.0f;     // Current compression %
        float compressionDelta = 0.0f;       // Change (positive = improvement)
        int baselineCodeSize = 0;            // Previous code size
        int currentCodeSize = 0;             // Current code size
        int codeSizeDelta = 0;               // Change in bytes
        bool isRegression = false;           // Compression got worse
        float regressionPercent = 0.0f;      // Magnitude of regression
        std::string status;                  // "IMPROVED", "REGRESSED", "STABLE"
    };
    RegressionAnalysis detectRegression(const BenchmarkResult& baseline,
                                        const BenchmarkResult& current);
    std::string generateRegressionReport(const RegressionAnalysis& analysis);

    // Phase 73: Automated optimization tuning
    struct TuningRecommendation {
        std::string parameterName;           // Parameter to tune
        std::string currentValue;            // Current setting
        std::string recommendedValue;        // Suggested setting
        float expectedImprovement = 0.0f;    // Expected % improvement
        std::string rationale;               // Why this tuning helps
        int priority = 0;                    // 1=high, 2=medium, 3=low
    };
    struct TuningAnalysis {
        std::vector<TuningRecommendation> recommendations;  // All recommendations
        float totalExpectedImprovement = 0.0f;              // Sum of improvements
        int highPriorityCount = 0;                          // Tunings marked high priority
        bool shouldTune = false;                            // Recommend tuning
    };
    TuningAnalysis analyzeTuningOpportunities(const BenchmarkResult& result,
                                              const OptimizationMetrics& metrics);
    std::string generateTuningReport(const TuningAnalysis& analysis);

    // Phase 74: Cross-program optimization orchestration
    struct OptimizationPattern {
        std::string patternName;             // Pattern identifier
        int occurrenceCount = 0;             // How many programs have it
        float averageImprovement = 0.0f;     // Average benefit across programs
        std::vector<std::string> programs;   // Programs exhibiting pattern
        std::string recommendation;          // What to do about this pattern
    };
    struct CrossProgramAnalysis {
        std::vector<OptimizationPattern> commonPatterns;    // Shared patterns
        float averageCompressionPercent = 0.0f;             // Average across all
        int programsAnalyzed = 0;                           // Total programs
        float aggregatedSavingsPotential = 0.0f;            // Total bytes saveable
        std::string deploymentStrategy;                     // Unified strategy
        int patternsFound = 0;                              // Number of patterns
    };
    CrossProgramAnalysis orchestrateOptimizations(const std::vector<BenchmarkResult>& results,
                                                  const std::vector<OptimizationMetrics>& metrics);
    std::string generateOrchestrationReport(const CrossProgramAnalysis& analysis);

    // Phase 75: Runtime profiling integration
    struct RuntimeProfile {
        std::string programName;             // Program being profiled
        float actualCompressionPercent = 0.0f;  // Measured compression
        float estimatedCompressionPercent = 0.0f;  // Predicted compression
        float compressionAccuracy = 0.0f;    // Accuracy % (actual/estimated)
        int actualCodeSize = 0;              // Measured code size
        int executionTimeMs = 0;             // Execution time in milliseconds
        float performanceGainPercent = 0.0f; // Measured performance gain
        bool validatesEstimate = false;      // Estimate was accurate
        std::string feedbackAction;          // Improvement recommendation
    };
    RuntimeProfile integrateRuntimeProfile(const BenchmarkResult& estimate,
                                          int actualCodeSize, int executionTimeMs);
    std::string generateProfileReport(const RuntimeProfile& profile);

    // Query if a specific parameter is specialized (constant)
    bool isParameterSpecialized(const std::string& funcName, int paramIdx, int64_t& outValue) const {
        auto it = specializedParams_.find(funcName);
        if (it == specializedParams_.end()) return false;
        auto pit = it->second.find(paramIdx);
        if (pit == it->second.end()) return false;
        if (!pit->second.isConstant) return false;
        outValue = pit->second.value;
        return true;
    }

    // Set warning output stream (default: stderr). Set to nullptr to suppress.
    void setWarningStream(std::ostream* os) { warnStream_ = os; }

    // Calling convention thunk mode
    enum ThunkMode { THUNK_AUTO, THUNK_WARN, THUNK_ERROR };
    void setThunkMode(ThunkMode m) { thunkMode_ = m; }

    // Write a detailed linker map to a stream. Call after link().
    void writeMap(std::ostream& out) const;

    // Write a JSON debug line map to a stream. Call after link().
    void writeLineMap(std::ostream& out) const;
    bool hasLineMap() const { return !mergedLineMap_.empty(); }

    struct MergedLineEntry {
        uint32_t address;
        std::string file;
        int line;
    };

private:
    struct InputObject {
        std::string filename;
        O45File obj;
        // Per-object offsets within the merged segments
        uint32_t textOffset = 0;
        uint32_t dataOffset = 0;
        uint32_t bssOffset = 0;
        uint32_t zpOffset = 0;

        // Sub-segment remapping for text body reordering.
        // Maps [srcOffset, srcLen] in original text body → destOffset in merged text.
        struct TextRemap {
            uint32_t srcOffset;   // offset within this object's original textBody
            uint32_t srcLen;      // length of this sub-segment
            uint32_t destOffset;  // offset within the merged text body
            std::string name;     // sub-segment name ("init", "code", etc.)
        };
        std::vector<TextRemap> textRemaps;

        // Translate an offset within this object's original text body
        // to an offset within the merged text body.
        uint32_t remapTextOffset(uint32_t origOff) const {
            for (const auto& r : textRemaps) {
                if (origOff >= r.srcOffset && origOff < r.srcOffset + r.srcLen) {
                    return r.destOffset + (origOff - r.srcOffset);
                }
            }
            // No remap — use simple textOffset (single-segment object)
            return textOffset + origOff;
        }
    };

    std::vector<InputObject> objects_;

    struct LibraryEntry {
        std::string filename;
        Ar45Archive lib;
    };
    std::vector<LibraryEntry> libraries_;

    uint32_t textBase_ = 0x0000;
    uint32_t dataBase_ = 0;  bool dataBaseSet_ = false;
    uint32_t bssBase_ = 0;   bool bssBaseSet_ = false;
    uint32_t zpBase_ = 0;    bool zpBaseSet_ = false;

    // Merged segment bodies
    std::vector<uint8_t> mergedText_;
    std::vector<uint8_t> mergedData_;
    uint32_t mergedBssLen_ = 0;
    uint32_t mergedZpLen_ = 0;

    // Global symbol table: name -> final absolute address
    std::map<std::string, uint32_t> globalSymbols_;
    // Track which file defined each symbol (for error messages)
    std::map<std::string, std::string> symbolSource_;
    // Track weak flag per symbol
    std::map<std::string, bool> symbolWeak_;
    // Track segment ID per symbol (for cross-segment overlap detection)
    std::map<std::string, uint8_t> symbolSegment_;

    // Function attribute map: function name -> O45FuncAttr
    std::map<std::string, O45FuncAttr> funcAttrs_;

    // Call graph: function name -> set of callee names
    std::map<std::string, std::set<std::string>> callGraph_;

    // Transitive clobber sets: function name -> merged clobbers through call chain
    std::map<std::string, O45FuncAttr> transitiveClobbers_;

    // Calling convention mismatch errors
    std::vector<std::string> convErrors_;

    // Merged debug line map
    std::vector<MergedLineEntry> mergedLineMap_;

    // Warning stream (nullptr to suppress)
    std::ostream* warnStream_ = &std::cerr;

    // Thunk mode and generated thunks
    ThunkMode thunkMode_ = THUNK_AUTO;
    // Maps callee name → thunk address (for mismatched call sites)
    std::map<std::string, uint32_t> thunkAddresses_;
    // Per-call-site override: (objectIdx, relocIdx) → thunk target address
    std::map<std::pair<int,int>, uint32_t> callSiteOverrides_;

    // Phase 2: Static allocation register (AR) base addresses
    // Maps SAC function name → final BSS-relative AR base address
    std::map<std::string, uint32_t> arBaseAddresses_;
    // Tracks which functions are address-taken (can't be colored/overlapped)
    std::set<std::string> addressTakenFunctions_;

    // Phase 4: Parameter specialization
    // Maps function name → parameter index → (is_constant, constant_value)
    // Tracks which parameters are TRULY constant across all call sites
    std::map<std::string, std::map<int, SpecializedParam>> specializedParams_;

    // Phase 50: IR Metadata analysis
    // Merged IR metadata from all objects (across compilation units)
    // Maps function name → O45IRFunction with cross-file call information
    std::map<std::string, O45IRFunction> mergedIRFunctions_;

    // Phase 52: Specialization analysis results
    // Maps function name → specialization analysis (patterns, frequencies, profitability)
    std::map<std::string, SpecializationAnalysis> specializationAnalysis_;

    // Phase 54: Call routing analysis
    // Maps function name → call routing information for specialization dispatch
    std::map<std::string, CallRoutingAnalysis> callRoutingAnalysis_;

    // Phase 55: Inlining analysis
    // Maps calling site → inlining candidates for cross-module optimization
    std::map<std::string, InliningAnalysis> inliningAnalysis_;

    // Phase 56: Dispatcher generation analysis
    // Maps function name → dispatcher stub information for multi-specialization routing
    std::map<std::string, DispatcherAnalysis> dispatcherAnalysis_;

    // Phase 58: Dispatcher assembly output
    // Generated assembly code for dispatcher stubs ready to emit
    std::string dispatcherAssemblyOutput_;

    // Phase 59: Dispatcher output tracking
    // Count of dispatcher stubs emitted to output
    int dispatcherStubsEmitted_ = 0;

    // Phase 60: Dispatcher file output
    // Path to dispatcher assembly file written
    std::string dispatcherAssemblyFilePath_;

    // Phase 61: Dispatcher object file
    // Path to dispatcher object file (.o45) after assembly
    std::string dispatcherObjectFilePath_;
    // Count of dispatchers assembled
    int dispatchersAssembled_ = 0;

    // Phase 62: Dispatcher re-linking
    // Whether dispatcher has been linked into final binary
    bool dispatcherLinked_ = false;
    // Binary output after dispatcher linking
    std::vector<uint8_t> dispatcherBinary_;

    // Phase 63: Symbol verification
    // Number of dispatcher symbols verified
    int dispatcherSymbolsVerified_ = 0;
    // Whether all dispatcher symbols resolved
    bool allDispatcherSymbolsResolved_ = false;

    bool resolveLibraries(std::string& errorMsg);
    bool layoutSegments(std::string& errorMsg);
    bool resolveSymbols(std::string& errorMsg);
    bool applyRelocations(std::string& errorMsg);
    void buildFuncAttrs();
    void buildCallGraph();
    void computeTransitiveClobbers();
    void analyzeConstantParameters();  // Cross-file parameter analysis from .param_const metadata
    void analyzeIRMetadata();           // Phase 50: Extract constant parameters from embedded IR
    void analyzeSpecializations();      // Phase 52: Analyze profitable specialization patterns
    void analyzeCallRouting();          // Phase 54: Analyze call site routing to specializations
    void analyzeInlining();             // Phase 55: Analyze cross-module inlining opportunities
    BenchmarkResult calculateBenchmarkMetrics(const std::string& programName);  // Phase 67: Calculate benchmark metrics
    std::string formatBenchmarkReport(const BenchmarkResult& result);  // Phase 68: Format report
    BenchmarkComparison aggregateBenchmarks(const std::vector<BenchmarkResult>& results);  // Phase 69: Aggregate metrics
    std::string formatComparisonReport(const BenchmarkComparison& comparison);  // Phase 69: Format comparison
    bool writeBenchmarkFile(const BenchmarkResult& result, const std::string& filepath);  // Phase 70: Write benchmark file
    bool writeComparisonFile(const BenchmarkComparison& comparison, const std::string& filepath);  // Phase 70: Write comparison file
    std::string createPipelineSummary();  // Phase 71: Create pipeline summary
    RegressionAnalysis analyzeRegressions(const BenchmarkResult& baseline, const BenchmarkResult& current);  // Phase 72: Analyze regressions
    std::string formatRegressionReport(const RegressionAnalysis& analysis);  // Phase 72: Format regression report
    TuningAnalysis optimizeTuningParameters(const BenchmarkResult& result, const OptimizationMetrics& metrics);  // Phase 73: Optimize tuning
    std::string formatTuningReport(const TuningAnalysis& analysis);  // Phase 73: Format tuning report
    CrossProgramAnalysis analyzePatterns(const std::vector<BenchmarkResult>& results, const std::vector<OptimizationMetrics>& metrics);  // Phase 74: Analyze patterns
    std::string formatOrchestrationReport(const CrossProgramAnalysis& analysis);  // Phase 74: Format orchestration report
    RuntimeProfile collectProfileData(const BenchmarkResult& estimate, int actualSize, int execMs);  // Phase 75: Collect profile
    std::string formatProfileReport(const RuntimeProfile& profile);  // Phase 75: Format profile report
    void emitDiagnostics();
    void verifyStaticAllocSafety();  // Verify SAC (static allocation convention) constraints
    void validateSACParameters();     // Phase 3: Validate SAC parameter metadata
    void colorStaticAllocRegisters();  // Phase 2: Assign overlapping AR addresses via graph coloring
    void patchStaticAllocAddresses();  // Phase 3: Apply computed AR addresses to symbol table
    void generateThunks();
    void mergeLineMaps();

    // Get the final base address for a segment ID
    uint32_t segmentBase(O45Segment seg) const;

    // Apply relocations from one object's reloc table to a merged segment body
    bool applyRelocs(const std::vector<O45Reloc>& relocs,
                     std::vector<uint8_t>& body,
                     uint32_t bodyBase,
                     uint32_t objOffset,
                     const InputObject& input,
                     std::string& errorMsg,
                     int objIdx = -1);
};
