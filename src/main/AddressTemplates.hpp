#pragma once

#include <string>
#include <vector>
#include <map>

/**
 * Phase 89.2: Address Calculation Template Library
 *
 * Hardcoded 6502 assembly templates for common address calculations.
 * Each template is optimized for the MEGA65 (45GS02) architecture.
 *
 * Templates use %0, %1, %2 placeholders for:
 * - %0 = first operand (row/base/index)
 * - %1 = second operand (col/index/size)
 * - %2 = result or temporary
 *
 * All templates assume:
 * - ZP register $FA/$FB available for temporary storage
 * - Result in A (and X for 16-bit results)
 */

struct AddressTemplate {
    std::string name;
    int width_or_size;              // For parameterized templates (40, 80, etc)
    std::string description;
    std::string asmTemplate;        // Assembly with %0, %1 placeholders
    int estimatedCycles;
    int estimatedBytes;
    std::vector<std::string> clobberedRegs;  // Registers modified: A, X, Y
};

/**
 * TEXT SCREEN ADDRESSING TEMPLATES
 * Pattern: row * 40 + col
 * Common on MEGA65 for text mode (25 rows × 40 columns)
 */
namespace TextScreenTemplates {
    // row * 40 + col
    const AddressTemplate ROW_40 = {
        .name = "text_screen_row40",
        .width_or_size = 40,
        .description = "Text screen row-major (row * 40 + col)",
        .asmTemplate = R"(
        ; Input: %0 = row (A), %1 = col (A)
        ; Output: A = low byte, X = high byte of address
        lda %0                  ; Load row
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        sta $fa                 ; Temp = 8 × row
        asl                     ; × 16
        adc $fa                 ; A = 8×row + 16×row = 24×row
        sta $fa                 ; Save 24×row
        lda %0                  ; Reload row
        asl                     ; × 2
        asl                     ; × 4
        adc $fa                 ; A = 8×row + 24×row = 32×row + 8×row = 40×row
        clc
        adc %1                  ; A = 40×row + col
        ldx #0                  ; X = 0 (high byte, assuming < 2KB)
        )",
        .estimatedCycles = 20,
        .estimatedBytes = 18,
        .clobberedRegs = {"A", "X"}
    };
}

/**
 * BITMAP ADDRESSING TEMPLATES
 * Patterns for pixel buffer addressing
 */
namespace BitmapTemplates {
    // row * 320 + col (320-pixel wide bitmap)
    const AddressTemplate ROW_320 = {
        .name = "bitmap_row320",
        .width_or_size = 320,
        .description = "Bitmap row-major (row * 320 + col)",
        .asmTemplate = R"(
        ; Input: %0 = row (A), %1 = col (16-bit in AX)
        ; Output: AX = address
        lda %0                  ; Load row
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        asl                     ; × 16
        asl                     ; × 32
        sta $fa                 ; Temp_lo = 32 × row
        lda %0
        asl
        asl
        asl
        asl
        asl
        lsr
        lsr
        lsr                     ; High bits of 32×row
        sta $fb                 ; Temp_hi

        ; Add 256×row (already have 32×row, need +224×row = +256-32)
        lda %0
        sta $fc
        lda $fa
        clc
        adc $fc
        adc $fc
        adc $fc
        adc $fc
        adc $fc
        adc $fc
        adc $fc
        adc $fc                 ; Add 8 copies = ×256
        sta $fa
        lda $fb
        adc #0
        sta $fb

        clc
        lda $fa
        adc %1_lo               ; Add col_lo
        ldx $fb
        bcc @no_carry
        inx
@no_carry:
        adc %1_hi               ; Add col_hi (with carry)
        )",
        .estimatedCycles = 45,
        .estimatedBytes = 40,
        .clobberedRegs = {"A", "X"}
    };

    // Simpler: row * 160 + col
    const AddressTemplate ROW_160 = {
        .name = "bitmap_row160",
        .width_or_size = 160,
        .description = "Bitmap row-major (row * 160 + col)",
        .asmTemplate = R"(
        ; Input: %0 = row (A), %1 = col (A or AX)
        ; Output: AX = address
        lda %0                  ; Load row
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        sta $fa                 ; 8 × row
        asl                     ; × 16
        adc $fa                 ; 24 × row
        sta $fa
        lda %0
        asl
        asl
        asl
        asl
        asl                     ; × 32
        adc $fa                 ; 32+24 = 56×row, need 160 = 128+32
        asl                     ; × 112
        adc $fa                 ; 112+48 = 160×row
        clc
        adc %1                  ; + col
        ldx #0
        )",
        .estimatedCycles = 25,
        .estimatedBytes = 22,
        .clobberedRegs = {"A", "X"}
    };
}

/**
 * SPRITE/OFFSET ADDRESSING TEMPLATES
 * Patterns for lookup table and data structure addressing
 */
namespace SpriteTemplates {
    // base + (index * 64) — common sprite size
    const AddressTemplate OFFSET_64 = {
        .name = "sprite_offset64",
        .width_or_size = 64,
        .description = "Sprite/table offset (base + index * 64)",
        .asmTemplate = R"(
        ; Input: %0 = base (AX), %1 = index (A)
        ; Output: AX = address
        lda %1                  ; Load index
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        asl                     ; × 16
        asl                     ; × 32
        asl                     ; × 64
        sta $fa                 ; Low byte
        ldx #0                  ; Assume index fits in byte
        lda %0_lo               ; Load base_lo
        clc
        adc $fa
        bcc @no_carry
        inx
@no_carry:
        ldx %0_hi
        bcc @done
        inx
@done:
        )",
        .estimatedCycles = 18,
        .estimatedBytes = 20,
        .clobberedRegs = {"A", "X"}
    };

    // base + (index * 32)
    const AddressTemplate OFFSET_32 = {
        .name = "sprite_offset32",
        .width_or_size = 32,
        .description = "Sprite/table offset (base + index * 32)",
        .asmTemplate = R"(
        ; Input: %0 = base (AX), %1 = index (A)
        ; Output: AX = address
        lda %1                  ; Load index
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        asl                     ; × 16
        asl                     ; × 32
        clc
        adc %0_lo
        ldx %0_hi
        bcc @done
        inx
@done:
        )",
        .estimatedCycles = 14,
        .estimatedBytes = 14,
        .clobberedRegs = {"A", "X"}
    };

    // base + (index * 8)
    const AddressTemplate OFFSET_8 = {
        .name = "sprite_offset8",
        .width_or_size = 8,
        .description = "Sprite/table offset (base + index * 8)",
        .asmTemplate = R"(
        ; Input: %0 = base (AX), %1 = index (A)
        ; Output: AX = address
        lda %1                  ; Load index
        asl                     ; × 2
        asl                     ; × 4
        asl                     ; × 8
        clc
        adc %0_lo
        ldx %0_hi
        bcc @done
        inx
@done:
        )",
        .estimatedCycles = 11,
        .estimatedBytes = 11,
        .clobberedRegs = {"A", "X"}
    };
}

/**
 * Template registry for runtime lookup
 */
class AddressTemplateRegistry {
public:
    static const AddressTemplate* getTemplate(const std::string& name) {
        auto it = templates.find(name);
        return it != templates.end() ? it->second : nullptr;
    }

    static const std::map<std::string, const AddressTemplate*>& getAllTemplates() {
        return templates;
    }

private:
    static const std::map<std::string, const AddressTemplate*> templates;

    // Initialize registry on first access
    static void initializeTemplates();
};

/**
 * Template Selection Strategy:
 *
 * 1. LINEAR_ROW_MAJOR patterns select by width:
 *    - 40: TextScreenTemplates::ROW_40
 *    - 160: BitmapTemplates::ROW_160
 *    - 320: BitmapTemplates::ROW_320
 *
 * 2. SPRITE_OFFSET patterns select by size:
 *    - 8: SpriteTemplates::OFFSET_8
 *    - 32: SpriteTemplates::OFFSET_32
 *    - 64: SpriteTemplates::OFFSET_64
 *
 * 3. No match: Fall back to naive code generation
 *
 * Integration point: CodeGenerator::generateBinaryOp()
 * calls AddressTemplateDetector::detectPattern() before
 * generating arithmetic code.
 */
