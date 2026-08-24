#include <cassert>
#include <iostream>
#include <cstdint>
#include <climits>
#include <limits>
#include <algorithm>

// Phase 19: Advanced Audio/Graphics Operators Tests
// Tests for saturating arithmetic, bit rotation, min/max, and saturating shift

// ============================================================================
// Test Helper: Simulate operator implementations
// ============================================================================

// Saturating addition
template<typename T>
T saturating_add(T a, T b) {
    if constexpr (std::is_signed_v<T>) {
        if (a > 0 && b > 0 && a > std::numeric_limits<T>::max() - b) {
            return std::numeric_limits<T>::max();
        }
        if (a < 0 && b < 0 && a < std::numeric_limits<T>::min() - b) {
            return std::numeric_limits<T>::min();
        }
    } else {
        if (a > std::numeric_limits<T>::max() - b) {
            return std::numeric_limits<T>::max();
        }
    }
    return a + b;
}

// Saturating subtraction
template<typename T>
T saturating_sub(T a, T b) {
    if constexpr (std::is_signed_v<T>) {
        if (a > 0 && b < 0 && a > std::numeric_limits<T>::max() + b) {
            return std::numeric_limits<T>::max();
        }
        if (a < 0 && b > 0 && a < std::numeric_limits<T>::min() + b) {
            return std::numeric_limits<T>::min();
        }
    } else {
        if (a < b) {
            return 0;
        }
    }
    return a - b;
}

// Saturating multiplication
template<typename T>
T saturating_mul(T a, T b) {
    if (a == 0 || b == 0) return 0;

    if constexpr (std::is_signed_v<T>) {
        int64_t result = static_cast<int64_t>(a) * static_cast<int64_t>(b);
        if (result > std::numeric_limits<T>::max()) {
            return std::numeric_limits<T>::max();
        }
        if (result < std::numeric_limits<T>::min()) {
            return std::numeric_limits<T>::min();
        }
        return static_cast<T>(result);
    } else {
        uint64_t result = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
        if (result > std::numeric_limits<T>::max()) {
            return std::numeric_limits<T>::max();
        }
        return static_cast<T>(result);
    }
}

// Bit rotation left
template<typename T>
T rotate_left(T x, int n) {
    constexpr int bits = sizeof(T) * 8;
    n = n % bits;
    return (x << n) | (x >> (bits - n));
}

// Bit rotation right
template<typename T>
T rotate_right(T x, int n) {
    constexpr int bits = sizeof(T) * 8;
    n = n % bits;
    return (x >> n) | (x << (bits - n));
}

// Min operator
template<typename T>
T min_op(T a, T b) {
    return std::min(a, b);
}

// Max operator
template<typename T>
T max_op(T a, T b) {
    return std::max(a, b);
}

// Saturating left shift
template<typename T>
T saturating_lshift(T a, int n) {
    if (n < 0 || n >= static_cast<int>(sizeof(T) * 8)) return a;

    if constexpr (std::is_signed_v<T>) {
        if (a > 0 && a > std::numeric_limits<T>::max() >> n) {
            return std::numeric_limits<T>::max();
        }
        if (a < 0 && a < std::numeric_limits<T>::min() >> n) {
            return std::numeric_limits<T>::min();
        }
    } else {
        if (a > std::numeric_limits<T>::max() >> n) {
            return std::numeric_limits<T>::max();
        }
    }
    return a << n;
}

// Saturating right shift (sign-preserving)
template<typename T>
T saturating_rshift(T a, int n) {
    if (n < 0 || n >= static_cast<int>(sizeof(T) * 8)) return a;
    return a >> n;
}

// ============================================================================
// Tests
// ============================================================================

// Test 1: Saturating addition - unsigned
void test_saturating_add_unsigned() {
    uint8_t a = 200, b = 100;
    uint8_t result = saturating_add(a, b);
    assert(result == 255);  // Clamped from 300

    uint8_t x = 50, y = 50;
    uint8_t result2 = saturating_add(x, y);
    assert(result2 == 100);  // Normal addition

    std::cout << "✓ Saturating addition (unsigned) test passed\n";
}

// Test 2: Saturating addition - signed
void test_saturating_add_signed() {
    int8_t a = 100, b = 50;
    int8_t result = saturating_add(a, b);
    assert(result == 127);  // Clamped from 150

    int8_t c = -100, d = -50;
    int8_t result2 = saturating_add(c, d);
    assert(result2 == -128);  // Clamped from -150

    int8_t e = 50, f = 50;
    int8_t result3 = saturating_add(e, f);
    assert(result3 == 100);  // Normal addition

    std::cout << "✓ Saturating addition (signed) test passed\n";
}

// Test 3: Saturating subtraction - unsigned
void test_saturating_sub_unsigned() {
    uint8_t a = 50, b = 100;
    uint8_t result = saturating_sub(a, b);
    assert(result == 0);  // Clamped from negative

    uint8_t x = 200, y = 50;
    uint8_t result2 = saturating_sub(x, y);
    assert(result2 == 150);  // Normal subtraction

    std::cout << "✓ Saturating subtraction (unsigned) test passed\n";
}

// Test 4: Saturating subtraction - signed
void test_saturating_sub_signed() {
    int8_t a = 100, b = -50;
    int8_t result = saturating_sub(a, b);
    assert(result == 127);  // Clamped from 150

    int8_t c = -100, d = 50;
    int8_t result2 = saturating_sub(c, d);
    assert(result2 == -128);  // Clamped from -150

    std::cout << "✓ Saturating subtraction (signed) test passed\n";
}

// Test 5: Saturating multiplication - unsigned
void test_saturating_mul_unsigned() {
    uint8_t a = 200, b = 2;
    uint8_t result = saturating_mul(a, b);
    assert(result == 255);  // Clamped from 400

    uint8_t x = 10, y = 10;
    uint8_t result2 = saturating_mul(x, y);
    assert(result2 == 100);  // Normal multiplication

    std::cout << "✓ Saturating multiplication (unsigned) test passed\n";
}

// Test 6: Saturating multiplication - signed
void test_saturating_mul_signed() {
    int8_t a = 100, b = 2;
    int8_t result = saturating_mul(a, b);
    assert(result == 127);  // Clamped from 200

    int8_t c = -100, d = 2;
    int8_t result2 = saturating_mul(c, d);
    assert(result2 == -128);  // Clamped from -200

    std::cout << "✓ Saturating multiplication (signed) test passed\n";
}

// Test 7: Bit rotation left
void test_rotate_left() {
    uint8_t x = 0xA5;  // 10100101
    uint8_t result = rotate_left(x, 1);
    assert(result == 0x4B);  // 01001011

    uint8_t y = 0x80;  // 10000000
    uint8_t result2 = rotate_left(y, 1);
    assert(result2 == 0x01);  // 00000001

    uint8_t z = 0xFF;  // 11111111
    uint8_t result3 = rotate_left(z, 4);
    assert(result3 == 0xFF);  // 11111111 (unchanged)

    std::cout << "✓ Bit rotation left test passed\n";
}

// Test 8: Bit rotation right
void test_rotate_right() {
    uint8_t x = 0xA5;  // 10100101
    uint8_t result = rotate_right(x, 1);
    assert(result == 0xD2);  // 11010010

    uint8_t y = 0x01;  // 00000001
    uint8_t result2 = rotate_right(y, 1);
    assert(result2 == 0x80);  // 10000000

    std::cout << "✓ Bit rotation right test passed\n";
}

// Test 9: Bit rotation - multi-bit
void test_rotate_multi_bit() {
    uint8_t x = 0x12;  // 00010010
    uint8_t result = rotate_left(x, 2);
    assert(result == 0x48);  // 01001000

    uint8_t y = 0x48;  // 01001000
    uint8_t result2 = rotate_right(y, 2);
    assert(result2 == 0x12);  // 00010010

    std::cout << "✓ Bit rotation (multi-bit) test passed\n";
}

// Test 10: Min/Max operators
void test_minmax_operators() {
    int a = 50, b = 30;
    assert(min_op(a, b) == 30);
    assert(max_op(a, b) == 50);

    uint8_t x = 200, y = 100;
    assert(min_op(x, y) == 100);
    assert(max_op(x, y) == 200);

    int c = -50, d = -30;
    assert(min_op(c, d) == -50);
    assert(max_op(c, d) == -30);

    std::cout << "✓ Min/Max operators test passed\n";
}

// Test 11: Min operator for clamping
void test_min_clamping() {
    uint8_t value = 200;  // Close to max
    uint8_t clamped = min_op(value, static_cast<uint8_t>(255));
    assert(clamped == 200);

    std::cout << "✓ Min operator clamping test passed\n";
}

// Test 12: Max operator for clamping
void test_max_clamping() {
    int8_t value = -100;  // Close to min
    int8_t clamped = max_op(value, static_cast<int8_t>(-128));
    assert(clamped == -100);

    std::cout << "✓ Max operator clamping test passed\n";
}

// Test 13: Saturating left shift - unsigned
void test_saturating_lshift_unsigned() {
    uint8_t a = 200;
    uint8_t result = saturating_lshift(a, 1);
    assert(result == 255);  // Clamped from 400

    uint8_t b = 50;
    uint8_t result2 = saturating_lshift(b, 1);
    assert(result2 == 100);  // Normal shift

    std::cout << "✓ Saturating left shift (unsigned) test passed\n";
}

// Test 14: Saturating left shift - signed
void test_saturating_lshift_signed() {
    int8_t a = 100;
    int8_t result = saturating_lshift(a, 1);
    assert(result == 127);  // Clamped from 200

    int8_t c = -100;
    int8_t result2 = saturating_lshift(c, 1);
    assert(result2 == -128);  // Clamped from -200

    std::cout << "✓ Saturating left shift (signed) test passed\n";
}

// Test 15: Saturating right shift
void test_saturating_rshift() {
    uint8_t a = 200;
    uint8_t result = saturating_rshift(a, 1);
    assert(result == 100);

    int8_t b = -100;
    int8_t result2 = saturating_rshift(b, 1);
    assert(result2 == -50);  // Sign-preserved

    std::cout << "✓ Saturating right shift test passed\n";
}

// Test 16: Audio mixing with saturation
void test_audio_mixing() {
    uint8_t ch1 = 180;  // Channel 1
    uint8_t ch2 = 150;  // Channel 2
    uint8_t mixed = saturating_add(ch1, ch2);
    assert(mixed == 255);  // Clamped, not 330

    std::cout << "✓ Audio mixing test passed\n";
}

// Test 17: Color component mixing
void test_color_mixing() {
    uint8_t red1 = 200;
    uint8_t red2 = 100;
    uint8_t result = saturating_add(red1, red2);
    assert(result == 255);

    std::cout << "✓ Color mixing test passed\n";
}

// Test 18: Sprite rotation
void test_sprite_rotation() {
    uint16_t sprite_bits = 0xAAAA;  // 1010101010101010
    uint16_t rotated = rotate_left(sprite_bits, 4);
    assert(rotated == 0xAAAA);  // Rotated pattern

    std::cout << "✓ Sprite rotation test passed\n";
}

// Test 19: Parameter clamping workflow
void test_parameter_clamping() {
    int frequency = 8000;  // User input
    int min_freq = 20;
    int max_freq = 20000;

    // Clamp to range
    int clamped = max_op(frequency, min_freq);
    clamped = min_op(clamped, max_freq);

    assert(clamped == 8000);

    int freq2 = 50000;
    int clamped2 = max_op(freq2, min_freq);
    clamped2 = min_op(clamped2, max_freq);
    assert(clamped2 == 20000);

    std::cout << "✓ Parameter clamping workflow test passed\n";
}

// Test 20: Combined operators in expression
void test_combined_operators() {
    uint8_t a = 100, b = 150;
    uint8_t mixed = saturating_add(a, b);  // 255
    uint8_t clamped = min_op(mixed, static_cast<uint8_t>(200));
    assert(clamped == 200);

    uint8_t rotated = rotate_left(clamped, 1);
    assert(rotated == 0x91);  // 11001000 rotated left 1 -> 10010001

    std::cout << "✓ Combined operators test passed\n";
}

int main() {
    std::cout << "\n=== Phase 19: Advanced Audio/Graphics Operators Tests ===\n";

    test_saturating_add_unsigned();
    test_saturating_add_signed();
    test_saturating_sub_unsigned();
    test_saturating_sub_signed();
    test_saturating_mul_unsigned();
    test_saturating_mul_signed();
    test_rotate_left();
    test_rotate_right();
    test_rotate_multi_bit();
    test_minmax_operators();
    test_min_clamping();
    test_max_clamping();
    test_saturating_lshift_unsigned();
    test_saturating_lshift_signed();
    test_saturating_rshift();
    test_audio_mixing();
    test_color_mixing();
    test_sprite_rotation();
    test_parameter_clamping();
    test_combined_operators();

    std::cout << "\n✅ All 20 Phase 19 advanced operators tests passed!\n";
    std::cout << "   Advanced operators ready for compiler implementation\n\n";

    return 0;
}
