/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma include_once

_Noreturn void exit(int status);

int atoi(char *s);
char *itoa(int value, char *str, int base);
