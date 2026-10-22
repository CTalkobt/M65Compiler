/* overlay_shared/b.c — overlay b: calls overlay a */
#pragma cc45 overlay(b)

int a1(void);
int a2(void);
int a3(void);

int b1(void) { return 100 + a1() + a2() + a3(); }
