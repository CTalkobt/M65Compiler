#include <cassert>
#include <iostream>
#include <cstdint>

// Phase 20: Operator Overloading Tests
// Tests demonstrating operator overloading for Phase 19 operators

// ============================================================================
// Test Struct 1: Fixed-Point Number with Saturating Arithmetic
// ============================================================================

struct Fixed {
    int16_t value;

    // Saturating addition
    Fixed operator_plus_sat(Fixed other) const {
        int32_t result = static_cast<int32_t>(this->value) +
                        static_cast<int32_t>(other.value);
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return Fixed{static_cast<int16_t>(result)};
    }

    // Saturating subtraction
    Fixed operator_minus_sat(Fixed other) const {
        int32_t result = static_cast<int32_t>(this->value) -
                        static_cast<int32_t>(other.value);
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return Fixed{static_cast<int16_t>(result)};
    }

    // Saturating multiplication
    Fixed operator_mul_sat(Fixed other) const {
        int32_t result = static_cast<int32_t>(this->value) *
                        static_cast<int32_t>(other.value);
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return Fixed{static_cast<int16_t>(result)};
    }

    // Min operator
    Fixed operator_min(Fixed other) const {
        return this->value < other.value ? *this : other;
    }

    // Max operator
    Fixed operator_max(Fixed other) const {
        return this->value > other.value ? *this : other;
    }
};

// ============================================================================
// Test Struct 2: Audio Sample with Safe Mixing
// ============================================================================

struct AudioSample {
    int16_t level;

    // Saturating add for safe mixing (prevents clipping)
    AudioSample operator_plus_sat(AudioSample other) const {
        int32_t mixed = static_cast<int32_t>(this->level) +
                       static_cast<int32_t>(other.level);
        if (mixed > 32767) mixed = 32767;
        if (mixed < -32768) mixed = -32768;
        return AudioSample{static_cast<int16_t>(mixed)};
    }

    // Min operator for limiter
    AudioSample operator_min(AudioSample max_level) const {
        int16_t clamped = this->level < max_level.level
            ? this->level : max_level.level;
        return AudioSample{clamped};
    }

    // Max operator for gate
    AudioSample operator_max(AudioSample min_level) const {
        int16_t gated = this->level > min_level.level
            ? this->level : min_level.level;
        return AudioSample{gated};
    }
};

// ============================================================================
// Test Struct 3: Sprite with Bit Rotation
// ============================================================================

struct Sprite {
    uint8_t pattern;

    // Rotate left
    Sprite operator_rot_left(int amount) const {
        uint8_t rotated = this->pattern;
        amount = amount % 8;
        rotated = (rotated << amount) | (rotated >> (8 - amount));
        return Sprite{rotated};
    }

    // Rotate right
    Sprite operator_rot_right(int amount) const {
        uint8_t rotated = this->pattern;
        amount = amount % 8;
        rotated = (rotated >> amount) | (rotated << (8 - amount));
        return Sprite{rotated};
    }
};

// ============================================================================
// Test Struct 4: Color with Safe Mixing
// ============================================================================

struct Color {
    uint8_t r, g, b;

    // Saturating add for safe color mixing
    Color operator_plus_sat(Color other) const {
        auto saturate_add = [](uint8_t a, uint8_t b) -> uint8_t {
            uint16_t sum = static_cast<uint16_t>(a) + static_cast<uint16_t>(b);
            return sum > 255 ? 255 : static_cast<uint8_t>(sum);
        };

        return Color{
            saturate_add(this->r, other.r),
            saturate_add(this->g, other.g),
            saturate_add(this->b, other.b)
        };
    }

    // Min operator for clamping
    Color operator_min(Color max_color) const {
        return Color{
            this->r < max_color.r ? this->r : max_color.r,
            this->g < max_color.g ? this->g : max_color.g,
            this->b < max_color.b ? this->b : max_color.b
        };
    }
};

// ============================================================================
// Tests
// ============================================================================

void test_fixed_saturating_add() {
    Fixed a{20000}, b{15000};
    Fixed result = a.operator_plus_sat(b);
    assert(result.value == 32767);  // Clamped, not 35000

    Fixed c{100}, d{200};
    Fixed result2 = c.operator_plus_sat(d);
    assert(result2.value == 300);  // Normal addition

    std::cout << "✓ Fixed-point saturating add test passed\n";
}

void test_fixed_saturating_sub() {
    Fixed a{-20000}, b{15000};
    Fixed result = a.operator_minus_sat(b);
    assert(result.value == -32768);  // Clamped from -35000

    std::cout << "✓ Fixed-point saturating sub test passed\n";
}

void test_fixed_saturating_mul() {
    Fixed a{500}, b{100};
    Fixed result = a.operator_mul_sat(b);
    assert(result.value == 32767);  // Clamped from 50000

    std::cout << "✓ Fixed-point saturating mul test passed\n";
}

void test_fixed_min_max() {
    Fixed a{5000}, b{3000};
    Fixed min_result = a.operator_min(b);
    assert(min_result.value == 3000);

    Fixed max_result = a.operator_max(b);
    assert(max_result.value == 5000);

    std::cout << "✓ Fixed-point min/max test passed\n";
}

void test_audio_sample_mixing() {
    AudioSample ch1{20000};
    AudioSample ch2{15000};

    // Safe mixing with saturation
    AudioSample mixed = ch1.operator_plus_sat(ch2);
    assert(mixed.level == 32767);  // Clipped at max

    std::cout << "✓ Audio sample mixing test passed\n";
}

void test_audio_sample_limiter() {
    AudioSample signal{25000};
    AudioSample max_level{20000};

    // Limiter using min operator
    AudioSample limited = signal.operator_min(max_level);
    assert(limited.level == 20000);

    std::cout << "✓ Audio sample limiter test passed\n";
}

void test_audio_sample_gate() {
    AudioSample signal{5000};
    AudioSample min_level{10000};

    // Gate using max operator
    AudioSample gated = signal.operator_max(min_level);
    assert(gated.level == 10000);

    std::cout << "✓ Audio sample gate test passed\n";
}

void test_sprite_rotation_left() {
    Sprite sprite{0xA5};  // 10100101
    Sprite rotated = sprite.operator_rot_left(1);
    assert(rotated.pattern == 0x4B);  // 01001011

    std::cout << "✓ Sprite rotation left test passed\n";
}

void test_sprite_rotation_right() {
    Sprite sprite{0xA5};  // 10100101
    Sprite rotated = sprite.operator_rot_right(1);
    assert(rotated.pattern == 0xD2);  // 11010010

    std::cout << "✓ Sprite rotation right test passed\n";
}

void test_sprite_multi_rotation() {
    Sprite sprite{0x12};  // 00010010
    Sprite rotated = sprite.operator_rot_left(2);
    assert(rotated.pattern == 0x48);  // 01001000

    std::cout << "✓ Sprite multi-rotation test passed\n";
}

void test_color_mixing() {
    Color c1{200, 150, 100};
    Color c2{100, 120, 80};

    Color mixed = c1.operator_plus_sat(c2);
    assert(mixed.r == 255);  // 200 + 100 = 300, clamped to 255
    assert(mixed.g == 255);  // 150 + 120 = 270, clamped to 255
    assert(mixed.b == 180);  // 100 + 80 = 180, no clamp

    std::cout << "✓ Color mixing test passed\n";
}

void test_color_clamping() {
    Color color{200, 150, 100};
    Color max_color{180, 160, 120};

    Color clamped = color.operator_min(max_color);
    assert(clamped.r == 180);
    assert(clamped.g == 150);
    assert(clamped.b == 100);

    std::cout << "✓ Color clamping test passed\n";
}

void test_chained_overloaded_operators() {
    // Demonstrate chaining: (a +| b) <> max_val
    Fixed a{20000}, b{15000};
    Fixed max_val{32000};

    Fixed sum = a.operator_plus_sat(b);      // 32767
    Fixed clamped = sum.operator_min(max_val); // 32000
    assert(clamped.value == 32000);

    std::cout << "✓ Chained overloaded operators test passed\n";
}

void test_audio_processing_chain() {
    // Real-world audio: mix two channels with limiter and gate
    AudioSample ch1{18000};
    AudioSample ch2{16000};
    AudioSample max_level{30000};
    AudioSample min_level{1000};

    // Mix channels safely
    AudioSample mixed = ch1.operator_plus_sat(ch2);  // 32767 (limited)

    // Apply limiter
    AudioSample limited = mixed.operator_min(max_level);  // 30000

    // Apply gate (minimum threshold)
    AudioSample gated = limited.operator_max(min_level);  // 30000

    assert(gated.level == 30000);
    std::cout << "✓ Audio processing chain test passed\n";
}

void test_graphics_sprite_processing() {
    // Sprite transformation chain
    Sprite sprite{0xF0};  // 11110000

    // Rotate and re-rotate
    Sprite rotated_l = sprite.operator_rot_left(2);   // 11000011
    Sprite rotated_r = rotated_l.operator_rot_right(2); // Back to 11110000

    assert(rotated_r.pattern == sprite.pattern);
    std::cout << "✓ Graphics sprite processing test passed\n";
}

int main() {
    std::cout << "\n=== Phase 20: Operator Overloading Tests ===\n";

    test_fixed_saturating_add();
    test_fixed_saturating_sub();
    test_fixed_saturating_mul();
    test_fixed_min_max();
    test_audio_sample_mixing();
    test_audio_sample_limiter();
    test_audio_sample_gate();
    test_sprite_rotation_left();
    test_sprite_rotation_right();
    test_sprite_multi_rotation();
    test_color_mixing();
    test_color_clamping();
    test_chained_overloaded_operators();
    test_audio_processing_chain();
    test_graphics_sprite_processing();

    std::cout << "\n✅ All 15 Phase 20 operator overloading tests passed!\n";
    std::cout << "   Operator overloading for advanced operators ready\n\n";

    return 0;
}
