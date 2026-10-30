// Example: gets / gets_s — a line typed on the keyboard
// Uses c45.lib stdio.h: stdin reads a typed line (echoed; DEL takes a
// character back; RETURN ends it). To run unattended the program puts its
// "typing" into the KERNAL's keyboard buffer first ($02B0, the count at
// $D0: up to 10 keys), as if typed.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdio.h>
#include <string.h>

#define REG(a) (*(volatile unsigned char *)(a))

static void type(const char *keys) {
    unsigned char n = 0;
    while (keys[n]) { REG(0x02B0 + n) = keys[n]; n++; }
    REG(0x00D0) = n;
}

int main() {
    char name[20];
    char code[4];

    // gets: the whole line ('\n' is RETURN)
    printf("What is your name? ");
    type("Ada\n");
    if (gets(name) != name) return 1;
    printf("Hello, %s!\n", name);
    if (strcmp(name, "Ada") != 0) return 2;

    // gets_s: at most sizeof code - 1 characters; a longer line gives ""
    printf("Code (3 characters)? ");
    type("1234\n");
    if (gets_s(code, sizeof code) != NULL) return 3;
    if (code[0] != 0) return 4;
    printf("Too long.\nCode (3 characters)? ");
    type("123\n");
    if (gets_s(code, sizeof code) != code) return 5;
    if (strcmp(code, "123") != 0) return 6;
    printf("Code %s accepted.\n", code);
    return 0;
}
