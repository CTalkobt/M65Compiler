/* test_printf_literal_xemu.c — printf of text without %: written without the
 * formatter (puts / putstr); test_xemu.sh compares the screen with the
 * same text printed through %s and checks the program has no formatter */
#include <stdio.h>
int main(void) {
#ifdef FORMATTED
    printf("%s", "HELLO\n"); printf("%s", "AB"); printf("%s", "C\nD\n"); printf("%s", ""); printf("%s", "E");
#else
    printf("HELLO\n"); printf("AB"); printf("C\nD\n"); printf(""); printf("E");
#endif
    return 0;
}
