/* input_joyport.c — what is on a joystick port, the Amiga mouse support
 * (<input.h>, #645): CIA 1 ($DC01 port 1, $DC00 port 2), the paddle lines
 * ($D620/$D621 port 1, $D622/$D623 port 2), $D61B (mega65-core
 * c65uart.vhdl: bits 0/1 Amiga mouse support on ports 1/2, bits 2/3 assume
 * an Amiga mouse; write only) */

#include <input.h>

#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char d61b;      /* (the value written last) */

joyport_device_t input_joyport_detect(unsigned char port) {
    unsigned char px, py, lines;
    if (port != 1 && port != 2) return JOYPORT_UNKNOWN;
    px = REG(port == 1 ? 0xD620 : 0xD622);
    py = REG(port == 1 ? 0xD621 : 0xD623);
    if (px == 0xFF && py == 0xFF) {
        lines = (unsigned char)(~REG(port == 1 ? 0xDC01 : 0xDC00) & 0x1F);
        return lines ? JOYPORT_JOYSTICK : JOYPORT_EMPTY;
    }
    if (px < 0x80 && py < 0x80)
        return (d61b & (port == 1 ? 0x01 : 0x02)) ? JOYPORT_MOUSE_AMIGA : JOYPORT_MOUSE_1351;
    return JOYPORT_PADDLE;
}

unsigned char input_set_mouse_mode(unsigned char port, unsigned char mode) {
    unsigned char en = port == 1 ? 0x01 : 0x02, as = port == 1 ? 0x04 : 0x08;
    if ((port != 1 && port != 2) || mode > INPUT_MOUSE_AUTO) return 0;
    d61b &= (unsigned char)~(en | as);
    if (mode == INPUT_MOUSE_AMIGA) d61b |= (unsigned char)(en | as);
    else if (mode == INPUT_MOUSE_AUTO) d61b |= en;
    REG(0xD61B) = d61b;
    return 1;
}

const char *input_joyport_name(joyport_device_t device) {
    switch (device) {
    case JOYPORT_EMPTY: return "empty";
    case JOYPORT_JOYSTICK: return "joystick";
    case JOYPORT_MOUSE_1351: return "1351 mouse";
    case JOYPORT_MOUSE_AMIGA: return "Amiga mouse";
    case JOYPORT_PADDLE: return "paddles";
    default: return "unknown";
    }
}
