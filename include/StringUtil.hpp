#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>

/**
 * Parses a numeric literal string into a uint32_t.
 * Supports:
 * - Hexadecimal: $prefix (e.g., $1234)
 * - Binary: %prefix (e.g., %1010)
 * - Decimal: no prefix (e.g., 1234)
 */
inline uint32_t parseNumericLiteral(const std::string& literal) {
    if (literal.empty()) throw std::runtime_error("Empty numeric literal");
    try {
        if (literal[0] == '$') return std::stoul(literal.substr(1), nullptr, 16);
        if (literal[0] == '%') return std::stoul(literal.substr(1), nullptr, 2);
        return std::stoul(literal, nullptr, 10);
    } catch (...) {
        throw std::runtime_error("Invalid numeric literal: '" + literal + "'");
    }
}
