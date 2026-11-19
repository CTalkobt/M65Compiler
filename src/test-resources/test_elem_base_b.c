// The other file of test_elem_base_xemu.c (#658): the extern union, and a
// reader that sees the bytes as they are in memory (not inlined)
union area { unsigned char m[256]; struct { unsigned char pad[100]; unsigned char rows[50]; unsigned w[20]; } r; };
union area area;
unsigned char eb_byte(unsigned i) { return area.m[i]; }
