// Phase 96.5.4: Linker-Level Field Caching Integration
// Coordinates cross-module field optimization pipeline

#pragma once

#include <memory>
#include <vector>
#include <string>

// Forward declarations from Phase 96.5
namespace phase96_5 {
    class GlobalPointerFieldDatabase;
    class InterTUPatternDetector;
    class FieldCachingAnalyzer;
    class LinkerFieldOptimizer;
    class AssemblerCoordinator;
}

/**
 * Phase 96.5.4: Linker Integration Coordinator
 *
 * Orchestrates the cross-module field caching optimization pipeline:
 * 1. Collects field access patterns from GlobalPointerFieldDatabase
 * 2. Detects optimization patterns using InterTUPatternDetector
 * 3. Analyzes caching feasibility using FieldCachingAnalyzer
 * 4. Merges profiles and optimizes using LinkerFieldOptimizer
 * 5. Generates assembly hints via AssemblerCoordinator
 *
 * Integration Points:
 * - Phase 91.4: Extends dispatcher generation for field caching
 * - Phase 95: Feeds hints to assembler optimizer
 * - ca45: Recognizes .cache_* directives
 */
class Phase96_5LinkerIntegration {
public:
    Phase96_5LinkerIntegration();
    ~Phase96_5LinkerIntegration() = default;

    // Main orchestration point: run the complete pipeline
    void coordinateFieldCachingOptimization(
        std::shared_ptr<phase96_5::GlobalPointerFieldDatabase> fieldDB,
        std::shared_ptr<phase96_5::InterTUPatternDetector> patternDetector,
        std::shared_ptr<phase96_5::FieldCachingAnalyzer> cachingAnalyzer,
        std::shared_ptr<phase96_5::LinkerFieldOptimizer> linkerOptimizer,
        std::shared_ptr<phase96_5::AssemblerCoordinator> assemblerCoord
    );

    // Phase 96.5.4.1: Analyze field access profiles for caching opportunities
    void analyzeFieldProfiles();

    // Phase 96.5.4.2: Generate dispatcher code for multi-version field access
    // Extends Phase 91.4 CrossModuleOptimizer dispatcher generation
    void generateFieldDispatchers();

    // Phase 96.5.4.3: Emit assembly hints for cache-aware code generation
    void emitAssemblyHints();

    // Get generated assembly hint directives
    std::vector<std::string> getAssemblyHintDirectives() const;

    // Statistics
    int getOptimizedFieldCount() const;
    double getEstimatedCodeSavings() const;

    // Generate optimization report
    std::string generateReport() const;

private:
    // Assembly hint directives (.cache_load, .cache_reuse, etc.)
    std::vector<std::string> assemblyHints_;

    // Statistics tracking
    int optimizedFieldCount_ = 0;
    double estimatedCodeSavings_ = 0.0;

    // Component references (set during coordination)
    std::shared_ptr<phase96_5::GlobalPointerFieldDatabase> fieldDB_;
    std::shared_ptr<phase96_5::InterTUPatternDetector> patternDetector_;
    std::shared_ptr<phase96_5::FieldCachingAnalyzer> cachingAnalyzer_;
    std::shared_ptr<phase96_5::LinkerFieldOptimizer> linkerOptimizer_;
    std::shared_ptr<phase96_5::AssemblerCoordinator> assemblerCoord_;

    // Internal helper methods
    void wirePhase96_5Pipeline();
    void validateCachingStrategy();
    void allocateRegistersForCaching();
};
