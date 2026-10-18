/* input.c — keyboard and joysticks on the MEGA65 (see <input.h>) */

#include <input.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))

__regparm unsigned char key_pressed(unsigned char keycode);

/* ============================================================================
 * Keyboard
 * ========================================================================== */

/* A code from the hardware queue ($D610: ASCII) as cc45 sees characters */
static unsigned char from_queue(unsigned char c) {
    if (c >= 0x41 && c <= 0x5A) return c + 0x80;   /* SHIFT+letter: 'A' is $C1 in cc45 */
    if (c >= 0x61 && c <= 0x7A) return c - 0x20;   /* letter: 'a' is $41 */
    if (c == 0xA3) return 0x5C;                     /* pound */
    if (c == 0xAF) return 0x5E;                     /* up arrow */
    return c;
}

unsigned char input_waiting(void) {
    return REG(0x00D0) != 0 || REG(0xD610) != 0;
}

unsigned char input_key(void) {
    unsigned char c;
    /* keys a program or BASIC put into the KERNAL's buffer first */
    if (REG(0x00D0)) return (unsigned char)getchar();
    c = REG(0xD610);
    if (!c) return 0;
    REG(0xD610) = 0;                                /* take it from the queue */
    return from_queue(c);
}

unsigned char input_wait(void) {
    unsigned char c;
    while (!(c = input_key())) ;
    return c;
}

void input_flush(void) {
    while (input_key()) ;
}

unsigned char input_pressed(unsigned char keycode) {
    return key_pressed(keycode);
}

unsigned char input_modifiers(void) {
    return REG(0xD611);
}

/* ============================================================================
 * Joysticks
 * ========================================================================== */

static unsigned char joy_type[2];

void input_joy_type(unsigned char port, unsigned char type) {
    if (port == 1 || port == 2) joy_type[port - 1] = type;
}

/* A paddle line's button: a C64GS-style button ties the line to +5 V (it
 * charges at once: a low reading); a Sega pad pulls its C line to ground
 * when pressed (it never charges: $FF) and drives it high otherwise */
static unsigned char pot_button(unsigned char value, unsigned char type) {
    return type == JOYTYPE_SEGA ? value >= 0x80 : value < 0x80;
}

unsigned char input_joy(unsigned char port) {
    unsigned char bits, type, potx, poty;
    if (port != 1 && port != 2) return 0;
    type = joy_type[port - 1];
    /* no keyboard row selected, so held keys don't show on port 1; with
       interrupts held off (the KERNAL's interrupt writes $DC00 too) */
    __asm__("php\n sei");
    REG(0xDC00) = 0xFF;
    bits = (unsigned char)(~(port == 1 ? REG(0xDC01) : REG(0xDC00)) & 0x1F);
    __asm__("plp");
    potx = REG(port == 1 ? 0xD620 : 0xD622);
    poty = REG(port == 1 ? 0xD621 : 0xD623);
    if (type == JOYTYPE_1BUTTON) return bits;
    if (pot_button(potx, type)) bits |= JOY_FIRE2;
    if (type != JOYTYPE_SEGA && pot_button(poty, type)) bits |= JOY_FIRE3;
    return bits;
}
