#include "FieldStripedOffsetCalc.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

std::vector<std::string> FieldStripedOffsetCalc::generateOffsetCalculation(
    const FieldStripedOffsetContext& ctx
) {
    std::vector<std::string> asm_code;

    // Phase 95.2: Calculate field-level striped offset
    // offset = field_region_offset +
    //          (col >> log2Stripe) * height * fieldSize +
    //          row * fieldSize +
    //          (col & mask) * stride_factor

    // Step 1: Calculate stripe select (col >> log2Stripe)
    auto stripe_select = generateStripeSelect(ctx);
    asm_code.insert(asm_code.end(), stripe_select.begin(), stripe_select.end());

    // Step 2: Multiply stripe_select by height * fieldSize
    asm_code.push_back("tax");  // Save stripe_select in X
    asm_code.push_back("lda " + ctx.zpRow);

    // Multiply row by fieldSize
    if (ctx.fieldSize == 4) {
        asm_code.push_back("asl");
        asm_code.push_back("asl");
    } else if (isPowerOfTwo(ctx.fieldSize)) {
        int shifts = getLog2(ctx.fieldSize);
        for (int i = 0; i < shifts; i++) {
            asm_code.push_back("asl");
        }
    } else {
        asm_code.push_back("ldy #$" + std::to_string(ctx.fieldSize & 0xFF));
        asm_code.push_back("mul.8y");
    }

    asm_code.push_back("tay");  // Save row*fieldSize in Y

    // Calculate stripe_select * height * fieldSize
    asm_code.push_back("lda.x");  // Load stripe_select from X (using implied addressing)

    int multiply_factor = ctx.height * ctx.fieldSize;
    if (multiply_factor == 4) {
        asm_code.push_back("asl");
        asm_code.push_back("asl");
    } else if (isPowerOfTwo(multiply_factor)) {
        int shifts = getLog2(multiply_factor);
        for (int i = 0; i < shifts; i++) {
            asm_code.push_back("asl");
        }
    } else {
        asm_code.push_back("tax");
        std::stringstream ss;
        ss << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (multiply_factor & 0xFF);
        asm_code.push_back("lda " + ss.str());
        if (multiply_factor > 255) {
            std::stringstream ss_hi;
            ss_hi << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << ((multiply_factor >> 8) & 0xFF);
            asm_code.push_back("ldy " + ss_hi.str());
            asm_code.push_back("mul.16 .ay");
        } else {
            asm_code.push_back("mul.8x");
        }
    }

    // Add row*fieldSize from Y
    asm_code.push_back("clc");
    asm_code.push_back("adc.16 .ay");  // A:X += Y (16-bit add)
    asm_code.push_back("tax");  // Save base offset in X

    // Step 3: Add column remainder offset: (col & mask) * stride_factor
    asm_code.push_back("lda " + ctx.zpCol);
    int mask = ctx.stripeWidth - 1;
    std::stringstream ss_mask;
    ss_mask << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (mask & 0xFF);
    asm_code.push_back("and " + ss_mask.str());

    // Multiply by stride_factor
    int stride_factor = ctx.strideFactor;
    if (stride_factor == 4) {
        asm_code.push_back("asl");
        asm_code.push_back("asl");
    } else if (isPowerOfTwo(stride_factor)) {
        int shifts = getLog2(stride_factor);
        for (int i = 0; i < shifts; i++) {
            asm_code.push_back("asl");
        }
    } else if (stride_factor > 0) {
        asm_code.push_back("ldy #$" + std::to_string(stride_factor & 0xFF));
        asm_code.push_back("mul.8y");
    }

    // Add to base offset and store result
    asm_code.push_back("clc");
    asm_code.push_back("adc.16 .tx");  // A:X += T:X

    // Add field region offset
    if (ctx.fieldRegionOffset > 0) {
        asm_code.push_back("clc");
        std::stringstream ss_field;
        ss_field << "#$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << ctx.fieldRegionOffset;
        asm_code.push_back("adc.16 " + ss_field.str());
    }

    // Store final result in zero-page
    asm_code.push_back("stax " + ctx.zpResult);

    return asm_code;
}

int FieldStripedOffsetCalc::calculateStrideFactor(int height, int fieldSize, int stripeWidth) {
    return (height * fieldSize) / stripeWidth;
}

int FieldStripedOffsetCalc::calculateFieldRegionOffset(
    const std::vector<int>& fieldSizes,
    int fieldIndex,
    int height,
    int width
) {
    int offset = 0;
    for (int i = 0; i < fieldIndex && i < (int)fieldSizes.size(); i++) {
        offset += height * width * fieldSizes[i];
    }
    return offset;
}

bool FieldStripedOffsetCalc::isPowerOfTwo(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

int FieldStripedOffsetCalc::getLog2(int powerOfTwo) {
    if (powerOfTwo <= 0) return 0;
    int log = 0;
    int val = powerOfTwo;
    while (val > 1) {
        val >>= 1;
        log++;
    }
    return log;
}

std::vector<std::string> FieldStripedOffsetCalc::generateShiftMultiply(
    const std::string& resultReg,
    int shiftAmount,
    const std::string& sourceReg
) {
    std::vector<std::string> asm_code;
    // Assuming source is already in sourceReg
    for (int i = 0; i < shiftAmount; i++) {
        asm_code.push_back("asl");  // Assuming sourceReg is A (accumulator)
    }
    return asm_code;
}

std::vector<std::string> FieldStripedOffsetCalc::generateMultiply(
    const std::string& resultReg,
    const std::string& lhsReg,
    const std::string& rhsValue
) {
    std::vector<std::string> asm_code;
    asm_code.push_back("tax");  // Move LHS to X
    asm_code.push_back("lda " + rhsValue);
    asm_code.push_back("mul.8x");  // A = A * X
    return asm_code;
}

std::vector<std::string> FieldStripedOffsetCalc::generateStripeSelect(
    const FieldStripedOffsetContext& ctx
) {
    std::vector<std::string> asm_code;
    // Calculate: col >> log2Stripe
    asm_code.push_back("lda " + ctx.zpCol);
    for (int i = 0; i < ctx.log2StripeWidth; i++) {
        asm_code.push_back("lsr");
    }
    return asm_code;
}

std::vector<std::string> FieldStripedOffsetCalc::generateBaseOffset(
    const FieldStripedOffsetContext& ctx
) {
    std::vector<std::string> asm_code;
    // This is a helper; actual implementation would calculate:
    // stripe_select * height * fieldSize + row * fieldSize
    return asm_code;
}

std::vector<std::string> FieldStripedOffsetCalc::generateColumnRemainderOffset(
    const FieldStripedOffsetContext& ctx
) {
    std::vector<std::string> asm_code;
    // This is a helper; actual implementation would calculate:
    // (col & mask) * stride_factor
    return asm_code;
}
