/* input.h — keyboard and joysticks on the MEGA65 (lib/stdlib/input.c)
 *
 * Keyboard. Typed keys wait in the keyboard's hardware queue at $D610 (the
 * C65 KERNAL does not copy them into its buffer); input_key reads them,
 * one code per key press, never waiting:
 *   - letters as PETSCII, so they match cc45's character constants
 *     ('a' unshifted, 'A' with SHIFT): the queue gives ASCII
 *   - RETURN, DEL, HOME, the cursor keys as PETSCII (CH_RETURN, CH_DELETE,
 *     CH_HOME, CH_CURSOR_UP/DOWN/LEFT/RIGHT from <cbm.h>)
 *   - RUN/STOP 3 (CH_STOP), ESC 27, TAB 9, F1-F14 $F1-$FE (CH_F1...)
 *   - keys put into the KERNAL's buffer ($02B0, by BASIC or a program)
 *     first, through GETIN
 * input_pressed(KEY_A) (the KEY_ matrix codes of <mega65.h>) tells whether a
 * key is held down now, several at once (SHIFT, CTRL, RUN/STOP too); it is
 * key_pressed.
 *
 * Joysticks. input_joy(1) / input_joy(2) read a port: JOY_UP, JOY_DOWN,
 * JOY_LEFT, JOY_RIGHT, JOY_FIRE, and the extra buttons on the paddle lines
 * (POT X: JOY_FIRE2, POT Y: JOY_FIRE3, $D620-$D623) once the program says
 * what is connected (input_joy_type; a mouse or paddles use the same lines):
 *   JOYTYPE_1BUTTON directions and fire only (the default)
 *   JOYTYPE_C64GS  3-button sticks (C64GS / Cheetah style: a button ties its
 *                  paddle line to +5 V)
 *   JOYTYPE_SEGA   Sega Genesis pads: directions, B (JOY_FIRE) and C
 *                  (JOY_FIRE2, pin 9 pulled to ground). The pad's select line
 *                  is pin 7, +5 V on a Commodore port, so A, START and a
 *                  6-button pad's X/Y/Z can't be read; the pad needs power
 *                  on pin 5 (an adapter)
 * Port 1 is CIA 1 port B ($DC01) and paddles $D620/$D621, port 2 CIA 1
 * port A ($DC00) and $D622/$D623.
 */
#pragma once

/* Key codes input_key returns besides characters */
#define CH_STOP         0x03
#define CH_TAB          0x09
#define CH_ESC          0x1B
#ifndef CH_RETURN
#define CH_RETURN       0x0D
#define CH_DELETE       0x14
#define CH_HOME         0x13
#define CH_CURSOR_UP    0x91
#define CH_CURSOR_DOWN  0x11
#define CH_CURSOR_LEFT  0x9D
#define CH_CURSOR_RIGHT 0x1D
#endif
#define CH_F1           0xF1
#define CH_F2           0xF2
#define CH_F3           0xF3
#define CH_F4           0xF4
#define CH_F5           0xF5
#define CH_F6           0xF6
#define CH_F7           0xF7
#define CH_F8           0xF8
#define CH_F9           0xF9
#define CH_F10          0xFA
#define CH_F11          0xFB
#define CH_F12          0xFC
#define CH_F13          0xFD
#define CH_F14          0xFE

/* The next key (above), or 0 when none is waiting */
unsigned char input_key(void);
/* The next key, waiting for one */
unsigned char input_wait(void);
/* 1 when a key is waiting */
unsigned char input_waiting(void);
/* Discard the keys waiting */
void input_flush(void);
/* 1 while the key (a KEY_ matrix code of <mega65.h>) is held down */
unsigned char input_pressed(unsigned char keycode);
/* The modifier keys held now: $D611 (bit 0 right SHIFT, 1 left SHIFT,
 * 2 CTRL, 3 MEGA, 4 ALT, 5 NO SCROLL, 6 CAPS LOCK) */
unsigned char input_modifiers(void);

/* Joysticks */
#ifndef JOY_UP
#define JOY_UP          0x01
#define JOY_DOWN        0x02
#define JOY_LEFT        0x04
#define JOY_RIGHT       0x08
#define JOY_FIRE        0x10
#endif
#define JOY_FIRE2       0x20
#define JOY_FIRE3       0x40

#define JOYTYPE_1BUTTON 0
#define JOYTYPE_C64GS   1
#define JOYTYPE_SEGA    2

/* What is connected to port 1 or 2 (JOYTYPE_1BUTTON by default) */
void input_joy_type(unsigned char port, unsigned char type);
/* Port 1 or 2: JOY_ bits of what is held down */
unsigned char input_joy(unsigned char port);
