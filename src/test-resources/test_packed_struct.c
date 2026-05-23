// Test packed-by-default structs and __unpacked keyword

// Packed by default: char + int should be 3 bytes (no padding)
struct Packed {
    char a;
    int b;
};

// __unpacked: char + int should be 4 bytes (1 byte padding after char)
struct __unpacked Padded {
    char a;
    int b;
};

// Mixed types packed: char + char + int = 4 bytes
struct Mixed {
    char x;
    char y;
    int z;
};

// Nested packed struct
struct Inner {
    char a;
    int b;
};

struct Outer {
    char c;
    struct Inner inner;
};

int main() {
    // Packed by default: sizeof should be 3 (char=1 + int=2, no padding)
    if (sizeof(struct Packed) != 3) return 1;

    // __unpacked: sizeof should be 4 (char=1 + 1 pad + int=2)
    if (sizeof(struct Padded) != 4) return 2;

    // Mixed packed: sizeof should be 4 (char+char+int = 1+1+2)
    if (sizeof(struct Mixed) != 4) return 3;

    // Nested: sizeof Inner = 3 (packed), sizeof Outer = 1 + 3 = 4
    if (sizeof(struct Inner) != 3) return 4;
    if (sizeof(struct Outer) != 4) return 5;

    // Verify member access works correctly with packed layout
    struct Packed p;
    p.a = 42;
    p.b = 1000;
    if (p.a != 42) return 6;
    if (p.b != 1000) return 7;

    return 0;
}
