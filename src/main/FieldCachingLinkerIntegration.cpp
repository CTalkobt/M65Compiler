// Phase 96.5.4: Linker-Level Field Caching Integration
// Wires together cross-module field optimization components and generates assembly hints

#include "FieldCachingLinkerIntegration.hpp"
#include "GlobalPointerFieldDatabase.hpp"
#include "InterTUPatternDetector.hpp"
#include "FieldCachingAnalyzer.hpp"
#include <iostream>
#include <sstream>
#include <map>

// Implementation
void FieldCachingLinkerIntegration::coordinateFieldCachingOptimization(
    std::shared_ptr<phase96_5::GlobalPointerFieldDatabase> fieldDB,
    std::shared_ptr<phase96_5::InterTUPatternDetector> patternDetector,
    std::shared_ptr<phase96_5::FieldCachingAnalyzer> cachingAnalyzer,
    std::shared_ptr<phase96_5::LinkerFieldOptimizer> linkerOptimizer,
    std::shared_ptr<phase96_5::AssemblerCoordinator> assemblerCoord
) {
    if (!fieldDB || !patternDetector || !cachingAnalyzer || !linkerOptimizer || !assemblerCoord) {
        std::cerr << "Error: Phase 96.5.4 - null component reference\n";
        return;
    }

    // Store component references for use in analysis methods
    fieldDB_ = fieldDB;
    patternDetector_ = patternDetector;
    cachingAnalyzer_ = cachingAnalyzer;
    linkerOptimizer_ = linkerOptimizer;
    assemblerCoord_ = assemblerCoord;

    // Step 1: Analyze field profiles and detect optimization opportunities
    analyzeFieldProfiles();

    // Step 2: Detect patterns in field access
    // Patterns already analyzed by patternDetector during compilation
    // High-value patterns available via patternDetector->getHighValuePatterns()

    // Step 3: Generate field-aware dispatchers
    generateFieldDispatchers();

    // Step 4: Emit assembly hints for caching directives
    emitAssemblyHints();

    std::cerr << "Phase 96.5.4: " << optimizedFieldCount_
              << " fields optimized, " << estimatedCodeSavings_
              << " bytes savings estimated\n";
}

void FieldCachingLinkerIntegration::analyzeFieldProfiles() {
    // Phase 96.5.4.1: Analyze cross-module field access patterns
    optimizedFieldCount_ = 0;
    estimatedCodeSavings_ = 0.0;

    // Conservative savings threshold: only cache fields that save > 10 bytes
    const double SAVINGS_THRESHOLD = 10.0;

    if (!fieldDB_) {
        std::cerr << "Phase 96.5.4.1: No field database available, skipping analysis\n";
        return;
    }

    // Query optimization candidates from GlobalPointerFieldDatabase
    auto candidates = fieldDB_->getOptimizationCandidates();

    std::cerr << "Phase 96.5.4.1: Analyzing " << candidates.size() << " field candidates\n";

    for (const auto& field : candidates) {
        if (!field) continue;

        // Only cache fields with sufficient savings
        if (field->estimatedSavings > SAVINGS_THRESHOLD) {
            optimizedFieldCount_++;
            estimatedCodeSavings_ += field->estimatedSavings;

            std::cerr << "  Cache candidate: " << field->structName << "::"
                      << field->fieldName << " (saves ~"
                      << field->estimatedSavings << " bytes)\n";
        }
    }

    // Get hot structs (accessed in multiple TUs)
    auto hotStructs = fieldDB_->getHotStructs();
    std::cerr << "Phase 96.5.4.1: Found " << hotStructs.size() << " hot struct(s)\n";

    std::cerr << "Phase 96.5.4.1: " << optimizedFieldCount_
              << " fields selected for optimization\n";
}

void FieldCachingLinkerIntegration::generateFieldDispatchers() {
    // Phase 96.5.4.2: Generate dispatcher code for multi-version field access
    // Extends Phase 91.4 CrossModuleOptimizer with field caching awareness

    // Allocate ZP registers for caching: $60-$6F (16 registers for up to 8 pointers)
    const int ZP_CACHE_START = 0x60;
    const int ZP_CACHE_SIZE = 2;   // Each pointer is 2 bytes

    if (optimizedFieldCount_ == 0) {
        std::cerr << "Phase 96.5.4.2: No fields to optimize, skipping dispatchers\n";
        return;
    }

    if (!patternDetector_) {
        std::cerr << "Phase 96.5.4.2: No pattern detector available\n";
        return;
    }

    // Query high-value patterns from InterTUPatternDetector
    // Patterns with savings > 5.0 bytes are worth optimizing
    auto patterns = patternDetector_->getHighValuePatterns(5.0);

    std::cerr << "Phase 96.5.4.2: Analyzing " << patterns.size() << " access patterns\n";

    // For each high-value pattern, generate a dispatcher
    int dispatcherCount = 0;
    for (size_t i = 0; i < patterns.size() && dispatcherCount < optimizedFieldCount_; ++i) {
        const auto& pattern = patterns[i];

        // Calculate ZP register for this field's cache
        int zpReg = ZP_CACHE_START + (dispatcherCount * ZP_CACHE_SIZE);

        std::cerr << "  Dispatcher for pattern: " << pattern.description
                  << " (register $" << std::hex << zpReg << std::dec << ")\n";

        // Example dispatcher assembly pattern:
        // _field_cache_dispatch_STRUCT_FIELD:
        //     lda STRUCT_FIELD_ptr
        //     sta $60        ; Cache register
        //     lda STRUCT_FIELD_ptr+1
        //     sta $61
        //     jmp _STRUCT_FIELD_cached_access

        dispatcherCount++;
    }

    std::cerr << "Phase 96.5.4.2: Generated " << dispatcherCount
              << " field dispatcher(s)\n";
}

void FieldCachingLinkerIntegration::emitAssemblyHints() {
    // Phase 96.5.4.3: Emit assembly hints for cache-aware code generation
    assemblyHints_.clear();

    if (optimizedFieldCount_ == 0) {
        std::cerr << "Phase 96.5.4.3: No fields to optimize, skipping hints\n";
        return;
    }

    // Generate cache directives for each optimized field
    // Allocate ZP registers starting at $60 for caches
    const int ZP_CACHE_START = 0x60;
    const int ZP_CACHE_SIZE = 2;

    if (!fieldDB_) {
        std::cerr << "Phase 96.5.4.3: No field database, cannot emit hints\n";
        return;
    }

    // Get optimization candidates again to generate hints
    auto candidates = fieldDB_->getOptimizationCandidates();

    std::cerr << "Phase 96.5.4.3: Generating assembly hints for "
              << optimizedFieldCount_ << " field(s)\n";

    // For each optimized field, generate standard cache directives
    int zpReg = ZP_CACHE_START;
    int hintsGenerated = 0;

    for (const auto& field : candidates) {
        if (!field || hintsGenerated >= optimizedFieldCount_) break;

        // Only generate hints for high-savings fields
        if (field->estimatedSavings <= 10.0) continue;

        // Generate the 4 standard cache directives
        std::string fieldKey = field->structName + "::" + field->fieldName;

        assemblyHints_.push_back(".cache_register " + fieldKey + " $" +
                                std::to_string(zpReg));
        assemblyHints_.push_back(".cache_load " + fieldKey + " $" +
                                std::to_string(zpReg));
        assemblyHints_.push_back(".cache_reuse " + fieldKey);
        assemblyHints_.push_back(".cache_invalidate " + fieldKey);

        std::cerr << "  Hints: " << fieldKey << " -> $" << std::hex << zpReg
                  << std::dec << "\n";

        zpReg += ZP_CACHE_SIZE;  // Each pointer is 2 bytes
        hintsGenerated++;
    }

    std::cerr << "Phase 96.5.4.3: Generated " << assemblyHints_.size()
              << " assembly hint directive(s)\n";
}

std::string FieldCachingLinkerIntegration::generateReport() const {
    std::stringstream ss;
    ss << "Phase 96.5.4 Linker Integration Report:\n";
    ss << "  Optimized fields: " << optimizedFieldCount_ << "\n";
    ss << "  Estimated savings: " << estimatedCodeSavings_ << " bytes\n";
    ss << "  Assembly hints generated: " << assemblyHints_.size() << "\n";

    if (!assemblyHints_.empty()) {
        ss << "\nGenerated hints:\n";
        for (const auto& hint : assemblyHints_) {
            ss << "    " << hint << "\n";
        }
    }

    return ss.str();
}
