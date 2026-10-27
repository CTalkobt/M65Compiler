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

    // Specialization pattern: vector of constant parameter values
    using SpecializationPattern = std::vector<int64_t>;

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
    // Address of the image's first byte: the text base, or the data base
    // when -d puts the data below the text
    uint32_t imageBase() const { return imageBase_; }
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

    // Phase 4.3: After a successful link, retrieve aggregated IPO hints from all objects
    // Returns merged IPO hints with global call counts and optimization metadata
    const O45IPOHints& getAggregatedIPOHints() const {
        return aggregatedIPOHints_;
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
    void setSacOverlay(bool on) { sacOverlayOn_ = on; }
    // Drop pulled library members nothing live references (default on)
    void setGcObjects(bool on) { gcObjectsOn_ = on; }
    // --gc-inputs: the input objects are trimmed too (roots: the startup
    // code, .keep sections, -u symbols)
    void setGcInputs(bool on) { gcInputs_ = on; }
    void addKeepSymbol(const std::string& s) { keepSymbols_.push_back(s); }
    const std::vector<std::string>& discardedObjects() const { return discardedObjects_; }
    const std::vector<std::string>& discardedSectionNames() const { return discardedSectionNames_; }
    // Byte crt0 fills BSS with (__bss_fill, unless an object defines it)
    void setBssFill(uint8_t v) { bssFill_ = v; }
    // The CPU stack's share below __stack_top: the heap ends there
    // (__stack_size, unless an object defines it)
    void setStackSize(uint16_t v) { stackSize_ = v; }
    // --section NAME=ADDR: the address of a __section variables' section
    void setSectionAddr(const std::string& name, uint32_t addr) { sectionAddrOpt_[name] = addr; }

    // Code overlays (cc45 #pragma cc45 overlay(name)): where their images
    // are kept in far memory: ranges (first, last byte), each in one bank,
    // the banks consecutive (default $50000-$5FFFF), one file per range
    // used; the first file's name the program loads them from (PETSCII; the
    // others count its last character up), and the files' bytes after
    // link() (a 2-byte load address, then the images; none without overlays)
    void setOverlayStore(uint32_t a) { ovlStores_ = {{a, a | 0xFFFF}}; }
    void setOverlayStores(const std::vector<std::pair<uint32_t, uint32_t>>& r) { ovlStores_ = r; }
    void setOverlayStaging(int bank) { ovlStaging_ = bank; }
    // Mapped overlays: run in a window of 8 KB blocks mapped to them (MAP),
    // not copied into a region
    void setOverlayMap(bool on) { ovlMap_ = on; }
    // write the program-wide choice of base-page block variables (#304)
    void setZpPlanOut(const std::string& path) { zpPlanOut_ = path; }
    // overlay call stubs of 21 bytes, a few cycles faster than the 6-byte ones (#332)
    void setOverlayFastStubs(bool on) { ovlFastStubs_ = on; }
    void setOverlayFileName(const std::string& n) { ovlFileName_ = n; }
    const std::vector<std::vector<uint8_t>>& overlayFiles() const { return ovlFiles_; }
    const std::vector<uint8_t>& overlayDescription() const { return ovlDesc_; }   // the .ovd file
    bool hasOverlays() const { return !ovlNames_.empty(); }

    // Write a detailed linker map to a stream. Call after link().
    void writeMap(std::ostream& out) const;

    // Write a JSON debug line map to a stream. Call after link().
    // The program's debug information (#202), JSON: ln45 -g, <program>.dbg
    void writeDebugFile(std::ostream& out, const std::string& program) const;
    bool hasLineMap() const { return !mergedLineMap_.empty(); }

    struct MergedLineEntry {
        uint32_t address;
        std::string file;
        int line;
        int ovl = 0;          // the code overlay it is in (its address is where it runs)
    };

private:
    struct InputObject {
        std::string filename;
        O45File obj;
        // Per-object offsets within the merged segments
        uint32_t textOffset = 0;    // (in its overlay's text when ovl > 0)
        uint32_t dataOffset = 0;
        uint32_t bssOffset = 0;
        uint32_t zpOffset = 0;

        // Sub-segment remapping for text body reordering.
        // Maps [srcOffset, srcLen] in original text body → destOffset in merged text.
        struct TextRemap {
            uint32_t srcOffset;   // offset within this object's original textBody
            uint32_t srcLen;      // length of this sub-segment
            uint32_t destOffset;  // offset within the merged text body (or its overlay's text)
            std::string name;     // sub-segment name ("init", "code", etc.)
            int ovl = 0;          // the code overlay it went to (1..n), 0: resident
            int sect = -1;        // a __section function's chunk: the section's code (sectCode_), -1: none
        };
        std::vector<TextRemap> textRemaps;
        int ovl = 0;                // its code overlay (1..n), 0: resident

        // The code overlay holding an offset of this object's text: its
        // chunk's (__far code: an automatic overlay), else the object's
        int ovlAt(uint32_t origOff) const {
            for (const auto& r : textRemaps)
                if (origOff >= r.srcOffset && origOff < r.srcOffset + r.srcLen) return r.ovl;
            return ovl;
        }

        // The section code holding an offset of this object's text (-1: none)
        int sectAt(uint32_t origOff) const {
            for (const auto& r : textRemaps)
                if (origOff >= r.srcOffset && origOff < r.srcOffset + r.srcLen) return r.sect;
            return -1;
        }

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

        // BSS placement: [srcOffset, srcOffset+srcLen) of this object's BSS
        // → destOffset in the merged BSS. Empty: the BSS is one piece at
        // bssOffset. SAC blocks moved into the overlay area are pieces of
        // their own; bssKeptLen is what stays at bssOffset.
        struct BssPiece { uint32_t srcOffset, srcLen, destOffset; bool sac; };
        std::vector<BssPiece> bssRemaps;
        uint32_t bssKeptLen = 0;

        uint32_t remapBssOffset(uint32_t origOff) const {
            for (const auto& r : bssRemaps)
                if (origOff >= r.srcOffset && origOff < r.srcOffset + r.srcLen)
                    return r.destOffset + (origOff - r.srcOffset);
            // the end of a piece (a pointer just past a variable)
            for (const auto& r : bssRemaps)
                if (!r.sac && origOff == r.srcOffset + r.srcLen)
                    return r.destOffset + r.srcLen;
            return bssOffset + origOff;
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
    uint32_t imageBase_ = 0;
    uint32_t bssBase_ = 0;   bool bssBaseSet_ = false;
    uint32_t dataAlign_ = 1, bssAlign_ = 1;  // largest cc45.align.data/.bss of any object
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
    uint8_t bssFill_ = 0;
    bool gcObjectsOn_ = true;
    bool gcInputs_ = false;
    std::vector<std::string> keepSymbols_;
    std::vector<std::string> discardedObjects_;  // library members dropped by gcSections
    struct DiscardedSections { std::string object; size_t count; uint32_t bytes; };
    std::vector<DiscardedSections> discardedSections_;   // per trimmed object
    std::vector<std::string> discardedSectionNames_;     // "object: section (n bytes)"
    std::vector<std::string> gcErrors_;
    void gcSections(size_t numRoots);
    bool compactObject(const O45File& o, const std::vector<bool>& keepSec,
                       const std::vector<O45Reloc>& textRel, const std::vector<O45Reloc>& dataRel,
                       const std::map<std::pair<uint8_t, uint32_t>, int>& refUnit,
                       const std::vector<std::pair<int, uint32_t>>& importRefs,
                       O45File& out, std::string& err);
    uint16_t stackSize_ = 0x1000;

    // Code overlays
    std::vector<std::string> ovlNames_;            // overlay i + 1
    std::vector<std::vector<uint8_t>> ovlText_;    // their code
    uint32_t ovlBase_ = 0, ovlSize_ = 0;           // the region they run in
    bool ovlMap_ = false;
    std::vector<uint32_t> ovlAt_;                  // each overlay's far address
    // where their images are kept in far memory (default bank 5): ranges
    // (chip or attic RAM); the files: the part of a range in one bank each
    // that holds overlays, and how much of it they take; each overlay's
    // file (its number - 1); the chip RAM bank attic RAM's files are
    // loaded into before they are copied up
    std::vector<std::pair<uint32_t, uint32_t>> ovlStores_{{0x50000, 0x5FFFF}};
    std::vector<std::pair<uint32_t, uint32_t>> ovlFileRange_;
    std::vector<uint32_t> ovlFileEnd_;
    std::vector<int> ovlFileOf_;
    int ovlStaging_ = 5;
    std::string ovlFileName_;
    std::vector<std::vector<uint8_t>> ovlFiles_;
    // the files' names on disk: the first, its last character counted up
    std::string ovlFileNameOf(size_t i) const {
        std::string n = ovlFileName_.empty() ? std::string("OVERLAYS") : ovlFileName_;
        if (i && !n.empty()) n.back() = (char)(n.back() + i);
        return n;
    }
    std::vector<uint8_t> ovlDesc_;   // the overlays' description (nm45, objdump45)
    std::map<std::string, int> symOvl_;            // a text symbol's overlay
    std::map<std::string, uint32_t> realAddr_;     // its address in the region
    std::map<std::string, uint32_t> ovlStubs_;     // its stub (resident)
    uint32_t ovlTableAddr_[6] = {}, ovlFileAddr_ = 0;   // __ovl_table0..5 (a byte per overlay each)
    std::set<std::string> forcedUndefined_;
    uint32_t textBaseOf(const struct InputObject& in) const;
    // the address of an offset of an object's text (resident, or where its overlay runs)
    uint32_t textAddrOf(const struct InputObject& in, uint32_t origOff) const;
    size_t ovlUserCount_ = 0;     // overlays named by the objects (.overlay); the rest: __far code
    bool hasFar_ = false;         // some object has __far code (a "far" text sub-segment)
    // __bank(n) (#69): the 64 KB bank an overlay must be kept in (-1: any;
    // __bank(n) functions' "far.n" code), and data overlays (the "bank.n"
    // variables: never run, their symbols at their far addresses)
    std::vector<int> ovlBank_;
    std::vector<bool> ovlData_;
    bool hasBanked_ = false;      // some object has "bank.n" / "far.n" text
    bool isDataOvl(int ovl) const { return ovl > 0 && (size_t)ovl <= ovlData_.size() && ovlData_[ovl - 1]; }
    // where the overlays are kept: the store ranges cut at banks, how far
    // each is filled, each overlay's part (placeOverlayStore)
    std::vector<std::pair<uint32_t, uint32_t>> storeParts_;
    std::vector<uint32_t> storeCur_;
    std::vector<size_t> ovlPartOf_;
    bool placeOverlayStore(std::string& errorMsg);
    void assignOverlays();
    bool placeOverlayRegion(std::string& errorMsg);
    bool generateOverlayStubs(std::string& errorMsg);
    void buildOverlayFile();
    // the key of zarWrapAt_: an address and the overlay it is in (0: resident)
    static uint32_t zarKey(int ovl, uint32_t addr) { return ((uint32_t)ovl << 16) | (addr & 0xFFFF); }
    // Maps callee name → thunk address (for mismatched call sites)
    std::map<std::string, uint32_t> thunkAddresses_;
    // Per-call-site override: (objectIdx, relocIdx) → thunk target address
    std::map<std::pair<int,int>, uint32_t> callSiteOverrides_;

    // SAC overlay: per object, the SAC blocks moved into the overlay area
    // (object BSS offset, size, offset in the overlay area)
    struct SacMove { uint32_t srcOffset, len, overlayOffset; std::string func; };
    std::map<int, std::vector<SacMove>> sacMoves_;
    uint32_t sacOverlayLen_ = 0;      // size of the overlay area
    std::map<std::string, uint32_t> zarOffset_;   // function → its base-page block (.zar)
    std::map<std::string, uint32_t> zarSize_;     // function → block size
    std::set<std::string> zarWrapped_;            // functions reached through a save/restore wrapper
    std::map<uint32_t, uint32_t> zarWrapAt_;      // entry address → its wrapper
    uint32_t zarUsed_ = 0, zarTotal_ = 0;         // bytes of the area used / asked for
    // The area of the base-page blocks and __zp variables: from the end of
    // the register pool ($30-$7F) the program's objects use (their largest
    // cc45.bptop; an object without one uses all of it) to $F1 ($F2-$FC:
    // cc45's 32-bit temp and the library's temps)
    static constexpr uint32_t ZAR_HI = 0xF2;
    uint32_t zarLo_ = 0x80;
    void computeZarLo();
    // __zp variables (cc45 cc45.zpvar.<sym> = size): placed at the top of
    // the area, below them the base-page blocks; <sym> = __zp_page + address.
    // One with cc45.zpinit.<sym> (its initial value, in data) is copied
    // there by the generated __zp_init that crt0 calls after the BSS fill
    std::map<std::string, uint32_t> zpVarAddr_, zpVarSize_;
    uint32_t zpVarTotal_ = 0;          // (with the zp segment below the variables)
    int64_t zpSegAt_ = -1;             // the zp segment's base-page address (placeZpVars), -1: at zpBase_
    // __section variables (cc45, Issue #70): each object's part of a section
    // (cc45.sectsize.NAME bytes) follows the previous one's from the
    // section's address (cc45.section.NAME, or --section NAME=ADDR); a
    // variable is at its part + cc45.sectvar.NAME.<sym>. One with
    // cc45.sectinit.<sym> (its initial value, in data) is copied there by
    // DMA in the generated __zp_init
    // A __section function's code (text sub-segment "sect.NAME") is linked
    // to run at the start of its section (sectCode_, its objects' chunks in
    // link order, then the variables); a copy of it in the program's text
    // (sectCodeImage_: its offset) is copied there at startup
    struct Section { uint32_t addr = 0, size = 0, code = 0; std::string from; };
    std::vector<std::string> sectCodeNames_;
    std::vector<std::vector<uint8_t>> sectCode_;
    std::vector<uint32_t> sectCodeImage_;
    void copySectionCode();
    std::map<std::string, Section> sections_;
    std::map<std::string, uint32_t> sectionAddrOpt_;   // --section NAME=ADDR
    struct SectVar { uint32_t addr = 0, size = 0; std::string section, object; };
    std::map<std::string, SectVar> sectVars_;
    bool placeSections(std::string& errorMsg);
    bool checkSections(std::string& errorMsg);    // against the program, the stack, overlays
    uint32_t zpInitAt_ = 0;                           // the generated __zp_init (0: none)
    std::vector<std::pair<uint32_t, std::string>> zpInitPatches_;   // text offset, symbol (an address word)
    uint32_t sacOverlayOffset_ = 0;   // its offset in the merged BSS
    uint32_t sacOverlaySaved_ = 0;    // bytes saved by sharing
    bool sacOverlayOn_ = true;        // --no-sac-overlay clears it

    // Phase 4: Parameter specialization
    // Maps function name → parameter index → (is_constant, constant_value)
    // Tracks which parameters are TRULY constant across all call sites
    std::map<std::string, std::map<int, SpecializedParam>> specializedParams_;

    // Phase 4.3: IPO Hints aggregation
    // Merged IPO hints from all objects with global call counts and optimization metadata
    O45IPOHints aggregatedIPOHints_;

    // Phase 4.4: IPO Hints application
    // Track functions that have optimization hints from IPO analysis
    // Maps function name → whether dispatcher/optimization should be applied
    std::map<std::string, bool> ipoOptimizationCandidates_;
    // Track leaf functions identified by IPO hints
    std::set<std::string> ipoLeafFunctions_;

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
    void checkAddressCollisions();
    bool applyRelocations(std::string& errorMsg);
    void buildFuncAttrs();
    void buildCallGraph();
    void computeTransitiveClobbers();
    void analyzeIPOHints();             // Phase 4.3: Aggregate IPO hints from all input objects
    void applyIPOHints();               // Phase 4.4: Apply IPO hints to optimization decisions
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
    int64_t bssLenBeforeZpPage_ = -1;  // BSS length before the __zp_page block was added
    std::map<std::string, int> funcObjMode_;  // function -> its object's cc45 mode (1 zp, 0 stack)
    std::map<std::string, std::set<std::string>> funcZpCalls_;  // function -> callees its object calls with the ZP convention (cc45.zpcall.<callee>)
    int funcMode(const std::string& fn) const;
    bool callNeedsThunk(const std::string& caller, const std::string& callee, bool& callerIsZp) const;
    void emitDiagnostics();
    void verifyStaticAllocSafety();  // Verify SAC (static allocation convention) constraints
    void validateSACParameters();     // Phase 3: Validate SAC parameter metadata
    void validateImmediateRelocations(); // Phase 78.4: Validate immediate relocation symbols
    void computeSacOverlay();          // place SAC blocks so inactive functions share storage
    // the calls between functions (JSR/JMP) and the functions whose address is used otherwise
    void buildCallGraph(std::map<std::string, std::set<std::string>>& graph, std::set<std::string>& addrTaken);
    // place the functions' private base-page blocks (cc45 .zar) in $80-$E9 of
    // the program's base page, sharing between functions never active together
    bool computeZarOverlay(std::string& errorMsg);
    // the order computeZarOverlay places blocks in: callers before callees,
    // largest first among unrelated ones and inside cycles
    static std::vector<std::pair<std::string, uint32_t>> zarOrder(
        const std::map<std::string, uint32_t>& sizes, std::map<std::string, std::set<std::string>>& reach);
    void buildZpPlan(std::map<std::string, std::set<std::string>>& reach, uint32_t area);
    std::string zpPlanOut_;
    bool ovlFastStubs_ = false;
    // a function whose block shares bytes with a function active together
    // with it is reached through a wrapper that saves and restores them
    void generateZarWrappers();
    bool placeZpVars(std::string& errorMsg);
    void checkZpVarsInIsr();      // base-page uses from code an interrupt handler reaches
    void generateZpInit();
    void patchZpInit();
    void layoutBss();                  // BSS layout (objects, then the SAC overlay area)
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
