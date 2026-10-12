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
 * - 0x / 0b prefixes, a leading '-' (two's complement)
 * The whole string must be the number ("1+2" is not a literal: the caller
 * evaluates it as an expression).
 */
inline uint32_t parseNumericLiteral(const std::string& literal) {
    if (literal.empty()) throw std::runtime_error("Empty numeric literal");
    std::string s = literal;
    bool neg = false;
    if (s[0] == '-') { neg = true; s = s.substr(1); }
    int base = 10;
    if (!s.empty() && s[0] == '$') { base = 16; s = s.substr(1); }
    else if (!s.empty() && s[0] == '%') { base = 2; s = s.substr(1); }
    else if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s = s.substr(2); }
    else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) { base = 2; s = s.substr(2); }
    size_t used = 0;
    unsigned long v = 0;
    try {
        v = std::stoul(s, &used, base);
    } catch (...) {
        throw std::runtime_error("Invalid numeric literal: '" + literal + "'");
    }
    if (used != s.size() || s.empty() || s[0] == '-' || s[0] == '+')
        throw std::runtime_error("Invalid numeric literal: '" + literal + "'");
    return neg ? (uint32_t)(0u - (uint32_t)v) : (uint32_t)v;
}
