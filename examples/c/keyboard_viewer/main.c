/* keyboard_viewer — the MEGA65 keyboard on screen; the keys held down light up
 *
 * Every frame each key of the 8x8 matrix is read with key_pressed()
 * (<mega65.h>: several keys at once, SHIFT and CTRL too); a key held down is
 * drawn reversed in yellow. The bottom lines show conio's kbhit() and the
 * characters getch() returns (PETSCII, as the KERNAL's keyboard buffer gives
 * them), with their codes. RUN/STOP (code 3 from getch) quits.
 *
 * This is the interactive test of kbhit/getch/cputsxy/textcolor: run it in
 * a windowed xemu (headless xemu can't press keys) or on a MEGA65.
 */
#include <conio.h>
#include <mega65.h>
#include <stdio.h>

struct key {
    unsigned char code;     /* matrix code: (column << 3) | row */
    unsigned char x, y;     /* where its label starts */
    const char *label;
};

/* The 64 keys of the matrix, laid out like the keyboard */
static const struct key keys[] = {
    /* function keys */
    { KEY_F1, 58, 3, "F1" }, { KEY_F3, 63, 3, "F3" }, { KEY_F5, 68, 3, "F5" }, { KEY_F7, 73, 3, "F7" },
    /* number row */
    { KEY_LARROW, 2, 5, "<-" }, { KEY_1, 6, 5, "1" }, { KEY_2, 10, 5, "2" }, { KEY_3, 14, 5, "3" },
    { KEY_4, 18, 5, "4" }, { KEY_5, 22, 5, "5" }, { KEY_6, 26, 5, "6" }, { KEY_7, 30, 5, "7" },
    { KEY_8, 34, 5, "8" }, { KEY_9, 38, 5, "9" }, { KEY_0, 42, 5, "0" }, { KEY_PLUS, 46, 5, "+" },
    { KEY_MINUS, 50, 5, "-" }, { KEY_POUND, 54, 5, "\\" }, { KEY_HOME, 58, 5, "HOME" },
    { KEY_DELETE, 64, 5, "DEL" },
    /* Q row */
    { KEY_CTRL, 2, 7, "CTRL" }, { KEY_Q, 8, 7, "Q" }, { KEY_W, 12, 7, "W" }, { KEY_E, 16, 7, "E" },
    { KEY_R, 20, 7, "R" }, { KEY_T, 24, 7, "T" }, { KEY_Y, 28, 7, "Y" }, { KEY_U, 32, 7, "U" },
    { KEY_I, 36, 7, "I" }, { KEY_O, 40, 7, "O" }, { KEY_P, 44, 7, "P" }, { KEY_AT, 48, 7, "@" },
    { KEY_STAR, 52, 7, "*" }, { KEY_UPARROW, 56, 7, "^" },
    /* A row */
    { KEY_STOP, 2, 9, "STOP" }, { KEY_A, 9, 9, "A" }, { KEY_S, 13, 9, "S" }, { KEY_D, 17, 9, "D" },
    { KEY_F, 21, 9, "F" }, { KEY_G, 25, 9, "G" }, { KEY_H, 29, 9, "H" }, { KEY_J, 33, 9, "J" },
    { KEY_K, 37, 9, "K" }, { KEY_L, 41, 9, "L" }, { KEY_COLON, 45, 9, ":" }, { KEY_SEMI, 49, 9, ";" },
    { KEY_EQUALS, 53, 9, "=" }, { KEY_RETURN, 58, 9, "RETURN" },
    /* Z row */
    { KEY_CBM, 2, 11, "C=" }, { KEY_LSHIFT, 6, 11, "SHIFT" }, { KEY_Z, 14, 11, "Z" },
    { KEY_X, 18, 11, "X" }, { KEY_C, 22, 11, "C" }, { KEY_V, 26, 11, "V" }, { KEY_B, 30, 11, "B" },
    { KEY_N, 34, 11, "N" }, { KEY_M, 38, 11, "M" }, { KEY_COMMA, 42, 11, "," },
    { KEY_PERIOD, 46, 11, "." }, { KEY_SLASH, 50, 11, "/" }, { KEY_RSHIFT, 54, 11, "SHIFT" },
    { KEY_DOWN, 64, 11, "CRSR-DN" },
    /* space row */
    { KEY_SPACE, 20, 13, "       SPACE       " }, { KEY_RIGHT, 64, 13, "CRSR-RT" },
};

#define NKEYS (sizeof keys / sizeof keys[0])

static unsigned char down[NKEYS];   /* as last drawn */

static void draw_key(unsigned char i, unsigned char pressed) {
    unsigned char x = keys[i].x;
    const char *s = keys[i].label;
    textcolor(pressed ? YELLOW : LIGHT_GRAY);
    revers(pressed);
    while (*s) cputch(x++, keys[i].y, *s++);
    revers(0);
}

static void wait_frame(void) {
    while (*(volatile unsigned char *)0xD012 != 0xF8) ;
    while (*(volatile unsigned char *)0xD012 == 0xF8) ;
}

static const char hex[] = "0123456789abcdef";

int main(void) {
    unsigned char i, count = 0;
    char line[8];

    putchar(14);            /* upper/lower case characters (cc45 text) */
    textbackground(BLUE);
    textcolor(WHITE);
    clrscr();
    cputsxy(2, 1, "MEGA65 Keyboard Viewer: press keys (several at once). Press RUN/STOP to quit.");
    for (i = 0; i < NKEYS; i++) draw_key(i, 0);
    textcolor(LIGHT_BLUE);
    cputsxy(2, 17, "kbhit():");
    cputsxy(2, 19, "getch(): ");
    cputsxy(2, 21, "Last keys:");

    for (;;) {
        wait_frame();
        for (i = 0; i < NKEYS; i++) {
            unsigned char p = key_pressed(keys[i].code);
            if (p != down[i]) {
                down[i] = p;
                draw_key(i, p);
            }
        }
        textcolor(WHITE);
        cputsxy(11, 17, kbhit() ? "1 (a key is waiting)" : "0                   ");
        while (kbhit()) {
            int c = getch();
            if (c == 3) goto quit;          /* RUN/STOP */
            /* the code, and the character itself when it prints */
            line[0] = '$';
            line[1] = hex[(c >> 4) & 15];
            line[2] = hex[c & 15];
            line[3] = ' ';
            line[4] = (c >= 0x20 && c < 0x80) || c >= 0xA0 ? (char)c : '.';
            line[5] = 0;
            textcolor(YELLOW);
            cputsxy(11, 19, line);
            /* a running list of the last keys */
            cputch(13 + count, 21, line[4]);
            count = (count + 1) % 60;
            cputch(13 + count, 21, ' ');
        }
    }
quit:
    textcolor(WHITE);
    textbackground(BLACK);
    clrscr();
    return 0;
}
