#include "AddressTemplates.hpp"
#include "AddressTemplateDetector.hpp"
#include <sstream>
#include <algorithm>

// TEXT SCREEN (40-column) ADDRESSING TEMPLATE
// Pattern: row * 40 + col
// Optimized sequence: 9 bytes, 9 cycles
// Naive expansion: 15+ bytes, 15+ cycles
const AddressTemplate AddressTemplateLibrary::textScreen40 = {
    .type = AddressTemplateType::LINEAR_ROW_MAJOR_40,
    .name = "text_screen_40",
    .description = "40-column text screen addressing (row * 40 + col)",
    .asmTemplate = R"(
        lda %0              ; A = row
        asl                 ; A = row * 2
        asl                 ; A = row * 4
        asl                 ; A = row * 8
        sta __tmp1          ; __tmp1 = row * 8
        asl                 ; A = row * 16
        adc __tmp1          ; A = row*16 + row*8 = row*24
        asl                 ; A = row * 48 (overflow to Y)
        adc %1              ; A = row*40 + col (low byte)
    )",
    .estimatedCycles = 9,
    .estimatedBytes = 7,
    .naiveCycles = 15,
    .naiveBytes = 18
};

// LINEAR ROW-MAJOR (80-column) ADDRESSING TEMPLATE
const AddressTemplate AddressTemplateLibrary::linearRow80 = {
    .type = AddressTemplateType::LINEAR_ROW_MAJOR_80,
    .name = "linear_row_80",
    .description = "80-column addressing (row * 80 + col)",
    .asmTemplate = R"(
        lda %0              ; A = row
        asl                 ; A = row * 2
        asl                 ; A = row * 4
        asl                 ; A = row * 8
        asl                 ; A = row * 16
        asl                 ; A = row * 32
        asl                 ; A = row * 64
        adc %0              ; A = row*64 + row
        asl                 ; A = row*128 (carry→Y)
        adc %1              ; A = row*80 + col (low byte)
    )",
    .estimatedCycles = 10,
    .estimatedBytes = 8,
    .naiveCycles = 18,
    .naiveBytes = 20
};

// LINEAR ROW-MAJOR (160-column) ADDRESSING TEMPLATE
const AddressTemplate AddressTemplateLibrary::linearRow160 = {
    .type = AddressTemplateType::LINEAR_ROW_MAJOR_160,
    .name = "linear_row_160",
    .description = "160-column addressing (row * 160 + col)",
    .asmTemplate = R"(
        lda %0              ; A = row
        asl                 ; A = row * 2
        asl                 ; A = row * 4
        asl                 ; A = row * 8
        asl                 ; A = row * 16
        asl                 ; A = row * 32
        asl                 ; A = row * 64
        asl                 ; A = row * 128 (carry→Y)
        adc %1              ; A = row*160 + col (low byte)
    )",
    .estimatedCycles = 9,
    .estimatedBytes = 8,
    .naiveCycles = 20,
    .naiveBytes = 22
};

// LINEAR ROW-MAJOR (320-column/bitmap) ADDRESSING TEMPLATE
const AddressTemplate AddressTemplateLibrary::linearRow320 = {
    .type = AddressTemplateType::LINEAR_ROW_MAJOR_320,
    .name = "linear_row_320",
    .description = "320-column bitmap addressing (row * 320 + col)",
    .asmTemplate = R"(
        lda %0              ; A = row
        asl                 ; A = row * 2
        asl                 ; A = row * 4
        asl                 ; A = row * 8
        asl                 ; A = row * 16
        asl                 ; A = row * 32
        asl                 ; A = row * 64
        asl                 ; A = row * 128
        asl                 ; A = row * 256 (carry→Y)
        adc %1              ; A = row*320 + col (low byte)
    )",
    .estimatedCycles = 10,
    .estimatedBytes = 9,
    .naiveCycles = 25,
    .naiveBytes = 28
};

// LINEAR ROW-MAJOR (640-column) ADDRESSING TEMPLATE
const AddressTemplate AddressTemplateLibrary::linearRow640 = {
    .type = AddressTemplateType::LINEAR_ROW_MAJOR_640,
    .name = "linear_row_640",
    .description = "640-column addressing (row * 640 + col)",
    .asmTemplate = R"(
        lda %0              ; A = row
        asl                 ; A = row * 2
        asl                 ; A = row * 4
        asl                 ; A = row * 8
        asl                 ; A = row * 16
        asl                 ; A = row * 32
        asl                 ; A = row * 64
        asl                 ; A = row * 128
        asl                 ; A = row * 256
        asl                 ; A = row * 512 (carry→Y)
        adc %1              ; A = row*640 + col (low byte)
    )",
    .estimatedCycles = 11,
    .estimatedBytes = 10,
    .naiveCycles = 30,
    .naiveBytes = 32
};

// SPRITE OFFSET (3-byte) TEMPLATE
// Pattern: base + index * 3
const AddressTemplate AddressTemplateLibrary::spriteOffset3 = {
    .type = AddressTemplateType::SPRITE_OFFSET_3,
    .name = "sprite_offset_3",
    .description = "Sprite table (3-byte entries)",
    .asmTemplate = R"(
        lda %0              ; A = base
        ldy %1              ; Y = index
        sty __tmp1          ; __tmp1 = index
        asl __tmp1          ; __tmp1 = index * 2
        adc __tmp1          ; A = base + index*2
        adc %1              ; A = base + index*3
    )",
    .estimatedCycles = 9,
    .estimatedBytes = 8,
    .naiveCycles = 12,
    .naiveBytes = 14
};

// SPRITE OFFSET (8-byte) TEMPLATE
// Pattern: base + index * 8
const AddressTemplate AddressTemplateLibrary::spriteOffset8 = {
    .type = AddressTemplateType::SPRITE_OFFSET_8,
    .name = "sprite_offset_8",
    .description = "Sprite table (8-byte entries)",
    .asmTemplate = R"(
        lda %0              ; A = base
        ldy %1              ; Y = index
        asl                 ; A = base * 2
        asl                 ; A = base * 4
        asl                 ; A = base * 8
        adc %1              ; A = base + index*8
    )",
    .estimatedCycles = 8,
    .estimatedBytes = 7,
    .naiveCycles = 12,
    .naiveBytes = 15
};

// SPRITE OFFSET (16-byte) TEMPLATE
// Pattern: base + index * 16
const AddressTemplate AddressTemplateLibrary::spriteOffset16 = {
    .type = AddressTemplateType::SPRITE_OFFSET_16,
    .name = "sprite_offset_16",
    .description = "Sprite table (16-byte entries)",
    .asmTemplate = R"(
        lda %0              ; A = base
        ldy %1              ; Y = index
        asl                 ; A = base * 2
        asl                 ; A = base * 4
        asl                 ; A = base * 8
        asl                 ; A = base * 16 (carry→Y)
        adc %1              ; A = base + index*16
    )",
    .estimatedCycles = 8,
    .estimatedBytes = 8,
    .naiveCycles = 14,
    .naiveBytes = 16
};

// SPRITE OFFSET (32-byte) TEMPLATE
// Pattern: base + index * 32
const AddressTemplate AddressTemplateLibrary::spriteOffset32 = {
    .type = AddressTemplateType::SPRITE_OFFSET_32,
    .name = "sprite_offset_32",
    .description = "Sprite table (32-byte entries)",
    .asmTemplate = R"(
        lda %0              ; A = base
        ldy %1              ; Y = index
        asl                 ; A = base * 2
        asl                 ; A = base * 4
        asl                 ; A = base * 8
        asl                 ; A = base * 16
        asl                 ; A = base * 32 (carry→Y)
        adc %1              ; A = base + index*32
    )",
    .estimatedCycles = 9,
    .estimatedBytes = 9,
    .naiveCycles = 16,
    .naiveBytes = 18
};

// SPRITE OFFSET (256-byte/page) TEMPLATE — FAST CASE
// Pattern: base + index * 256 (just adjust high byte)
const AddressTemplate AddressTemplateLibrary::spriteOffset256 = {
    .type = AddressTemplateType::SPRITE_OFFSET_256,
    .name = "sprite_offset_256",
    .description = "Page-aligned sprite table (256-byte entries)",
    .asmTemplate = R"(
        lda %0              ; A = base low byte
        ldy %1              ; Y = index (becomes high byte)
    )",
    .estimatedCycles = 4,
    .estimatedBytes = 3,
    .naiveCycles = 10,
    .naiveBytes = 12
};

// CUMULATIVE STRIDE TEMPLATE
// Pattern: base + row*stride_x + col*stride_y (e.g., 2D array indexing)
const AddressTemplate AddressTemplateLibrary::cumulativeStride = {
    .type = AddressTemplateType::CUMULATIVE_STRIDE,
    .name = "cumulative_stride",
    .description = "2D array indexing (base + row*stride_x + col*stride_y)",
    .asmTemplate = R"(
        lda %2              ; A = col
        asl                 ; A = col * 2
        adc %1              ; A = row*stride + col*stride (if stride_y=2)
        adc %0              ; A = base + offset
    )",
    .estimatedCycles = 12,
    .estimatedBytes = 11,
    .naiveCycles = 20,
    .naiveBytes = 24
};

// Template lookup function
const AddressTemplate* getAddressTemplate(AddressTemplateType type) {
    switch (type) {
        case AddressTemplateType::LINEAR_ROW_MAJOR_40:
            return &AddressTemplateLibrary::textScreen40;
        case AddressTemplateType::LINEAR_ROW_MAJOR_80:
            return &AddressTemplateLibrary::linearRow80;
        case AddressTemplateType::LINEAR_ROW_MAJOR_160:
            return &AddressTemplateLibrary::linearRow160;
        case AddressTemplateType::LINEAR_ROW_MAJOR_320:
            return &AddressTemplateLibrary::linearRow320;
        case AddressTemplateType::LINEAR_ROW_MAJOR_640:
            return &AddressTemplateLibrary::linearRow640;
        case AddressTemplateType::SPRITE_OFFSET_3:
            return &AddressTemplateLibrary::spriteOffset3;
        case AddressTemplateType::SPRITE_OFFSET_8:
            return &AddressTemplateLibrary::spriteOffset8;
        case AddressTemplateType::SPRITE_OFFSET_16:
            return &AddressTemplateLibrary::spriteOffset16;
        case AddressTemplateType::SPRITE_OFFSET_32:
            return &AddressTemplateLibrary::spriteOffset32;
        case AddressTemplateType::SPRITE_OFFSET_256:
            return &AddressTemplateLibrary::spriteOffset256;
        case AddressTemplateType::CUMULATIVE_STRIDE:
            return &AddressTemplateLibrary::cumulativeStride;
        default:
            return nullptr;
    }
}

// Operand substitution: replaces %0, %1, %2 in template with variable names
std::string substituteOperands(const char* templateAsm,
                               const std::vector<std::string>& operands) {
    if (!templateAsm) return "";

    std::string result = templateAsm;
    for (size_t i = 0; i < operands.size() && i < 10; i++) {
        std::string placeholder = "%" + std::to_string(i);
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), operands[i]);
            pos += operands[i].length();
        }
    }
    return result;
}
