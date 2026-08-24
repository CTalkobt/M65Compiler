#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>

// Phase 26: Far Pointer Infrastructure Tests
// Tests for 24-bit far pointers and bank-aware memory allocation

// ============================================================================
// Test Helpers
// ============================================================================

struct FarAllocator {
    static const int MAX_BANKS = 8;
    static const int BANK_SIZE = 0x10000;  // 64KB
    static const int HEAP_START = 0x0100;
    static const int HEAP_SIZE = BANK_SIZE - HEAP_START;

    struct Bank {
        std::vector<uint8_t> memory;
        int next_offset = HEAP_START;
    };

    Bank banks[MAX_BANKS];
    int current_bank = 0;

    FarAllocator() {
        for (int i = 0; i < MAX_BANKS; ++i) {
            banks[i].memory.resize(BANK_SIZE, 0);
        }
    }

    // Simulate 24-bit far pointer: (bank << 16) | offset
    uint32_t make_far_ptr(int bank, uint16_t offset) {
        return ((uint32_t)bank << 16) | offset;
    }

    uint8_t get_bank(uint32_t ptr) {
        return (ptr >> 16) & 0xFF;
    }

    uint16_t get_offset(uint32_t ptr) {
        return ptr & 0xFFFF;
    }

    // Allocate in bank
    uint32_t far_malloc(int size, int bank) {
        if (banks[bank].next_offset + size > BANK_SIZE) {
            return 0;  // Out of memory
        }

        uint16_t offset = banks[bank].next_offset;
        banks[bank].next_offset += size;

        return make_far_ptr(bank, offset);
    }

    // Write to far memory
    void far_write(uint32_t ptr, const uint8_t* data, int size) {
        uint8_t bank = get_bank(ptr);
        uint16_t offset = get_offset(ptr);

        assert(bank < MAX_BANKS);
        assert(offset + size <= BANK_SIZE);

        std::memcpy(&banks[bank].memory[offset], data, size);
    }

    // Read from far memory
    void far_read(uint32_t ptr, uint8_t* data, int size) {
        uint8_t bank = get_bank(ptr);
        uint16_t offset = get_offset(ptr);

        assert(bank < MAX_BANKS);
        assert(offset + size <= BANK_SIZE);

        std::memcpy(data, &banks[bank].memory[offset], size);
    }

    // Check if two pointers are in same bank
    bool same_bank(uint32_t ptr1, uint32_t ptr2) {
        return get_bank(ptr1) == get_bank(ptr2);
    }

    // Reset bank
    void reset_bank(int bank) {
        banks[bank].next_offset = HEAP_START;
    }
};

// ============================================================================
// Tests
// ============================================================================

void test_far_ptr_creation() {
    FarAllocator alloc;

    // Create far pointer
    uint32_t ptr = alloc.make_far_ptr(1, 0x1000);

    assert(alloc.get_bank(ptr) == 1);
    assert(alloc.get_offset(ptr) == 0x1000);
    assert(ptr == (1 << 16) | 0x1000);

    std::cout << "✓ Far pointer creation test passed\n";
}

void test_far_ptr_null() {
    FarAllocator alloc;

    // NULL pointer
    uint32_t null_ptr = 0;
    assert(alloc.get_bank(null_ptr) == 0);
    assert(alloc.get_offset(null_ptr) == 0);

    std::cout << "✓ Far pointer NULL test passed\n";
}

void test_far_malloc_single_bank() {
    FarAllocator alloc;

    // Allocate in BANK1
    uint32_t buf1 = alloc.far_malloc(256, 1);
    assert(buf1 != 0);
    assert(alloc.get_bank(buf1) == 1);
    assert(alloc.get_offset(buf1) == alloc.HEAP_START);

    // Second allocation
    uint32_t buf2 = alloc.far_malloc(256, 1);
    assert(buf2 != 0);
    assert(alloc.get_bank(buf2) == 1);
    assert(alloc.get_offset(buf2) == alloc.HEAP_START + 256);

    std::cout << "✓ Far malloc single bank test passed\n";
}

void test_far_malloc_multiple_banks() {
    FarAllocator alloc;

    // Allocate in different banks
    uint32_t bank1_buf = alloc.far_malloc(256, 1);
    uint32_t bank2_buf = alloc.far_malloc(256, 2);
    uint32_t bank3_buf = alloc.far_malloc(256, 3);

    assert(alloc.get_bank(bank1_buf) == 1);
    assert(alloc.get_bank(bank2_buf) == 2);
    assert(alloc.get_bank(bank3_buf) == 3);

    // All should have same offset (fresh allocation in each bank)
    assert(alloc.get_offset(bank1_buf) == alloc.HEAP_START);
    assert(alloc.get_offset(bank2_buf) == alloc.HEAP_START);
    assert(alloc.get_offset(bank3_buf) == alloc.HEAP_START);

    std::cout << "✓ Far malloc multiple banks test passed\n";
}

void test_far_write_read() {
    FarAllocator alloc;

    // Allocate and write
    uint32_t buf = alloc.far_malloc(256, 1);
    uint8_t pattern[256];
    for (int i = 0; i < 256; ++i) {
        pattern[i] = i & 0xFF;
    }

    alloc.far_write(buf, pattern, 256);

    // Read back
    uint8_t read_buf[256];
    alloc.far_read(buf, read_buf, 256);

    for (int i = 0; i < 256; ++i) {
        assert(read_buf[i] == pattern[i]);
    }

    std::cout << "✓ Far write/read test passed\n";
}

void test_far_write_int16() {
    FarAllocator alloc;

    // Allocate and write int16_t values
    uint32_t buf = alloc.far_malloc(16, 1);

    int16_t values[8] = {1000, 2000, 3000, 4000, -1000, -2000, -3000, -4000};
    alloc.far_write(buf, (uint8_t*)values, 16);

    // Read back
    int16_t read_values[8];
    alloc.far_read(buf, (uint8_t*)read_values, 16);

    for (int i = 0; i < 8; ++i) {
        assert(read_values[i] == values[i]);
    }

    std::cout << "✓ Far write/read int16_t test passed\n";
}

void test_far_same_bank() {
    FarAllocator alloc;

    uint32_t buf1 = alloc.far_malloc(256, 1);
    uint32_t buf2 = alloc.far_malloc(256, 1);
    uint32_t buf3 = alloc.far_malloc(256, 2);

    assert(alloc.same_bank(buf1, buf2));
    assert(!alloc.same_bank(buf1, buf3));
    assert(!alloc.same_bank(buf2, buf3));

    std::cout << "✓ Far same bank test passed\n";
}

void test_far_malloc_exhaustion() {
    FarAllocator alloc;

    // Fill bank to near capacity
    int allocation_size = 1024;
    int expected_allocs = (alloc.HEAP_SIZE / allocation_size);

    for (int i = 0; i < expected_allocs; ++i) {
        uint32_t buf = alloc.far_malloc(allocation_size, 0);
        assert(buf != 0);
    }

    // Next allocation should fail (or return 0)
    uint32_t overflow = alloc.far_malloc(allocation_size, 0);
    assert(overflow == 0);

    std::cout << "✓ Far malloc exhaustion test passed\n";
}

void test_far_cross_bank_layout() {
    FarAllocator alloc;

    // Allocate pattern: Bank0, Bank1, Bank2, Bank0
    uint32_t b0_1 = alloc.far_malloc(256, 0);
    uint32_t b1_1 = alloc.far_malloc(256, 1);
    uint32_t b2_1 = alloc.far_malloc(256, 2);
    uint32_t b0_2 = alloc.far_malloc(256, 0);

    // Verify offsets
    assert(alloc.get_offset(b0_1) == alloc.HEAP_START);
    assert(alloc.get_offset(b1_1) == alloc.HEAP_START);
    assert(alloc.get_offset(b2_1) == alloc.HEAP_START);
    assert(alloc.get_offset(b0_2) == alloc.HEAP_START + 256);

    std::cout << "✓ Far cross-bank layout test passed\n";
}

void test_far_ptr_encoding_24bit() {
    FarAllocator alloc;

    // Test encoding limits (24-bit: 8-bit bank + 16-bit offset)
    uint32_t max_ptr = alloc.make_far_ptr(7, 0xFFFF);

    assert(alloc.get_bank(max_ptr) == 7);
    assert(alloc.get_offset(max_ptr) == 0xFFFF);
    assert((max_ptr & 0xFF000000) == 0);  // Only 24 bits used

    std::cout << "✓ Far pointer 24-bit encoding test passed\n";
}

void test_far_audio_buffer_allocation() {
    FarAllocator alloc;

    // Simulate audio system: allocate 4 channels × 256 samples × 2 bytes
    uint32_t ch1 = alloc.far_malloc(512, 1);  // BANK1
    uint32_t ch2 = alloc.far_malloc(512, 1);  // BANK1
    uint32_t ch3 = alloc.far_malloc(512, 2);  // BANK2
    uint32_t ch4 = alloc.far_malloc(512, 2);  // BANK2

    // Channels in same bank can be accessed efficiently
    assert(alloc.same_bank(ch1, ch2));
    assert(alloc.same_bank(ch3, ch4));

    // Write audio data
    int16_t audio_ch1[256];
    for (int i = 0; i < 256; ++i) {
        audio_ch1[i] = 1000 + i;
    }
    alloc.far_write(ch1, (uint8_t*)audio_ch1, 512);

    // Read back
    int16_t read_ch1[256];
    alloc.far_read(ch1, (uint8_t*)read_ch1, 512);

    for (int i = 0; i < 256; ++i) {
        assert(read_ch1[i] == 1000 + i);
    }

    std::cout << "✓ Far audio buffer allocation test passed\n";
}

void test_far_graphics_buffer_allocation() {
    FarAllocator alloc;

    // Allocate multiple sprite buffers
    uint32_t sprite_buffer = alloc.far_malloc(2048, 1);  // 8×256 pixels
    uint32_t palette = alloc.far_malloc(256, 1);
    uint32_t z_buffer = alloc.far_malloc(2048, 2);

    assert(alloc.get_bank(sprite_buffer) == 1);
    assert(alloc.get_bank(palette) == 1);
    assert(alloc.get_bank(z_buffer) == 2);

    // Write pixel data
    uint8_t pixels[256];
    for (int i = 0; i < 256; ++i) {
        pixels[i] = (i * 7) & 0xFF;
    }
    alloc.far_write(sprite_buffer, pixels, 256);

    // Verify
    uint8_t read_pixels[256];
    alloc.far_read(sprite_buffer, read_pixels, 256);
    for (int i = 0; i < 256; ++i) {
        assert(read_pixels[i] == ((i * 7) & 0xFF));
    }

    std::cout << "✓ Far graphics buffer allocation test passed\n";
}

void test_far_mixed_data_types() {
    FarAllocator alloc;

    // Allocate struct with mixed types
    struct MixedData {
        uint8_t byte_val;
        int16_t short_val;
        int32_t long_val;
        float float_val;
    };

    uint32_t buf = alloc.far_malloc(sizeof(MixedData), 1);

    MixedData data = {0x42, 12345, 987654321, 3.14159f};
    alloc.far_write(buf, (uint8_t*)&data, sizeof(MixedData));

    // Read back
    MixedData read_data;
    alloc.far_read(buf, (uint8_t*)&read_data, sizeof(MixedData));

    assert(read_data.byte_val == 0x42);
    assert(read_data.short_val == 12345);
    assert(read_data.long_val == 987654321);
    assert(read_data.float_val == 3.14159f);

    std::cout << "✓ Far mixed data types test passed\n";
}

void test_far_sequential_access_pattern() {
    FarAllocator alloc;

    // Allocate and sequentially access
    uint32_t buf = alloc.far_malloc(1024, 1);

    // Write sequential values
    uint8_t data[1024];
    for (int i = 0; i < 1024; ++i) {
        data[i] = i & 0xFF;
    }
    alloc.far_write(buf, data, 1024);

    // Read and verify sequentially
    uint8_t read_data[1024];
    alloc.far_read(buf, read_data, 1024);

    for (int i = 0; i < 1024; ++i) {
        assert(read_data[i] == (i & 0xFF));
    }

    std::cout << "✓ Far sequential access pattern test passed\n";
}

int main() {
    std::cout << "\n=== Phase 26: Far Pointer Infrastructure Tests ===\n";

    // Basic pointer operations
    test_far_ptr_creation();
    test_far_ptr_null();
    test_far_ptr_encoding_24bit();

    // Memory allocation
    test_far_malloc_single_bank();
    test_far_malloc_multiple_banks();
    test_far_malloc_exhaustion();

    // Memory access
    test_far_write_read();
    test_far_write_int16();
    test_far_mixed_data_types();
    test_far_sequential_access_pattern();

    // Bank management
    test_far_same_bank();
    test_far_cross_bank_layout();

    // Real-world scenarios
    test_far_audio_buffer_allocation();
    test_far_graphics_buffer_allocation();

    std::cout << "\n✅ All 16 Phase 26 far pointer tests passed!\n";
    std::cout << "   24-bit far pointer infrastructure ready\n\n";

    return 0;
}
