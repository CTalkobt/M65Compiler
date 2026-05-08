// Test _Alignas on local variables
int test_alignas_local_4byte(void) {
    char before;           // offset 0, size 1
    _Alignas(4) int x;     // should be at offset 4, size 2
    return (int)&x & 3;    // should return 0 (aligned to 4 bytes)
}

int test_alignas_local_16byte(void) {
    char before;           // offset 0, size 1
    _Alignas(16) char buf[8]; // should be at offset 16
    return (int)&buf & 15; // should return 0 (aligned to 16 bytes)
}

// Test _Alignas on struct members
struct AlignedStruct {
    char a;                  // offset 0, size 1
    _Alignas(4) int b;       // should be at offset 4, size 2
    char c;                  // offset 6, size 1
    _Alignas(16) int arr[2]; // should be at offset 16, size 4
};

int test_alignas_struct_member(void) {
    struct AlignedStruct s;
    int offset_b = (int)&s.b - (int)&s;
    int offset_arr = (int)&s.arr - (int)&s;

    // Check alignment of member b (should be at offset 4)
    if (offset_b != 4) return 1;
    // Check alignment of array (should be at offset 16)
    if (offset_arr != 16) return 2;
    // Check member b is 4-byte aligned
    if (offset_b & 3) return 3;
    // Check array is 16-byte aligned
    if (offset_arr & 15) return 4;

    return 0;
}

// Test _Alignas with nested scopes (slot reuse)
int test_alignas_scope_reuse(void) {
    {
        _Alignas(16) char buf1[8];
        char unused;
    }
    {
        _Alignas(16) char buf2[8];
        // buf2 should be able to reuse buf1's frame slot (slot reuse across sibling scopes)
    }
    return 0;
}

// Test global _Alignas
_Alignas(8) int global_aligned = 42;
_Alignas(16) char global_big[8];

int test_alignas_global(void) {
    // Just verify globals compiled; alignment is enforced by .align directives in asm
    return global_aligned == 42 ? 0 : 1;
}
