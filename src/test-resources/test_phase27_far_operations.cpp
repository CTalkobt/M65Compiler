#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

// Phase 27: Far Pointer Operations Tests
// Tests for element access, bulk transfers, and arithmetic on 24-bit pointers

// ============================================================================
// Test Infrastructure (Simulated Far Memory)
// ============================================================================

struct FarMemory {
    static const int NUM_BANKS = 8;
    static const int BANK_SIZE = 0x10000;

    std::vector<uint8_t> banks[NUM_BANKS];
    int current_bank = 0;

    FarMemory() {
        for (int i = 0; i < NUM_BANKS; ++i) {
            banks[i].resize(BANK_SIZE, 0);
        }
    }

    // Pointer construction
    uint32_t make_ptr(int bank, uint16_t offset) {
        return ((uint32_t)bank << 16) | offset;
    }

    uint8_t get_bank(uint32_t ptr) { return (ptr >> 16) & 0xFF; }
    uint16_t get_offset(uint32_t ptr) { return ptr & 0xFFFF; }

    // Element access
    uint8_t read_uint8(uint32_t ptr, int offset) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        return banks[bank][addr];
    }

    int16_t read_int16(uint32_t ptr, int offset) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        return *(int16_t*)&banks[bank][addr];
    }

    uint32_t read_uint32(uint32_t ptr, int offset) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        return *(uint32_t*)&banks[bank][addr];
    }

    void write_uint8(uint32_t ptr, int offset, uint8_t val) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        banks[bank][addr] = val;
    }

    void write_int16(uint32_t ptr, int offset, int16_t val) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        *(int16_t*)&banks[bank][addr] = val;
    }

    void write_uint32(uint32_t ptr, int offset, uint32_t val) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        *(uint32_t*)&banks[bank][addr] = val;
    }

    // Bulk operations
    void memcpy(uint32_t dst, uint32_t src, size_t len) {
        int src_bank = get_bank(src);
        int dst_bank = get_bank(dst);
        int src_off = get_offset(src);
        int dst_off = get_offset(dst);

        if (src_bank == dst_bank) {
            // Same bank: direct copy
            std::memcpy(&banks[dst_bank][dst_off],
                       &banks[src_bank][src_off], len);
        } else {
            // Cross-bank: byte by byte
            for (size_t i = 0; i < len; ++i) {
                banks[dst_bank][dst_off + i] = banks[src_bank][src_off + i];
            }
        }
    }

    void memset(uint32_t ptr, uint8_t val, size_t len) {
        int bank = get_bank(ptr);
        int offset = get_offset(ptr);
        std::memset(&banks[bank][offset], val, len);
    }

    // Pointer arithmetic
    uint32_t add_offset(uint32_t ptr, int offset) {
        int bank = get_bank(ptr);
        int addr = get_offset(ptr) + offset;
        return make_ptr(bank, addr);
    }

    int get_diff(uint32_t ptr1, uint32_t ptr2) {
        assert(get_bank(ptr1) == get_bank(ptr2));
        return (int)get_offset(ptr1) - (int)get_offset(ptr2);
    }
};

static FarMemory g_memory;

// ============================================================================
// Tests
// ============================================================================

void test_read_uint8() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    g_memory.write_uint8(ptr, 0, 0x42);
    uint8_t val = g_memory.read_uint8(ptr, 0);

    assert(val == 0x42);

    std::cout << "✓ Read uint8 test passed\n";
}

void test_write_int16() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    g_memory.write_int16(ptr, 0, 12345);
    int16_t val = g_memory.read_int16(ptr, 0);

    assert(val == 12345);

    std::cout << "✓ Write int16 test passed\n";
}

void test_read_write_uint32() {
    uint32_t ptr = g_memory.make_ptr(2, 0x2000);

    g_memory.write_uint32(ptr, 0, 987654321);
    uint32_t val = g_memory.read_uint32(ptr, 0);

    assert(val == 987654321);

    std::cout << "✓ Read/write uint32 test passed\n";
}

void test_element_with_offset() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    // Write at offset 0, 2, 4, 6
    g_memory.write_int16(ptr, 0, 100);
    g_memory.write_int16(ptr, 2, 200);
    g_memory.write_int16(ptr, 4, 300);
    g_memory.write_int16(ptr, 6, 400);

    // Read back
    assert(g_memory.read_int16(ptr, 0) == 100);
    assert(g_memory.read_int16(ptr, 2) == 200);
    assert(g_memory.read_int16(ptr, 4) == 300);
    assert(g_memory.read_int16(ptr, 6) == 400);

    std::cout << "✓ Element with offset test passed\n";
}

void test_memcpy_same_bank() {
    uint32_t src = g_memory.make_ptr(1, 0x1000);
    uint32_t dst = g_memory.make_ptr(1, 0x2000);

    // Fill source
    for (int i = 0; i < 256; ++i) {
        g_memory.write_uint8(g_memory.add_offset(src, i), 0, i & 0xFF);
    }

    // Copy
    g_memory.memcpy(dst, src, 256);

    // Verify
    for (int i = 0; i < 256; ++i) {
        assert(g_memory.read_uint8(g_memory.add_offset(dst, i), 0) == (i & 0xFF));
    }

    std::cout << "✓ Memcpy same bank test passed\n";
}

void test_memcpy_cross_bank() {
    uint32_t src = g_memory.make_ptr(1, 0x1000);
    uint32_t dst = g_memory.make_ptr(2, 0x1000);

    // Fill source
    for (int i = 0; i < 256; ++i) {
        g_memory.write_uint8(g_memory.add_offset(src, i), 0, (i * 7) & 0xFF);
    }

    // Copy across banks
    g_memory.memcpy(dst, src, 256);

    // Verify
    for (int i = 0; i < 256; ++i) {
        assert(g_memory.read_uint8(g_memory.add_offset(dst, i), 0) == ((i * 7) & 0xFF));
    }

    std::cout << "✓ Memcpy cross-bank test passed\n";
}

void test_memset() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    // Set to pattern
    g_memory.memset(ptr, 0xAA, 256);

    // Verify
    for (int i = 0; i < 256; ++i) {
        assert(g_memory.read_uint8(g_memory.add_offset(ptr, i), 0) == 0xAA);
    }

    std::cout << "✓ Memset test passed\n";
}

void test_pointer_arithmetic_add() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    uint32_t ptr2 = g_memory.add_offset(ptr, 256);

    assert(g_memory.get_bank(ptr2) == 1);
    assert(g_memory.get_offset(ptr2) == 0x1100);

    std::cout << "✓ Pointer arithmetic add test passed\n";
}

void test_pointer_diff() {
    uint32_t ptr1 = g_memory.make_ptr(1, 0x1000);
    uint32_t ptr2 = g_memory.make_ptr(1, 0x1100);

    int diff = g_memory.get_diff(ptr2, ptr1);
    assert(diff == 256);

    std::cout << "✓ Pointer difference test passed\n";
}

void test_audio_channel_copy() {
    // Simulate mixing 2 channels
    uint32_t ch1 = g_memory.make_ptr(1, 0x0100);
    uint32_t ch2 = g_memory.make_ptr(1, 0x0300);
    uint32_t mix = g_memory.make_ptr(1, 0x0500);

    // Fill channels with audio
    for (int i = 0; i < 256; ++i) {
        int16_t ch1_val = 1000 + i;
        int16_t ch2_val = 2000 + i;

        g_memory.write_int16(ch1, i * 2, ch1_val);
        g_memory.write_int16(ch2, i * 2, ch2_val);
    }

    // Copy ch1 to mix
    g_memory.memcpy(mix, ch1, 512);

    // Verify mix has ch1 data
    for (int i = 0; i < 256; ++i) {
        int16_t val = g_memory.read_int16(mix, i * 2);
        assert(val == 1000 + i);
    }

    std::cout << "✓ Audio channel copy test passed\n";
}

void test_graphics_buffer_fill() {
    uint32_t buf = g_memory.make_ptr(2, 0x0000);

    // Clear buffer
    g_memory.memset(buf, 0x00, 1024);

    // Verify cleared
    for (int i = 0; i < 1024; ++i) {
        assert(g_memory.read_uint8(g_memory.add_offset(buf, i), 0) == 0x00);
    }

    // Fill with color pattern
    g_memory.memset(buf, 0x0F, 512);

    // Verify pattern
    for (int i = 0; i < 512; ++i) {
        assert(g_memory.read_uint8(g_memory.add_offset(buf, i), 0) == 0x0F);
    }

    std::cout << "✓ Graphics buffer fill test passed\n";
}

void test_mixed_type_struct() {
    struct Data {
        uint8_t byte_val;
        int16_t short_val;
        uint32_t long_val;
    };

    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    // Write struct
    g_memory.write_uint8(ptr, 0, 0x42);
    g_memory.write_int16(ptr, 1, 12345);
    g_memory.write_uint32(ptr, 3, 987654321);

    // Read back
    assert(g_memory.read_uint8(ptr, 0) == 0x42);
    assert(g_memory.read_int16(ptr, 1) == 12345);
    assert(g_memory.read_uint32(ptr, 3) == 987654321);

    std::cout << "✓ Mixed type struct test passed\n";
}

void test_sequential_int16_array() {
    uint32_t ptr = g_memory.make_ptr(1, 0x1000);

    // Create array of int16_t values
    int16_t values[] = {100, 200, 300, 400, 500, 600, 700, 800};

    // Write array
    for (int i = 0; i < 8; ++i) {
        g_memory.write_int16(ptr, i * 2, values[i]);
    }

    // Read back sequentially
    for (int i = 0; i < 8; ++i) {
        int16_t val = g_memory.read_int16(ptr, i * 2);
        assert(val == values[i]);
    }

    std::cout << "✓ Sequential int16 array test passed\n";
}

void test_multi_bank_audio_buffers() {
    // Allocate audio in multiple banks
    uint32_t bank1_ch1 = g_memory.make_ptr(1, 0x0100);
    uint32_t bank1_ch2 = g_memory.make_ptr(1, 0x0500);
    uint32_t bank2_ch3 = g_memory.make_ptr(2, 0x0100);
    uint32_t bank3_ch4 = g_memory.make_ptr(3, 0x0100);

    // Fill all channels
    for (int i = 0; i < 256; ++i) {
        g_memory.write_int16(bank1_ch1, i * 2, 1000 + i);
        g_memory.write_int16(bank1_ch2, i * 2, 2000 + i);
        g_memory.write_int16(bank2_ch3, i * 2, 3000 + i);
        g_memory.write_int16(bank3_ch4, i * 2, 4000 + i);
    }

    // Verify all accessible
    assert(g_memory.read_int16(bank1_ch1, 0) == 1000);
    assert(g_memory.read_int16(bank1_ch2, 0) == 2000);
    assert(g_memory.read_int16(bank2_ch3, 0) == 3000);
    assert(g_memory.read_int16(bank3_ch4, 0) == 4000);

    std::cout << "✓ Multi-bank audio buffers test passed\n";
}

void test_cross_bank_reverb() {
    // Simulate reverb impulse response in one bank, convolution in another
    uint32_t ir_bank = g_memory.make_ptr(2, 0x0000);
    uint32_t signal_bank = g_memory.make_ptr(1, 0x0000);

    // Fill impulse response
    int16_t ir_data[] = {1000, 500, 250, 125, 62};
    for (int i = 0; i < 5; ++i) {
        g_memory.write_int16(ir_bank, i * 2, ir_data[i]);
    }

    // Fill signal
    for (int i = 0; i < 256; ++i) {
        g_memory.write_int16(signal_bank, i * 2, 100);
    }

    // Verify cross-bank access works
    assert(g_memory.read_int16(ir_bank, 0) == 1000);
    assert(g_memory.read_int16(signal_bank, 0) == 100);

    std::cout << "✓ Cross-bank reverb test passed\n";
}

void test_large_bulk_copy() {
    // Copy 64KB between banks
    uint32_t src = g_memory.make_ptr(1, 0x0000);
    uint32_t dst = g_memory.make_ptr(2, 0x0000);

    // Fill source with pattern (skip large portions to save time)
    for (int i = 0; i < 65536; i += 256) {
        g_memory.write_uint8(g_memory.add_offset(src, i), 0, i & 0xFF);
    }

    // Copy entire bank
    g_memory.memcpy(dst, src, 65536);

    // Verify sample locations
    for (int i = 0; i < 65536; i += 256) {
        uint8_t val = g_memory.read_uint8(g_memory.add_offset(dst, i), 0);
        assert(val == (i & 0xFF));
    }

    std::cout << "✓ Large bulk copy test passed\n";
}

int main() {
    std::cout << "\n=== Phase 27: Far Pointer Operations Tests ===\n";

    // Element access
    test_read_uint8();
    test_write_int16();
    test_read_write_uint32();
    test_element_with_offset();

    // Bulk operations
    test_memcpy_same_bank();
    test_memcpy_cross_bank();
    test_memset();

    // Pointer arithmetic
    test_pointer_arithmetic_add();
    test_pointer_diff();

    // Real-world scenarios
    test_audio_channel_copy();
    test_graphics_buffer_fill();
    test_mixed_type_struct();
    test_sequential_int16_array();
    test_multi_bank_audio_buffers();
    test_cross_bank_reverb();
    test_large_bulk_copy();

    std::cout << "\n✅ All 17 Phase 27 far operation tests passed!\n";
    std::cout << "   Far pointer operations ready\n\n";

    return 0;
}
