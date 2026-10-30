/* joystick.h — cc65's joystick API on the MEGA65's two joystick ports
 * (lib/stdlib_asm/joystick.s45, over <input.h>'s input_joy)
 *
 * The driver is built in: joy_install(joy_static_stddrv) or
 * joy_load_driver(any name) installs it (joy_read works either way).
 * joy_read(JOY_1) is port 1, joy_read(JOY_2) port 2; the bits are
 * input_joy's: up, down, left, right, the fire button, and the second and
 * third buttons on the paddle lines once input_joy_type says what is
 * connected. The JOY_UP(v) ... macros test them, as cc65's: this header
 * redefines <input.h>'s JOY_UP ... masks as those macros (use the _MASK
 * names for the bits).
 */
#pragma once

#define JOY_ERR_OK              0
#define JOY_ERR_NO_DRIVER       1
#define JOY_ERR_CANNOT_LOAD     2
#define JOY_ERR_INV_DRIVER      3
#define JOY_ERR_NO_DEVICE       4

#define JOY_1                   0
#define JOY_2                   1

#define JOY_UP_MASK             0x01
#define JOY_DOWN_MASK           0x02
#define JOY_LEFT_MASK           0x04
#define JOY_RIGHT_MASK          0x08
#define JOY_BTN_1_MASK          0x10
#define JOY_BTN_2_MASK          0x20
#define JOY_BTN_3_MASK          0x40
#define JOY_BTN_4_MASK          0x00    /* (none) */
#define JOY_FIRE_MASK           JOY_BTN_1_MASK

#undef JOY_UP
#undef JOY_DOWN
#undef JOY_LEFT
#undef JOY_RIGHT
#undef JOY_FIRE
#define JOY_UP(v)               ((v) & JOY_UP_MASK)
#define JOY_DOWN(v)             ((v) & JOY_DOWN_MASK)
#define JOY_LEFT(v)             ((v) & JOY_LEFT_MASK)
#define JOY_RIGHT(v)            ((v) & JOY_RIGHT_MASK)
#define JOY_BTN_1(v)            ((v) & JOY_BTN_1_MASK)
#define JOY_BTN_2(v)            ((v) & JOY_BTN_2_MASK)
#define JOY_BTN_3(v)            ((v) & JOY_BTN_3_MASK)
#define JOY_BTN_4(v)            ((v) & JOY_BTN_4_MASK)
#define JOY_FIRE(v)             ((v) & JOY_FIRE_MASK)

extern const char joy_stddrv[];
extern const unsigned char joy_static_stddrv[];

__fastcall__ unsigned char joy_load_driver(const char *driver);
__fastcall__ unsigned char joy_unload(void);
__fastcall__ unsigned char joy_install(const void *driver);
__fastcall__ unsigned char joy_uninstall(void);
/* 2 */
__fastcall__ unsigned char joy_count(void);
/* The bits of what is held on JOY_1 or JOY_2 (0 for another) */
__fastcall__ unsigned char joy_read(unsigned char joystick);
