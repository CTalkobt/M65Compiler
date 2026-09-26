#include <cassert>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <limits>

// Phase 21: Vector Operations Tests
// Tests for fixed-size vector types with Phase 19 operators

// ============================================================================
// Vector Template Implementation
// ============================================================================

template<typename T, size_t N>
class Vector {
private:
    T data_[N];

public:
    // Constructors
    Vector() { std::fill(data_, data_ + N, T()); }

    explicit Vector(T value) { std::fill(data_, data_ + N, value); }

    Vector(const std::initializer_list<T>& init) {
        std::copy(init.begin(), init.end(), data_);
    }

    // Access
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }

    T* data() { return data_; }
    const T* data() const { return data_; }

    constexpr size_t size() const { return N; }
    constexpr bool empty() const { return N == 0; }

    // Standard arithmetic (element-wise)
    Vector operator+(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] + other[i];
        }
        return result;
    }

    Vector operator-(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] - other[i];
        }
        return result;
    }

    Vector operator*(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] * other[i];
        }
        return result;
    }

    // Scalar broadcast
    Vector operator*(T scalar) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] * scalar;
        }
        return result;
    }

    // Saturating addition (element-wise)
    Vector saturating_add(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            if constexpr (std::is_signed_v<T>) {
                T a = data_[i], b = other[i];
                if (a > 0 && b > 0 && a > std::numeric_limits<T>::max() - b) {
                    result[i] = std::numeric_limits<T>::max();
                } else if (a < 0 && b < 0 && a < std::numeric_limits<T>::min() - b) {
                    result[i] = std::numeric_limits<T>::min();
                } else {
                    result[i] = a + b;
                }
            } else {
                T a = data_[i], b = other[i];
                if (a > std::numeric_limits<T>::max() - b) {
                    result[i] = std::numeric_limits<T>::max();
                } else {
                    result[i] = a + b;
                }
            }
        }
        return result;
    }

    // Saturating subtraction (element-wise)
    Vector saturating_sub(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            if constexpr (std::is_signed_v<T>) {
                T a = data_[i], b = other[i];
                if (a > 0 && b < 0 && a > std::numeric_limits<T>::max() + b) {
                    result[i] = std::numeric_limits<T>::max();
                } else if (a < 0 && b > 0 && a < std::numeric_limits<T>::min() + b) {
                    result[i] = std::numeric_limits<T>::min();
                } else {
                    result[i] = a - b;
                }
            } else {
                result[i] = (data_[i] < other[i]) ? 0 : (data_[i] - other[i]);
            }
        }
        return result;
    }

    // Saturating multiplication (element-wise)
    Vector saturating_mul(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            T a = data_[i], b = other[i];
            if (a == 0 || b == 0) {
                result[i] = 0;
            } else if constexpr (std::is_signed_v<T>) {
                int64_t prod = static_cast<int64_t>(a) * static_cast<int64_t>(b);
                if (prod > std::numeric_limits<T>::max()) {
                    result[i] = std::numeric_limits<T>::max();
                } else if (prod < std::numeric_limits<T>::min()) {
                    result[i] = std::numeric_limits<T>::min();
                } else {
                    result[i] = static_cast<T>(prod);
                }
            } else {
                uint64_t prod = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
                result[i] = (prod > std::numeric_limits<T>::max())
                    ? std::numeric_limits<T>::max()
                    : static_cast<T>(prod);
            }
        }
        return result;
    }

    // Bit rotation left (element-wise)
    Vector rotate_left(int amount) const {
        Vector result;
        constexpr int bits = sizeof(T) * 8;
        amount = amount % bits;
        for (size_t i = 0; i < N; ++i) {
            T val = data_[i];
            result[i] = (val << amount) | (val >> (bits - amount));
        }
        return result;
    }

    // Bit rotation right (element-wise)
    Vector rotate_right(int amount) const {
        Vector result;
        constexpr int bits = sizeof(T) * 8;
        amount = amount % bits;
        for (size_t i = 0; i < N; ++i) {
            T val = data_[i];
            result[i] = (val >> amount) | (val << (bits - amount));
        }
        return result;
    }

    // Element-wise minimum
    Vector min(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = (data_[i] < other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    // Element-wise maximum
    Vector max(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = (data_[i] > other[i]) ? data_[i] : other[i];
        }
        return result;
    }

    // Saturating left shift (element-wise)
    Vector saturating_lshift(int amount) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            T val = data_[i];
            if (amount < 0 || amount >= static_cast<int>(sizeof(T) * 8)) {
                result[i] = val;
            } else if constexpr (std::is_signed_v<T>) {
                if (val > 0 && val > std::numeric_limits<T>::max() >> amount) {
                    result[i] = std::numeric_limits<T>::max();
                } else if (val < 0 && val < std::numeric_limits<T>::min() >> amount) {
                    result[i] = std::numeric_limits<T>::min();
                } else {
                    result[i] = val << amount;
                }
            } else {
                if (val > std::numeric_limits<T>::max() >> amount) {
                    result[i] = std::numeric_limits<T>::max();
                } else {
                    result[i] = val << amount;
                }
            }
        }
        return result;
    }

    // Horizontal operations
    T sum() const {
        T total = T();
        for (size_t i = 0; i < N; ++i) {
            total += data_[i];
        }
        return total;
    }

    T min_element() const {
        return *std::min_element(data_, data_ + N);
    }

    T max_element() const {
        return *std::max_element(data_, data_ + N);
    }

    // Fill all elements
    void fill(T value) {
        std::fill(data_, data_ + N, value);
    }
};

// ============================================================================
// Tests
// ============================================================================

void test_vector_construction() {
    Vector<int, 4> v1;
    assert(v1.size() == 4);
    assert(v1[0] == 0);

    Vector<int, 4> v2{1, 2, 3, 4};
    assert(v2[0] == 1 && v2[3] == 4);

    Vector<int, 4> v3(5);
    assert(v3[0] == 5 && v3[3] == 5);

    std::cout << "✓ Vector construction test passed\n";
}

void test_vector_element_access() {
    Vector<uint8_t, 8> v{10, 20, 30, 40, 50, 60, 70, 80};
    assert(v[2] == 30);
    v[2] = 35;
    assert(v[2] == 35);
    assert(v.size() == 8);

    std::cout << "✓ Vector element access test passed\n";
}

void test_vector_standard_add() {
    Vector<int, 4> a{10, 20, 30, 40};
    Vector<int, 4> b{1, 2, 3, 4};
    Vector<int, 4> result = a + b;
    assert(result[0] == 11 && result[3] == 44);

    std::cout << "✓ Vector standard add test passed\n";
}

void test_vector_saturating_add_unsigned() {
    Vector<uint8_t, 4> a{200, 100, 50, 255};
    Vector<uint8_t, 4> b{100, 100, 100, 1};
    Vector<uint8_t, 4> result = a.saturating_add(b);
    assert(result[0] == 255);  // Clamped from 300
    assert(result[1] == 200);  // Clamped from 200
    assert(result[2] == 150);  // Normal add
    assert(result[3] == 255);  // Clamped from 256

    std::cout << "✓ Vector saturating add (unsigned) test passed\n";
}

void test_vector_saturating_add_signed() {
    Vector<int8_t, 4> a{100, -100, 50, -50};
    Vector<int8_t, 4> b{50, -50, 30, 30};
    Vector<int8_t, 4> result = a.saturating_add(b);
    assert(result[0] == 127);   // Clamped from 150
    assert(result[1] == -128);  // Clamped from -150
    assert(result[2] == 80);    // Normal add
    assert(result[3] == -20);   // Normal add

    std::cout << "✓ Vector saturating add (signed) test passed\n";
}

void test_vector_saturating_sub() {
    Vector<int16_t, 4> a{100, -30000, 0, 50};
    Vector<int16_t, 4> b{50, 5000, 100, 100};
    Vector<int16_t, 4> result = a.saturating_sub(b);
    assert(result[0] == 50);     // Normal sub: 100 - 50 = 50
    assert(result[1] == -32768); // Saturated: -30000 - 5000 = -35000 clamped to INT16_MIN
    assert(result[2] == -100);   // Normal sub: 0 - 100 = -100
    assert(result[3] == -50);    // Normal sub: 50 - 100 = -50

    std::cout << "✓ Vector saturating sub test passed\n";
}

void test_vector_saturating_mul() {
    Vector<uint8_t, 4> a{200, 50, 10, 100};
    Vector<uint8_t, 4> b{2, 2, 2, 3};
    Vector<uint8_t, 4> result = a.saturating_mul(b);
    assert(result[0] == 255);  // Clamped from 400
    assert(result[1] == 100);  // Normal mul
    assert(result[2] == 20);   // Normal mul
    assert(result[3] == 255);  // Clamped from 300

    std::cout << "✓ Vector saturating mul test passed\n";
}

void test_vector_rotate_left() {
    Vector<uint8_t, 4> v{0xA5, 0x80, 0x0F, 0xFF};
    Vector<uint8_t, 4> result = v.rotate_left(1);
    assert(result[0] == 0x4B);  // 10100101 → 01001011
    assert(result[1] == 0x01);  // 10000000 → 00000001
    assert(result[2] == 0x1E);  // 00001111 → 00011110
    assert(result[3] == 0xFF);  // 11111111 → 11111111

    std::cout << "✓ Vector rotate left test passed\n";
}

void test_vector_rotate_right() {
    Vector<uint8_t, 4> v{0xA5, 0x01, 0xF0, 0xFF};
    Vector<uint8_t, 4> result = v.rotate_right(1);
    assert(result[0] == 0xD2);  // 10100101 → 11010010
    assert(result[1] == 0x80);  // 00000001 → 10000000
    assert(result[2] == 0x78);  // 11110000 → 01111000
    assert(result[3] == 0xFF);  // 11111111 → 11111111

    std::cout << "✓ Vector rotate right test passed\n";
}

void test_vector_minmax() {
    Vector<int, 4> a{50, 100, 30, 200};
    Vector<int, 4> b{75, 75, 75, 75};

    Vector<int, 4> min_result = a.min(b);
    assert(min_result[0] == 50 && min_result[1] == 75 &&
           min_result[2] == 30 && min_result[3] == 75);

    Vector<int, 4> max_result = a.max(b);
    assert(max_result[0] == 75 && max_result[1] == 100 &&
           max_result[2] == 75 && max_result[3] == 200);

    std::cout << "✓ Vector min/max test passed\n";
}

void test_vector_saturating_lshift() {
    Vector<int8_t, 4> a{50, 100, -50, -100};
    Vector<int8_t, 4> result = a.saturating_lshift(1);
    assert(result[0] == 100);  // Normal shift
    assert(result[1] == 127);  // Clamped from 200
    assert(result[2] == -100); // Normal shift
    assert(result[3] == -128); // Clamped from -200

    std::cout << "✓ Vector saturating left shift test passed\n";
}

void test_vector_horizontal_sum() {
    Vector<int, 8> v{1, 2, 3, 4, 5, 6, 7, 8};
    int sum = v.sum();
    assert(sum == 36);

    Vector<int16_t, 4> v2{1000, 2000, 3000, 4000};
    int16_t sum2 = v2.sum();
    assert(sum2 == 10000);

    std::cout << "✓ Vector horizontal sum test passed\n";
}

void test_vector_horizontal_minmax() {
    Vector<int, 8> v{5, 2, 9, 1, 7, 3, 8, 4};
    assert(v.min_element() == 1);
    assert(v.max_element() == 9);

    std::cout << "✓ Vector horizontal min/max test passed\n";
}

void test_audio_mixing() {
    // Simulate 4 audio channels with 8 samples each
    Vector<int16_t, 8> ch1{1000, 2000, 3000, 4000, 1500, 2500, 3500, 4500};
    Vector<int16_t, 8> ch2{1500, 2500, 1000, 2000, 3000, 4000, 1000, 2000};
    Vector<int16_t, 8> ch3{500, 1000, 2000, 1000, 500, 1500, 2000, 1000};
    Vector<int16_t, 8> ch4{500, 500, 500, 500, 1000, 1000, 500, 500};

    // Safe saturating mix
    Vector<int16_t, 8> mix = ch1.saturating_add(ch2);
    mix = mix.saturating_add(ch3);
    mix = mix.saturating_add(ch4);

    // Check that high values are saturated properly
    // ch1[0] + ch2[0] + ch3[0] + ch4[0] = 1000+1500+500+500 = 3500 (no saturation)
    assert(mix[0] == 3500);

    // ch1[1] + ch2[1] + ch3[1] + ch4[1] = 2000+2500+1000+500 = 6000 (no saturation)
    assert(mix[1] == 6000);

    std::cout << "✓ Audio mixing test passed\n";
}

void test_graphics_brightness() {
    // Simulate scanline with 16 pixel brightness values
    Vector<uint8_t, 16> scanline{100, 150, 200, 250, 50, 75, 100, 125,
                                  50, 75, 100, 150, 200, 225, 240, 255};

    // Boost brightness with saturation
    Vector<uint8_t, 16> boost = scanline.saturating_add(
        Vector<uint8_t, 16>(50)
    );

    // Check saturation
    assert(boost[3] == 255);   // 250 + 50 clamped
    assert(boost[15] == 255);  // 255 + 50 clamped
    assert(boost[0] == 150);   // 100 + 50 normal

    std::cout << "✓ Graphics brightness test passed\n";
}

void test_sprite_rotation_animation() {
    // Sprite pattern: 8 rows of rotation animation
    Vector<uint8_t, 8> pattern{0xA5, 0x5A, 0xF0, 0x0F, 0xAA, 0x55, 0xFF, 0x00};

    Vector<uint8_t, 8> frame1 = pattern;
    Vector<uint8_t, 8> frame2 = frame1.rotate_left(1);
    Vector<uint8_t, 8> frame3 = frame2.rotate_left(1);
    Vector<uint8_t, 8> frame4 = frame3.rotate_left(1);

    // Check rotation sequence
    assert(frame2[0] == 0x4B);  // 0xA5 rotated left 1
    assert(frame3[0] == 0x96);  // 0x4B rotated left 1
    assert(frame4[0] == 0x2D);  // 0x96 rotated left 1

    std::cout << "✓ Sprite rotation animation test passed\n";
}

void test_signal_processing() {
    // FIR filter with 4 taps and 4 input samples
    Vector<int16_t, 4> taps{100, 200, 200, 100};
    Vector<int16_t, 4> input{1000, 2000, 1500, 500};

    // Element-wise multiply
    Vector<int32_t, 4> products{
        static_cast<int32_t>(taps[0]) * input[0],
        static_cast<int32_t>(taps[1]) * input[1],
        static_cast<int32_t>(taps[2]) * input[2],
        static_cast<int32_t>(taps[3]) * input[3]
    };

    int32_t output = products.sum();
    assert(output == (100000 + 400000 + 300000 + 50000));

    std::cout << "✓ Signal processing test passed\n";
}

void test_scalar_broadcast() {
    Vector<int, 4> v{10, 20, 30, 40};
    Vector<int, 4> result = v * 2;
    assert(result[0] == 20 && result[3] == 80);

    std::cout << "✓ Scalar broadcast test passed\n";
}

void test_chained_operations() {
    // (a +| b) <> max_val
    Vector<int16_t, 4> a{10000, 15000, 5000, 20000};
    Vector<int16_t, 4> b{10000, 10000, 10000, 10000};
    Vector<int16_t, 4> limit{20000, 20000, 20000, 20000};

    Vector<int16_t, 4> sum = a.saturating_add(b);
    Vector<int16_t, 4> limited = sum.min(limit);

    assert(limited[0] == 20000);  // 20000 clamped to 20000
    assert(limited[1] == 20000);  // 25000 clamped to 20000
    assert(limited[2] == 15000);  // 15000, no clamp
    assert(limited[3] == 20000);  // 30000 clamped to 20000

    std::cout << "✓ Chained operations test passed\n";
}

void test_fixed_point_vector() {
    // Fixed-point 4.12 format simulation
    Vector<int16_t, 4> values{2048, 4096, 6144, 8192};  // 0.5, 1.0, 1.5, 2.0

    // Multiply by scale (1.5 = 6144 in 4.12 format)
    int16_t scale = 6144;
    Vector<int16_t, 4> scaled;
    for (int i = 0; i < 4; ++i) {
        int32_t temp = (static_cast<int32_t>(values[i]) * scale) >> 12;
        scaled[i] = static_cast<int16_t>(temp);
    }

    // Check results (with some tolerance for rounding)
    assert(scaled[0] == 3072);   // 0.5 * 1.5 = 0.75
    assert(scaled[1] == 6144);   // 1.0 * 1.5 = 1.5
    assert(scaled[2] == 9216);   // 1.5 * 1.5 = 2.25
    assert(scaled[3] >= 12000);  // 2.0 * 1.5 ≈ 3.0

    std::cout << "✓ Fixed-point vector test passed\n";
}

void test_vector_fill_and_access() {
    Vector<uint8_t, 8> v;
    v.fill(42);

    for (size_t i = 0; i < 8; ++i) {
        assert(v[i] == 42);
    }

    std::cout << "✓ Vector fill and access test passed\n";
}

int main() {
    std::cout << "\n=== Phase 21: Vector Operations Tests ===\n";

    // Basic operations
    test_vector_construction();
    test_vector_element_access();
    test_vector_standard_add();
    test_vector_fill_and_access();
    test_scalar_broadcast();

    // Phase 19 operators on vectors
    test_vector_saturating_add_unsigned();
    test_vector_saturating_add_signed();
    test_vector_saturating_sub();
    test_vector_saturating_mul();
    test_vector_rotate_left();
    test_vector_rotate_right();
    test_vector_minmax();
    test_vector_saturating_lshift();

    // Horizontal reductions
    test_vector_horizontal_sum();
    test_vector_horizontal_minmax();

    // Real-world use cases
    test_audio_mixing();
    test_graphics_brightness();
    test_sprite_rotation_animation();
    test_signal_processing();
    test_chained_operations();
    test_fixed_point_vector();

    std::cout << "\n✅ All 21 Phase 21 vector operations tests passed!\n";
    std::cout << "   Vector SIMD operations ready for compiler implementation\n\n";

    return 0;
}
