#pragma once

#include <string>
#include <vector>

// Assembly templates for address calculation optimization
// These templates encode the most efficient 6502/45GS02 sequences
// for common addressing patterns, avoiding naive arithmetic expansion

enum class AddressTemplateType {
    LINEAR_ROW_MAJOR_40,     // row * 40 + col (text screen)
    LINEAR_ROW_MAJOR_80,     // row * 80 + col
    LINEAR_ROW_MAJOR_160,    // row * 160 + col
    LINEAR_ROW_MAJOR_320,    // row * 320 + col (bitmap)
    LINEAR_ROW_MAJOR_640,    // row * 640 + col
    SPRITE_OFFSET_3,         // base + index * 3
    SPRITE_OFFSET_8,         // base + index * 8
    SPRITE_OFFSET_16,        // base + index * 16
    SPRITE_OFFSET_32,        // base + index * 32
    SPRITE_OFFSET_256,       // base + index * 256 (fast case)
    CUMULATIVE_STRIDE,       // base + row*stride_x + col*stride_y
};

// A single assembly template with operand placeholders
struct AddressTemplate {
    AddressTemplateType type;
    const char* name;           // Human-readable name (e.g., "text_screen_40")
    const char* description;    // What this template optimizes

    // Template assembly code with %0, %1, %2 placeholders for operands
    // Example: "lda %0\nasl\nasl\nasl\nsta temp1" where %0 is row variable
    const char* asmTemplate;

    int estimatedCycles;        // Cycle count for this template (vs naive)
    int estimatedBytes;         // Byte count for this template (vs naive)
    int naiveCycles;            // Cycle count for naive expansion (reference)
    int naiveBytes;             // Byte count for naive expansion (reference)
};

// Get a template by type
const AddressTemplate* getAddressTemplate(AddressTemplateType type);

// Template library functions
namespace AddressTemplateLibrary {
    // Linear row-major templates (row * WIDTH + col)
    extern const AddressTemplate textScreen40;    // 40-column text mode
    extern const AddressTemplate linearRow80;     // 80-column
    extern const AddressTemplate linearRow160;    // 160-column bitmap
    extern const AddressTemplate linearRow320;    // 320-column bitmap
    extern const AddressTemplate linearRow640;    // 640-column bitmap

    // Sprite offset templates (base + index * SIZE)
    extern const AddressTemplate spriteOffset3;   // Char-sized (3 bytes)
    extern const AddressTemplate spriteOffset8;   // Small sprite (8 bytes)
    extern const AddressTemplate spriteOffset16;  // Medium sprite (16 bytes)
    extern const AddressTemplate spriteOffset32;  // Large sprite (32 bytes)
    extern const AddressTemplate spriteOffset256; // Full page (256 bytes)

    // Cumulative stride template (base + row*X_STRIDE + col*Y_STRIDE)
    extern const AddressTemplate cumulativeStride;
}

// Operand substitution: replaces %0, %1, %2 in template with actual variable names
std::string substituteOperands(const char* templateAsm,
                               const std::vector<std::string>& operands);
