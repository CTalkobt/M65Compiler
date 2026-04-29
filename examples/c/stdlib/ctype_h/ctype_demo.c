#include <stdio.h>
#include <ctype.h>

// Demonstrate PETSCII-aware ctype.h functions on the MEGA65.
//
// PETSCII reminder:
//   lowercase a-z = $41-$5A  (what you type unshifted)
//   uppercase A-Z = $C1-$DA  (shifted)
//   digits  0-9  = $30-$39
//
// This program classifies a few characters, converts case,
// and prints the results via KERNAL CHROUT.

void print_hex(int val) {
    int hi;
    int lo;
    hi = (val >> 4) & 15;
    lo = val & 15;
    putchar('$');
    if (hi < 10) putchar('0' + hi); else putchar('a' + hi - 10);
    if (lo < 10) putchar('0' + lo); else putchar('a' + lo - 10);
}

void check_char(int c) {
    putchar(' ');
    print_hex(c);
    putchar(':');
    if (isalpha(c))  putchar('a'); else putchar('-');
    if (isdigit(c))  putchar('d'); else putchar('-');
    if (isalnum(c))  putchar('n'); else putchar('-');
    if (isspace(c))  putchar('s'); else putchar('-');
    if (isprint(c))  putchar('p'); else putchar('-');
    putchar(' ');
}

int main() {
    puts("ctype.h demo");
    puts("flags: a=alpha d=digit n=alnum s=space p=print");
    puts("");

    // Lowercase 'h' ($48 in PETSCII)
    check_char('h');
    puts("'h' lowercase");

    // Uppercase 'H' ($C8 in PETSCII) - use hex since shifted
    check_char(0xc8);
    puts("'h' uppercase");

    // Digit '5' ($35)
    check_char('5');
    puts("'5' digit");

    // Space ($20)
    check_char(' ');
    puts("' ' space");

    // CR ($0D)
    check_char(0x0d);
    puts("cr control");

    // toupper / tolower
    puts("");
    puts("case conversion:");
    putchar(' ');
    print_hex(toupper('h'));    // $48 -> $C8
    puts(" = toupper('h')");
    putchar(' ');
    print_hex(tolower(0xc8));  // $C8 -> $48
    puts(" = tolower('h')");

    return 0;
}
