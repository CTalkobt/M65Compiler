// cl45 fixture: mega65-signedness. Plain char/short/int/long are unsigned
// in cc45: negative constants, x < 0, and / % >> of what standard C would
// have negative.
int count;
unsigned int ucount;
signed int scount;
signed char delta;

int plain(int a, int b) {
    int x = -1;                         // EXPECT mega65-signedness
    count = -5;                         // EXPECT mega65-signedness
    if (a < 0) return 1;                // EXPECT mega65-signedness
    if (b == -1) return 2;              // EXPECT mega65-signedness
    x = a / -2;                         // EXPECT mega65-signedness
    x += (a - b) / 2;                   // EXPECT mega65-signedness
    x += (a - b) >> 1;                  // EXPECT mega65-signedness
    x += (a - 1) % 3;                   // EXPECT mega65-signedness
    x += -a / 4;                        // EXPECT mega65-signedness
    return x;
}

int fine(signed int a, signed int b, unsigned int u, unsigned int v) {
    signed int x = -1;
    scount = -5;
    delta = -3;
    if (a < 0) return 1;
    if (b == -1) return 2;
    x = a / -2;
    x += (a - b) / 2;
    x += (a - b) >> 1;
    ucount = (u - v) / 2;       // declared unsigned: meant
    ucount += (u + v) >> 1;
    return x;
}
