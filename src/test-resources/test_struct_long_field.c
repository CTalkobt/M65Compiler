/* Test: struct field access for long (32-bit) types
   Issue #90: Loading struct long fields was only reading 2 bytes instead of 4
   Regression: ensure all 4 bytes are loaded from memory when accessing long fields
*/

struct Data {
    long value;
};

struct Mixed {
    int a;
    long b;
    char c;
};

int main(void) {
    struct Data d;
    d.value = 0x12345678;
    long x = d.value;

    struct Mixed m;
    m.a = 0x1122;
    m.b = 0xAABBCCDD;
    m.c = 42;

    long y = m.b;

    return 0;
}
