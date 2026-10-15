/* test_input_xemu.c — <input.h>: keys pressed through the MEGA65's virtual
 * key registers ($D615/$D616: the key is held as if pressed, so it goes
 * through the hardware scanner, the $D610 queue and the matrix like real
 * typing), joysticks
 *
 * Results at $C0C0 (01 = pass):
 *   [0] input_key: a letter as cc45 writes it ('a', SHIFT 'A'), 0 when none
 *   [1] special keys: RETURN, DEL, HOME, cursor down/up/right/left, F1, F2,
 *       RUN/STOP (3), the pound sign
 *   [2] input_waiting, input_flush
 *   [3] a key put into the KERNAL's buffer comes first (through GETIN)
 *   [4] input_pressed while held, two keys at once, RUN/STOP; not after
 *   [5] input_modifiers: SHIFT and CTRL
 *   [6] input_joy: nothing connected is 0, also while keys are held; a bad
 *       port is 0; the 3-button and Sega types read the paddle lines (xemu
 *       does not emulate them: they read 0) the two ways round
 *   [7] conio's kbhit / getch see typed keys
 *   [8] $AA sentinel
 */
#include <input.h>
#include <mega65.h>
#include <conio.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static void frames(unsigned char n) {
    while (n--) {
        while (REG(0xD012) != 0xF0) ;
        while (REG(0xD012) == 0xF0) ;
    }
}

/* the virtual key register's code: row * 8 + column (KEY_ is column * 8 + row) */
#define VK(k) ((unsigned char)((((k) & 7) << 3) | ((k) >> 3)))

/* press (with an optional held modifier) and release */
static void tap2(unsigned char mod, unsigned char key) {
    if (mod != 0xFF) REG(0xD616) = VK(mod);
    REG(0xD615) = VK(key);
    frames(3);
    REG(0xD615) = 0x7F;
    REG(0xD616) = 0x7F;
    frames(3);
}
#define tap(k) tap2(0xFF, (k))

static int t_letters(void) {
    int ok;
    input_flush();
    ok = input_key() == 0;
    tap(KEY_A);
    ok = ok && input_key() == 'a' && input_key() == 0;
    tap2(KEY_LSHIFT, KEY_B);
    return ok && input_key() == 'B';
}

static int t_special(void) {
    static const unsigned char keys[] = { KEY_RETURN, KEY_DELETE, KEY_HOME, KEY_DOWN, KEY_RIGHT, KEY_F1, KEY_STOP, KEY_POUND };
    static const unsigned char want[] = { CH_RETURN, CH_DELETE, CH_HOME, CH_CURSOR_DOWN, CH_CURSOR_RIGHT, CH_F1, CH_STOP, 0x5C };
    unsigned char i, ok = 1;
    for (i = 0; i < sizeof keys; i++) {
        tap(keys[i]);
        if (input_key() != want[i]) ok = 0;
    }
    tap2(KEY_LSHIFT, KEY_DOWN);
    ok = ok && input_key() == CH_CURSOR_UP;
    tap2(KEY_RSHIFT, KEY_RIGHT);
    ok = ok && input_key() == CH_CURSOR_LEFT;
    tap2(KEY_LSHIFT, KEY_F1);
    return ok && input_key() == CH_F2;
}

static int t_waiting(void) {
    int ok = !input_waiting();
    tap(KEY_X);
    ok = ok && input_waiting();
    tap(KEY_Y);
    input_flush();
    return ok && !input_waiting() && input_key() == 0;
}

static int t_kernal_buffer(void) {
    REG(0x02B0) = 'q';
    REG(0x00D0) = 1;
    tap(KEY_W);
    return input_key() == 'q' && input_key() == 'w' && input_key() == 0;
}

static int t_pressed(void) {
    int ok;
    REG(0xD615) = VK(KEY_A);
    REG(0xD616) = VK(KEY_STOP);
    frames(3);
    ok = input_pressed(KEY_A) && input_pressed(KEY_STOP) && !input_pressed(KEY_B);
    REG(0xD615) = 0x7F;
    REG(0xD616) = 0x7F;
    frames(3);
    input_flush();
    return ok && !input_pressed(KEY_A) && !input_pressed(KEY_STOP);
}

static int t_modifiers(void) {
    int ok = (input_modifiers() & 0x07) == 0;
    REG(0xD615) = VK(KEY_LSHIFT);
    REG(0xD616) = VK(KEY_CTRL);
    frames(3);
    /* (xemu reports left SHIFT in bit 0, the MEGA65 Book says bit 1) */
    ok = ok && (input_modifiers() & 0x03) && (input_modifiers() & 0x04);
    REG(0xD615) = 0x7F;
    REG(0xD616) = 0x7F;
    frames(3);
    input_flush();
    return ok && (input_modifiers() & 0x07) == 0;
}

static int t_joy(void) {
    int ok;
    unsigned char sega;
    ok = input_joy(1) == 0 && input_joy(2) == 0 && input_joy(0) == 0 && input_joy(3) == 0;
    /* keys held do not show as joystick bits */
    REG(0xD615) = VK(KEY_SPACE);
    REG(0xD616) = VK(KEY_2);
    frames(3);
    ok = ok && input_joy(1) == 0 && input_joy(2) == 0;
    REG(0xD615) = 0x7F;
    REG(0xD616) = 0x7F;
    frames(3);
    input_flush();
    /* 3-button: a button reads low; Sega: its C button reads high */
    input_joy_type(2, JOYTYPE_C64GS);
    sega = input_joy(2);
    ok = ok && ((sega & JOY_FIRE2) != 0) == (REG(0xD622) < 0x80) && ((sega & JOY_FIRE3) != 0) == (REG(0xD623) < 0x80);
    input_joy_type(2, JOYTYPE_SEGA);
    sega = input_joy(2);
    ok = ok && (sega & ~JOY_FIRE2) == 0 && ((sega & JOY_FIRE2) != 0) == (REG(0xD622) >= 0x80);
    input_joy_type(2, JOYTYPE_1BUTTON);
    return ok && input_joy(2) == 0;
}

static int t_conio(void) {
    int ok = !kbhit();
    tap(KEY_Z);
    ok = ok && kbhit() && getch() == 'z' && !kbhit();
    return ok;
}

int main(void) {
    OUT[0] = t_letters();
    OUT[1] = t_special();
    OUT[2] = t_waiting();
    OUT[3] = t_kernal_buffer();
    OUT[4] = t_pressed();
    OUT[5] = t_modifiers();
    OUT[6] = t_joy();
    OUT[7] = t_conio();
    OUT[8] = 0xAA;
    return 0;
}
