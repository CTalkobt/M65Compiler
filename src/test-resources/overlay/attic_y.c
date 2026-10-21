/* attic_y.c — overlay y of the attic RAM overlay test (attic_main.c) */
#pragma cc45 overlay(y)

unsigned char ay(unsigned char v) { return v + 20; }
