#include "DMAOperationTemplate.hpp"
#include "TranslationUnit.hpp"
#include "ir/Module.hpp"

DMAOperationTemplate::DMAOperationTemplate()
    : operationsOptimized_(0), bytesReduced_(0) {
    metrics_.optimizationName = "DMA Operation Template";
    metrics_.type = OptimizationType::DMA_OPERATION_TEMPLATE;
}

DMAOperationTemplate::~DMAOperationTemplate() = default;

void DMAOperationTemplate::apply(TranslationUnit& ast) {
    // AST-level memory operation detection
    detectDMAPatterns(ast);

    // Apply DMA optimization where beneficial
    applyDMAOptimization(ast);

    // Update metrics
    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = operationsOptimized_;
}

void DMAOperationTemplate::apply(ir::Module& irModule) {
    // IR-level memory operation detection
    detectDMAPatternsIR(irModule);

    // Apply IR-level DMA optimization
    applyDMAOptimizationIR(irModule);

    // Update metrics
    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = operationsOptimized_;
}

void DMAOperationTemplate::detectDMAPatterns(TranslationUnit& /* ast */) {
    // Pattern detection for memory operations suitable for DMA
    //
    // MEGA65 F018B DMA Controller characteristics:
    //   - Bus cycle cost: 1 cycle/byte (vs. 10+ cycles for 6502 LDA/STA loop)
    //   - Setup overhead: 4-6 instructions for address/length setup
    //   - Break-even: ~20 bytes (4-6 instructions = 20 bytes transfer)
    //
    // Detectable patterns:
    //   1. memcpy(dst, src, len) → DMA_COPY if len >= 20
    //   2. memset(dst, val, len) → DMA_FILL if len >= 20
    //   3. Inline copy loops: for(i=0; i<N; i++) dst[i] = src[i]
    //   4. Inline fill loops: for(i=0; i<N; i++) dst[i] = val
    //   5. struct initialization: memcpy pattern during struct init
    //   6. Array copy: copying entire arrays
    //
    // Non-suitable patterns:
    //   1. Small copies: < 20 bytes (overhead > savings)
    //   2. Overlapping regions: memcpy vs memmove distinction
    //   3. Volatile memory: DMA may not work with HW registers
    //   4. Non-constant addresses: runtime-computed addresses (risky)

    // TODO: Walk AST for FunctionCall nodes
    // TODO: Detect memcpy, memset, memmove calls
    // TODO: Extract constant length arguments
    // TODO: Analyze inline copy loops
}

void DMAOperationTemplate::detectDMAPatternsIR(ir::Module& /* irModule */) {
    // IR-level memory operation detection
    // Scan for LOAD/STORE sequences that form memcpy/memset patterns
    // TODO: Walk IR for repeated load/store pairs
}

bool DMAOperationTemplate::isDMABeneficial(const DMACandidate& candidate) const {
    // DMA break-even analysis
    //
    // CPU copy loop (LDA/STA pair):
    //   - LDA src,X        (5 bytes, 5 cycles at high speed)
    //   - STA dst,X        (5 bytes, 5 cycles)
    //   - INX              (1 byte, 2 cycles)
    //   - BNE loop         (2 bytes, 3 cycles if taken)
    //   Total per byte: ~15 cycles of overhead for loop control
    //   Effective: 15-20 cycles per byte
    //
    // DMA copy:
    //   - 1 cycle per byte direct transfer
    //
    // Setup cost (6 instructions / 12 bytes):
    //   - LDA #len
    //   - STA DMA_LENGTH
    //   - LDA src_lo
    //   - STA DMA_SOURCE
    //   - ... etc
    //
    // Break-even: 12 bytes setup / (20-1) cycles per byte = 12 / 19 ≈ 0.6 bytes
    // Actually need longer: 20 bytes break-even at reasonable constants
    //
    // Rule: Use DMA if length >= 20 bytes AND addresses are reasonable

    if (!candidate.isConstantLength) return false;  // Runtime length too risky
    return candidate.length >= 20;
}

int DMAOperationTemplate::estimateDMABenefit(unsigned int length) const {
    // Estimate code size reduction from DMA
    //
    // CPU loop cost:
    //   - Loop body: ~20 bytes (LDA/STA/INX/BNE)
    //   - Unrolled optimized: ~15 bytes for every 2-4 bytes transferred
    //   - Worst case: length + 10 bytes
    //
    // DMA cost:
    //   - Setup: ~20 bytes (load addresses + length)
    //   - Invocation: ~2 bytes (JSR DMA or inline)
    //   - Total: ~22 bytes
    //
    // Benefit = CPUCost - DMACost
    // For N bytes: (N + 10) - 22 = N - 12
    // Positive when N > 12

    if (length < 12) return 0;  // Not beneficial
    return length - 12;          // Estimated bytes saved
}

void DMAOperationTemplate::applyDMAOptimization(TranslationUnit& /* ast */) {
    // Apply DMA optimization to AST
    // For each identified DMA candidate:
    //   1. Replace function call or loop with DMA invocation
    //   2. Generate DMA setup code
    //   3. Update metrics

    // TODO: Replace memcpy calls with __dma_copy
    // TODO: Replace memset calls with __dma_fill
    // TODO: Replace inline loops with DMA
}

void DMAOperationTemplate::applyDMAOptimizationIR(ir::Module& /* irModule */) {
    // Apply IR-level DMA optimization
    // TODO: Transform IR LOAD/STORE sequences into DMA ops
}
