/* test_embed_xemu.c — a file's bytes in an initializer: #include binary
 * (char / word / long) and C23 #embed with its parameters
 *
 * embed/data.bin is 1000 bytes, byte i = ((i * 7 + 3) ^ (i >> 3)) & $FF;
 * embed/empty.bin is empty; embed/motto.txt is "kept in the program".
 *
 * Results at $C0C0 (01 = pass):
 *   [0] #include binary: 1000 bytes, every one right
 *   [1] #include binary word: 500 little-endian words
 *   [2] #include binary long: 250 little-endian longs
 *   [3] #embed limit(4) cc45::offset(10) prefix(...) suffix(...)
 *   [4] #embed of an empty file: if_empty(...) instead
 *   [5] __has_embed: found, empty, not found, an unknown parameter;
 *       __STDC_EMBED_*__ in #if
 *   [6] a text file as a string (suffix(, 0)), in a function's static: its
 *       bytes as they are (ASCII, compared with an a"..." literal)
 *   [7] the file named by a macro, limit by a macro
 *   [8] $AA sentinel
 */
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char expect(unsigned int i) { return (unsigned char)((i * 7 + 3) ^ (i >> 3)); }

const unsigned char data[] = {
#include binary "embed/data.bin"
};

const unsigned int words[] = {
#include binary word "embed/data.bin"
};

const unsigned long longs[] = {
#include binary long "embed/data.bin"
};

const unsigned char part[] = {
#embed "embed/data.bin" limit(4) cc45::offset(10) prefix(0xEE, ) suffix(, 0xDD)
};

const unsigned char none[] = { 1
#embed "embed/empty.bin" prefix(, 0xEE) if_empty(, 2, 3)
};

const unsigned char has[] = {
    __has_embed("embed/data.bin"), __has_embed("embed/empty.bin"),
    __has_embed("embed/missing.bin"), __has_embed("embed/data.bin" other::param(1)),
    __has_embed("embed/data.bin" limit(0)),
};

#if __has_embed("embed/data.bin") == __STDC_EMBED_FOUND__ && __has_embed(<embed/none>) == __STDC_EMBED_NOT_FOUND__
#define IF_OK 1
#else
#define IF_OK 0
#endif

#define DATA_FILE "embed/data.bin"
#define FEW 3
const unsigned char few[] = {
#embed DATA_FILE limit(FEW)
};

static unsigned char motto_ok(void) {
    static const char motto[] = {
#embed "embed/motto.txt" suffix(, 0)
    };
    return sizeof motto == 20 && strcmp(motto, a"kept in the program") == 0;
}

int main(void) {
    unsigned int i;
    unsigned char ok;

    ok = sizeof data == 1000;
    for (i = 0; i < 1000; i++)
        if (data[i] != expect(i)) ok = 0;
    OUT[0] = ok;

    ok = sizeof words == 1000;
    for (i = 0; i < 500; i++)
        if (words[i] != (expect(2 * i) | (unsigned int)expect(2 * i + 1) << 8)) ok = 0;
    OUT[1] = ok;

    ok = sizeof longs == 1000;
    for (i = 0; i < 250; i++)
        if ((unsigned int)longs[i] != (expect(4 * i) | (unsigned int)expect(4 * i + 1) << 8) ||
            (unsigned int)(longs[i] >> 16) != (expect(4 * i + 2) | (unsigned int)expect(4 * i + 3) << 8))
            ok = 0;
    OUT[2] = ok;

    OUT[3] = sizeof part == 6 && part[0] == 0xEE && part[1] == expect(10) && part[2] == expect(11) &&
             part[3] == expect(12) && part[4] == expect(13) && part[5] == 0xDD;
    OUT[4] = sizeof none == 3 && none[0] == 1 && none[1] == 2 && none[2] == 3;
    OUT[5] = has[0] == 1 && has[1] == 2 && has[2] == 0 && has[3] == 0 && has[4] == 2 && IF_OK;
    OUT[6] = motto_ok();
    OUT[7] = sizeof few == 3 && few[0] == expect(0) && few[2] == expect(2);
    OUT[8] = 0xAA;
    return 0;
}
