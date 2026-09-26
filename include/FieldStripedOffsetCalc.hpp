#pragma once

// Phase 95.2: Field-level offset calculation for striped struct arrays
// Computes memory addresses for individual struct fields in field-striped layout

#include <string>
#include <vector>

// Field-level offset calculation context
struct FieldStripedOffsetContext {
    // Array dimensions
    int height;                    // Last dimension (second-to-last in N-D array)
    int width;                     // Last dimension

    // Stripe configuration
    int stripeWidth;               // 4, 8, etc. (power of 2)
    int log2StripeWidth;           // log2(stripeWidth)
    int strideFactor;              // (height * fieldSize) / stripeWidth

    // Field information
    std::string fieldName;         // e.g., "r", "g", "b"
    int fieldSize;                 // Size of this field in bytes (1, 2, 4, etc.)
    int fieldRegionOffset;         // Byte offset where this field's data starts

    // Zero-page temporary addresses for calculation
    std::string zpBase;            // Base address of array (as "$XX")
    std::string zpRow;             // Row index (as "$XX")
    std::string zpCol;             // Column index (as "$XX")
    std::string zpResult;          // Result offset (as "$XX")

    // Outer dimension info (for 3D+ arrays)
    int outerDimSize = 0;          // Product of dimensions before height/width
    std::string zpOuterOffset;     // Zero-page address for outer dimension offset
};

// Assembly code generator for field-level offset calculation
class FieldStripedOffsetCalc {
public:
    // Generate optimized assembly for field-striped offset calculation
    // Returns vector of assembly instructions
    std::vector<std::string> generateOffsetCalculation(
        const FieldStripedOffsetContext& ctx
    );

    // Calculate the stride factor for a given field
    // stride_factor = (height * fieldSize) / stripeWidth
    static int calculateStrideFactor(int height, int fieldSize, int stripeWidth);

    // Calculate memory region offset where field data starts
    // For RGB[256][256]: r at 0, g at 65536, b at 131072
    static int calculateFieldRegionOffset(
        const std::vector<int>& fieldSizes,
        int fieldIndex,
        int height,
        int width
    );

    // Check if a value is a power of 2
    static bool isPowerOfTwo(int value);

    // Get log2 of a power-of-2 value
    static int getLog2(int powerOfTwo);

private:
    // Generate shift-based multiplication (for power-of-2 multipliers)
    std::vector<std::string> generateShiftMultiply(
        const std::string& resultReg,
        int shiftAmount,
        const std::string& sourceReg = "A"
    );

    // Generate multiply instruction (for non-power-of-2 multipliers)
    std::vector<std::string> generateMultiply(
        const std::string& resultReg,
        const std::string& lhsReg,
        const std::string& rhsValue
    );

    // Generate the stripe select calculation: col >> log2Stripe
    std::vector<std::string> generateStripeSelect(
        const FieldStripedOffsetContext& ctx
    );

    // Generate the base offset calculation
    // = stripe_select * height * fieldSize + row * fieldSize
    std::vector<std::string> generateBaseOffset(
        const FieldStripedOffsetContext& ctx
    );

    // Generate the column remainder offset
    // = (col & mask) * stride_factor
    std::vector<std::string> generateColumnRemainderOffset(
        const FieldStripedOffsetContext& ctx
    );
};
