/* test_inplace_xemu.c — g += c on a global done in place (inc / dec, or an
 * add on the variable itself), across the byte and word wrap points, and
 * pointer temporaries kept in base page across calls
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned char ++ / -- / += 7 / -= 3, wrapping at 255 / 0
 *   [1] unsigned int ++ / -- (carry and borrow into the high byte, $FFFF)
 *       and += / -= constants
 *   [2] signed int and signed char updates; x++ in a statement
 *   [3] a pointer to a struct used before and after calls
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char c8;
unsigned int c16;
signed int s16;
signed char s8;
struct It { unsigned char kind; signed int value; unsigned char x, y; };
struct It items[20];
unsigned int total;
static volatile unsigned char sink;
static void note(unsigned char v) { sink = v; }

static unsigned char use_items(void) {
    unsigned char i, ok = 1;
    for (i = 0; i < 20; i++) {
        struct It *it = &items[i];
        note(it->kind);
        total += it->value;
        note(it->x);
        if (it->y != (unsigned char)(i * 3)) ok = 0;
        note(it->y);
        it->value = -it->value;
    }
    return ok;
}

int main(void) {
    unsigned char i, ok = 1;
    c8 = 250;
    for (i = 0; i < 10; i++) c8++;
    if (c8 != 4) ok = 0;
    for (i = 0; i < 10; i++) c8--;
    if (c8 != 250) ok = 0;
    c8 += 7; if (c8 != 1) ok = 0;
    c8 -= 3; if (c8 != 254) ok = 0;
    OUT[0] = ok;
    ok = 1;
    c16 = 0x00FE; c16++; c16++; if (c16 != 0x0100) ok = 0;
    c16--; if (c16 != 0x00FF) ok = 0;
    c16 = 0xFFFF; c16++; if (c16 != 0) ok = 0;
    c16--; if (c16 != 0xFFFF) ok = 0;
    c16 = 0x1234; c16 += 0x0FCD; if (c16 != 0x2201) ok = 0;
    c16 -= 0x2202; if (c16 != 0xFFFF) ok = 0;
    c16 = 0x7F00; c16 += 256; if (c16 != 0x8000) ok = 0;
    OUT[1] = ok;
    ok = 1;
    s16 = -1; s16++; if (s16 != 0) ok = 0;
    s16--; s16--; if (s16 != -2) ok = 0;
    s16 += 1000; if (s16 != 998) ok = 0;
    s16 -= 2000; if (s16 != -1002) ok = 0;
    s8 = 127; s8++; if (s8 != -128) ok = 0;
    s8 -= 5; if (s8 != 123) ok = 0;
    OUT[2] = ok;
    ok = 1;
    for (i = 0; i < 20; i++) { items[i].kind = i; items[i].value = i * 100; items[i].x = i + 1; items[i].y = i * 3; }
    total = 0;
    if (!use_items() || total != 19000) ok = 0;
    for (i = 0; i < 20; i++) if (items[i].value != -(signed int)(i * 100)) ok = 0;
    OUT[3] = ok;
    OUT[4] = 0xAA;
    return 0;
}
