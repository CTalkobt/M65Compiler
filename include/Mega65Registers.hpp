#pragma once
#include <cstdint>

namespace m65 {
    // Math Accelerator (Hardware Divider/Multiplier) — per MEGA65 VHDL (gs4510.vhdl)
    constexpr uint16_t MATH_BUSY_STATUS = 0xD70F; // bit 7 = divider busy, bit 6 = multiplier busy

    // Multiplier and Divider share input registers $D770/$D774
    // Writing to $D770-$D777 triggers division (div_start_over signal)
    // Multiplier is combinational (result available immediately)
    constexpr uint16_t MULT_ARG1        = 0xD770; // 0xD770-0xD773 (also dividend for divider)
    constexpr uint16_t MULT_ARG2        = 0xD774; // 0xD774-0xD777 (also divisor for divider)
    constexpr uint16_t MULT_RES         = 0xD778; // 0xD778-0xD77F (64-bit product)

    // Divider: inputs shared with multiplier, outputs at $D768/$D76C
    constexpr uint16_t DIV_ARG1         = 0xD770; // 0xD770-0xD773 (Dividend — same as MULT_ARG1)
    constexpr uint16_t DIV_ARG2         = 0xD774; // 0xD774-0xD777 (Divisor — same as MULT_ARG2)
    constexpr uint16_t DIV_FRAC         = 0xD768; // 0xD768-0xD76B (Fractional part of quotient)
    constexpr uint16_t DIV_RES          = 0xD76C; // 0xD76C-0xD76F (Integer quotient)
    // No hardware remainder register — compute as: dividend - quotient * divisor

    constexpr uint16_t MATH_SIGN        = 0xD76E; // Scratch byte for sign tracking in signed math ops

    // DMA Controller — per MEGA65 VHDL (gs4510.vhdl)
    // Writing $D700 (LSB) TRIGGERS DMA execution — must be written LAST
    constexpr uint16_t DMA_ADDR_LSB_TRIG = 0xD700; // DMA list address bits 0-7 (write triggers DMA)
    constexpr uint16_t DMA_ADDR_MSB      = 0xD701; // DMA list address bits 8-15
    constexpr uint16_t DMA_ADDR_BANK     = 0xD702; // DMA list address bank bits 16-22
    constexpr uint16_t DMA_MODE          = 0xD703; // DMA mode/control (bit 0 = F018B mode)
    constexpr uint16_t DMA_ADDR_MB       = 0xD704; // DMA list address megabyte bits 20-27
    constexpr uint16_t DMA_ETRIG         = 0xD705; // Enhanced trigger (LSB of flat 28-bit address)
    constexpr uint16_t DMA_ADDR_LSB_NOTRIG = 0xD70E; // DMA list address LSB WITHOUT triggering
}
