// The other file of test_member_addr_xemu.c (#660): the extern union, and
// functions that take the addresses (not inlined: another file)
union area { unsigned char m[256]; struct { unsigned char pad[100]; unsigned char rows[50]; unsigned w[20]; } r; };
union area area;
unsigned ma_addr(const void *p) { return (unsigned)p; }
void ma_fill(unsigned char *p, unsigned char v, unsigned char n) { while (n--) *p++ = v; }
unsigned char ma_sum(const unsigned char *p, unsigned char n) { unsigned char s = 0; while (n--) s += *p++; return s; }
void ma_putw(unsigned *p, unsigned v) { *p = v; }
