/* attic_x.c — overlay x of the attic RAM overlay test (attic_main.c) */
#pragma cc45 overlay(x)

unsigned char ax(unsigned char v) { return v + 10; }
