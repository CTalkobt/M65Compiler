#pragma once
#include <cstdint>

namespace m65 {
    // Math Accelerator (Hardware Divider/Multiplier)
    constexpr uint16_t MATH_CONTROL     = 0xD700; // and DMA control
    constexpr uint16_t MATH_BUSY_STATUS = 0xD70F; // bit 7 set if busy
    
    // 32-bit Multiplier
    constexpr uint16_t MULT_ARG1        = 0xD770; // 0xD770-0xD773
    constexpr uint16_t MULT_ARG2        = 0xD774; // 0xD774-0xD777
    constexpr uint16_t MULT_RES         = 0xD778; // 0xD778-0xD77B
    
    // 32-bit Divider
    constexpr uint16_t DIV_ARG1         = 0xD760; // 0xD760-0xD763
    constexpr uint16_t DIV_ARG2         = 0xD764; // 0xD764-0xD767
    constexpr uint16_t DIV_RES          = 0xD768; // 0xD768-0xD76B (Quotient)
    constexpr uint16_t DIV_REM          = 0xD76C; // 0xD76C-0xD76F (Remainder)
    
    constexpr uint16_t MATH_SIGN        = 0xD76E;

    // DMA Controller
    constexpr uint16_t DMA_CONTROL      = 0xD700;
    constexpr uint16_t DMA_ADDR_LO      = 0xD701;
    constexpr uint16_t DMA_ADDR_MI      = 0xD702;
    constexpr uint16_t DMA_ADDR_HI      = 0xD703;
    constexpr uint16_t DMA_ADDR_MB      = 0xD704;
    constexpr uint16_t DMA_ETRIG        = 0xD705;
}
