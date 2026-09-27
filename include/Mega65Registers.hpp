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

    // DMA Controller
    constexpr uint16_t DMA_CONTROL      = 0xD700;
    constexpr uint16_t DMA_ADDR_LO      = 0xD701;
    constexpr uint16_t DMA_ADDR_MI      = 0xD702;
    constexpr uint16_t DMA_ADDR_HI      = 0xD703;
    constexpr uint16_t DMA_ADDR_MB      = 0xD704;
    constexpr uint16_t DMA_ETRIG        = 0xD705;
}
