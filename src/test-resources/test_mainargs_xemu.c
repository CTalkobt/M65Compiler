/* test_mainargs_xemu.c — main(argc, argv) from the BASIC line that ran it
 *
 * Run by test_xemu.sh with different typed lines (xemu -prgtest); the
 * script compares what this writes at $C100:
 *   argc, then each argv[i] followed by '|', then 'N' if argv[argc] is NULL
 * (the text is PETSCII as BASIC stored it: typed letters arrive as $41-$5A)
 */
#define OUT ((volatile unsigned char *)0xC100)

int main(int argc, char **argv) {
    unsigned char k = 0;
    int i;
    char *s;
    OUT[k++] = (unsigned char)argc;
    for (i = 0; i < argc; i++) {
        for (s = argv[i]; *s; s++) OUT[k++] = (unsigned char)*s;
        OUT[k++] = 0x7C;   /* '|' */
    }
    OUT[k++] = argv[argc] == 0 ? 0x4E : 0x3F;   /* 'N' / '?' */
    OUT[k] = 0;
    return 0;
}
