// test_bitfield: bitfield operations
// Returns 0 (A=$00) on success, non-zero on failure.

struct Flags {
    unsigned char visible : 1;
    unsigned char dirty : 1;
    unsigned char active : 1;
    unsigned char layer : 3;
    unsigned char unused : 2;
};

struct Mixed {
    int value;
    unsigned char bits : 4;
    unsigned char more : 4;
};

int main() {
    struct Flags f;

    // Set individual bits
    f.visible = 1;
    f.dirty = 0;
    f.active = 1;
    f.layer = 5;
    f.unused = 0;

    if (f.visible != 1) return 1;
    if (f.dirty != 0) return 2;
    if (f.active != 1) return 3;
    if (f.layer != 5) return 4;

    // Modify
    f.dirty = 1;
    if (f.dirty != 1) return 5;
    // Other fields unchanged
    if (f.visible != 1) return 6;
    if (f.layer != 5) return 7;

    // Clear
    f.visible = 0;
    if (f.visible != 0) return 8;

    // Mixed struct + bitfield
    struct Mixed m;
    m.value = 1234;
    m.bits = 0x0A;
    m.more = 0x05;
    if (m.value != 1234) return 9;
    if (m.bits != 0x0A) return 10;
    if (m.more != 0x05) return 11;

    // Bitfield overflow (should truncate)
    m.bits = 0xFF;  // only 4 bits
    if (m.bits != 0x0F) return 12;

    return 0;
}
