// Phase 113: Learning Feedback Loop Tests
// Validates end-to-end feedback cycle: hooks → metrics → learner → next compilation

#include "LearnerFeedbackRecorder.hpp"
#include "OnlineLearner.hpp"
#include <cassert>
#include <iostream>
#include <string>

// Test 1: Feedback recorder initializes correctly
void testFeedbackRecorderInitialization() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_1");

    LearnerFeedbackRecorder recorder(learner.get());

    assert(recorder.getTotalCompilationsRecorded() == 0);
    assert(recorder.getAverageOverallBenefit() == 0.0);

    std::cout << "✓ Test 1 PASSED: Feedback recorder initialization" << std::endl;
}

// Test 2: Recording compilation metrics
void testRecordingCompilationMetrics() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_2");

    LearnerFeedbackRecorder recorder(learner.get());

    CompilationMetrics metrics;
    metrics.sourceFile = "test.c";
    metrics.finalAssemblySize = 1024;
    metrics.totalCompileTimeMs = 50.0;
    metrics.optimizationLevel = 2;

    metrics.appliedOptimizations.push_back("inline-small-functions");
    metrics.appliedOptimizations.push_back("constant-folding");

    metrics.optimizationSuccessful["inline-small-functions"] = true;
    metrics.optimizationSuccessful["constant-folding"] = true;

    metrics.actualSpeedup["inline-small-functions"] = 5.0;   // 5% speedup
    metrics.actualSpeedup["constant-folding"] = 3.0;         // 3% speedup

    metrics.actualSizeReduction["inline-small-functions"] = 8.0;  // 8% smaller
    metrics.actualSizeReduction["constant-folding"] = 4.0;        // 4% smaller

    recorder.recordMetrics(metrics);

    assert(recorder.getTotalCompilationsRecorded() == 1);

    std::cout << "✓ Test 2 PASSED: Recording compilation metrics" << std::endl;
}

// Test 3: Finalizing compilation feedback
void testFinalizingCompilationFeedback() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_3");

    LearnerFeedbackRecorder recorder(learner.get());

    CompilationMetrics metrics;
    metrics.sourceFile = "test.c";
    metrics.finalAssemblySize = 2048;
    metrics.totalCompileTimeMs = 100.0;

    metrics.appliedOptimizations.push_back("loop-unrolling");
    metrics.optimizationSuccessful["loop-unrolling"] = true;
    metrics.actualSpeedup["loop-unrolling"] = 12.0;       // 12% speedup
    metrics.actualSizeReduction["loop-unrolling"] = 10.0; // 10% smaller

    FeedbackResult result = recorder.finalizeCompilation(metrics);

    assert(result.success);
    assert(result.optimizationsApplied == 1);
    assert(result.optimizationsBeneficial == 1);
    assert(result.overallBenefitScore > 0.0);

    std::cout << "✓ Test 3 PASSED: Finalizing compilation feedback (score="
              << result.overallBenefitScore << ")" << std::endl;
}

// Test 4: Multiple compilations and averaging
void testMultipleCompilationsAveraging() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_4");

    LearnerFeedbackRecorder recorder(learner.get());

    // First compilation
    CompilationMetrics m1;
    m1.sourceFile = "file1.c";
    m1.finalAssemblySize = 1024;
    m1.appliedOptimizations.push_back("constant-folding");
    m1.optimizationSuccessful["constant-folding"] = true;
    m1.actualSpeedup["constant-folding"] = 2.0;
    m1.actualSizeReduction["constant-folding"] = 2.0;

    FeedbackResult r1 = recorder.finalizeCompilation(m1);
    double avgAfterFirst = recorder.getAverageOverallBenefit();

    // Second compilation
    CompilationMetrics m2;
    m2.sourceFile = "file2.c";
    m2.finalAssemblySize = 1536;
    m2.appliedOptimizations.push_back("dead-code-elimination");
    m2.optimizationSuccessful["dead-code-elimination"] = true;
    m2.actualSpeedup["dead-code-elimination"] = 4.0;
    m2.actualSizeReduction["dead-code-elimination"] = 6.0;

    FeedbackResult r2 = recorder.finalizeCompilation(m2);
    double avgAfterSecond = recorder.getAverageOverallBenefit();

    assert(recorder.getTotalCompilationsRecorded() == 2);
    assert(avgAfterSecond >= 0.0);  // Averaging should be non-negative

    std::cout << "✓ Test 4 PASSED: Multiple compilations averaging (avg="
              << avgAfterSecond << ")" << std::endl;
}

// Test 5: Handling unsuccessful optimizations
void testHandlingUnsuccessfulOptimizations() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_5");

    LearnerFeedbackRecorder recorder(learner.get());

    CompilationMetrics metrics;
    metrics.sourceFile = "test.c";
    metrics.finalAssemblySize = 1024;

    metrics.appliedOptimizations.push_back("aggressive-inline");
    metrics.appliedOptimizations.push_back("loop-invariant-removal");

    // One successful, one unsuccessful
    metrics.optimizationSuccessful["aggressive-inline"] = true;
    metrics.optimizationSuccessful["loop-invariant-removal"] = false;

    metrics.actualSpeedup["aggressive-inline"] = 10.0;
    metrics.actualSpeedup["loop-invariant-removal"] = -2.0;  // Made it slower!

    metrics.actualSizeReduction["aggressive-inline"] = 5.0;
    metrics.actualSizeReduction["loop-invariant-removal"] = -1.0;  // Made it bigger!

    FeedbackResult result = recorder.finalizeCompilation(metrics);

    assert(result.success);
    assert(result.optimizationsApplied == 2);
    assert(result.optimizationsBeneficial == 1);  // Only aggressive-inline was beneficial

    std::cout << "✓ Test 5 PASSED: Handling unsuccessful optimizations (beneficial="
              << result.optimizationsBeneficial << "/" << result.optimizationsApplied << ")"
              << std::endl;
}

// Test 6: Benefit score calculation
void testBenefitScoreCalculation() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_6");

    LearnerFeedbackRecorder recorder(learner.get());

    // Optimize for code size (60% weight) + performance (40% weight)
    CompilationMetrics metrics;
    metrics.sourceFile = "test.c";

    metrics.appliedOptimizations.push_back("test-opt");
    metrics.optimizationSuccessful["test-opt"] = true;

    // 10% speedup (40% weight) + 20% size reduction (60% weight) = 16% benefit score
    metrics.actualSpeedup["test-opt"] = 10.0;
    metrics.actualSizeReduction["test-opt"] = 20.0;

    FeedbackResult result = recorder.finalizeCompilation(metrics);

    // Expected: (10 * 0.4) + (20 * 0.6) = 4 + 12 = 16
    double expectedBenefit = 16.0;
    assert(std::abs(result.overallBenefitScore - expectedBenefit) < 0.1);

    std::cout << "✓ Test 6 PASSED: Benefit score calculation (score="
              << result.overallBenefitScore << ", expected=" << expectedBenefit << ")"
              << std::endl;
}

// Test 7: Learning signals feedback
void testLearningSignalsFeedback() {
    auto learner = std::make_unique<phase107::OnlineLearner>();
    learner->initializeSession("test_session_7");

    LearnerFeedbackRecorder recorder(learner.get());

    CompilationMetrics metrics;
    metrics.sourceFile = "large_file.c";
    metrics.finalAssemblySize = 5000;  // Large file
    metrics.appliedOptimizations.push_back("optimization-a");
    metrics.optimizationSuccessful["optimization-a"] = true;
    metrics.actualSpeedup["optimization-a"] = 5.0;
    metrics.actualSizeReduction["optimization-a"] = 8.0;

    // Finalize triggers signal collection
    FeedbackResult result = recorder.finalizeCompilation(metrics);

    assert(result.success);
    // Signals should have been sent to learner
    const auto& sessionState = learner->getSessionState();
    assert(sessionState.collectedSignals.size() > 0);

    std::cout << "✓ Test 7 PASSED: Learning signals feedback (signals="
              << sessionState.collectedSignals.size() << ")" << std::endl;
}

// Main test runner
int main() {
    std::cout << "\n=== Phase 113: Learning Feedback Loop Tests ===" << std::endl;

    try {
        testFeedbackRecorderInitialization();
        testRecordingCompilationMetrics();
        testFinalizingCompilationFeedback();
        testMultipleCompilationsAveraging();
        testHandlingUnsuccessfulOptimizations();
        testBenefitScoreCalculation();
        testLearningSignalsFeedback();

        std::cout << "\n✓ ALL TESTS PASSED (7/7)" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
