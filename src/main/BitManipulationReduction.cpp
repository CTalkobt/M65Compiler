#include "BitManipulationReduction.hpp"

BitManipulationReduction::BitManipulationReduction()
    : OptimizationPassBase(OptimizationType::BIT_MANIPULATION_REDUCTION,
                          "Bit Manipulation Strength Reduction"),
      patternsOptimized_(0), bytesReduced_(0) {
}

BitManipulationReduction::~BitManipulationReduction() = default;

void BitManipulationReduction::apply(TranslationUnit& ast) {
    // AST-level bit pattern detection
    detectBitPatterns(ast);

    // Apply strength reduction transformations
    applyStrengthReduction(ast);

    // Update metrics
    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = patternsOptimized_;
}

void BitManipulationReduction::apply(ir::Module& irModule) {
    // IR-level bit pattern detection
    detectBitPatternsIR(irModule);

    // Apply IR-level transformations
    applyStrengthReductionIR(irModule);

    // Update metrics
    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = patternsOptimized_;
}

void BitManipulationReduction::detectBitPatterns(TranslationUnit& /* ast */) {
    // Pattern detection for common bit operations
    //
    // Strength reduction rules:
    //   1. AND with mask: 0x01 → single bit test (TSB/TRB)
    //   2. AND with 0xFF → clear high byte (clear A / AND #0xFF for 8-bit)
    //   3. OR with 0xFF → set all bits (LDA #0xFF)
    //   4. XOR with 0x00 → noop (remove operation)
    //   5. Shift by 1 → ASL/LSR (faster than multiply/divide)
    //   6. Shift by power-of-2 → multiple ASL/LSR chains
    //   7. Rotate left/right → ROL/ROR instructions
    //   8. Bit extraction: (x >> n) & mask → bit shift + mask
    //   9. Bit insertion: x = (x & ~mask) | (bits & mask) → TRB + TSB
    //   10. Multiple bit tests: (x & 0x03) == 0x03 → BIT then BVC/BVS

    // TODO: Walk AST for binary operations
    // TODO: Detect AND, OR, XOR, shift operations with constants
    // TODO: Check if cheaper instruction exists
}

void BitManipulationReduction::detectBitPatternsIR(ir::Module& /* irModule */) {
    // IR-level bit operation detection
    // Scan for AND/OR/XOR nodes with constant operands
    // TODO: Walk IR module for bit operations
    // TODO: Extract operand patterns
}

bool BitManipulationReduction::matchesMaskPattern(unsigned int mask) const {
    // Check if mask matches a pattern with special instruction
    //   0x01 → single bit test (TSB/TRB one of bits 0-7)
    //   0xFF → all bits in byte
    //   0x0F → nibble mask
    //   0x03 → 2-bit mask
    //   Power-of-2 → potential bit test

    // Single bit masks
    if (__builtin_popcount(mask) == 1) return true;  // TSB/TRB instruction
    if (mask == 0xFF) return true;      // 8-bit all
    if (mask == 0x0F) return true;      // 4-bit (nibble)
    if (mask == 0x03) return true;      // 2-bit
    if (mask == 0x01) return true;      // 1-bit (LSB)

    return false;
}

std::string BitManipulationReduction::getOptimalInstruction(unsigned int mask, const std::string& op) {
    // Return cheaper instruction for bit pattern
    //
    // Examples:
    //   AND 0x01 → TSB $addr (test single bit)
    //   AND 0xFF → NOP (identity)
    //   OR  0x01 → TSB $addr (set bit)
    //   XOR 0x00 → NOP (identity)
    //   Shift 1 → ASL/LSR (1-byte)
    //   Shift 2 → ASL ASL / LSR LSR (2-bytes)
    //   Shift 3+ → MUL/DIV (cheaper than cascade shifts)

    if (op == "AND") {
        if (mask == 0x01) return "BIT $addr";  // Test LSB
        if (mask == 0xFF) return "NOP";         // Identity
        if (__builtin_popcount(mask) == 1) return "TSB check";  // Test single bit
    } else if (op == "OR") {
        if (mask == 0xFF) return "LDA #0xFF";  // All bits set
        if (__builtin_popcount(mask) == 1) return "TSB set";   // Set single bit
    } else if (op == "XOR") {
        if (mask == 0x00) return "NOP";         // Identity
        if (mask == 0xFF) return "EOR #0xFF";  // Bitwise NOT
    }

    return "";  // No special instruction
}

void BitManipulationReduction::applyStrengthReduction(TranslationUnit& /* ast */) {
    // Apply strength reduction transformations to AST
    // For each detected pattern:
    //   1. Replace with cheaper equivalent
    //   2. Update metrics (instruction count, bytes reduced)
    //   3. Mark for code generation

    // TODO: Replace operations with reduced forms
    // TODO: Update bytesReduced_ and patternsOptimized_
}

void BitManipulationReduction::applyStrengthReductionIR(ir::Module& /* irModule */) {
    // Apply IR-level strength reduction
    // TODO: Transform IR operations
}
