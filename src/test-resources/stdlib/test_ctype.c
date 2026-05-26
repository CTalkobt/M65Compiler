// test_ctype: validate ctype.h functions (PETSCII)
// Returns 0 (A=$00) on success, non-zero on failure.
//
// PETSCII: lowercase a-z = $41-$5A, uppercase A-Z = $C1-$DA,
//          digits 0-9 = $30-$39

#include <ctype.h>

int main() {
    // isdigit
    if (!isdigit('5')) return 1;
    if (isdigit('a')) return 2;

    // isalpha: lowercase 'h' = $48
    if (!isalpha('h')) return 3;
    // isalpha: uppercase 'H' = $C8
    if (!isalpha(0xC8)) return 4;
    if (isalpha('5')) return 5;

    // isalnum
    if (!isalnum('h')) return 6;
    if (!isalnum('5')) return 7;
    if (isalnum(' ')) return 8;

    // isspace
    if (!isspace(' ')) return 9;
    if (!isspace(0x0D)) return 10;  // CR
    if (isspace('a')) return 11;

    // isupper / islower (PETSCII)
    if (!islower('h')) return 12;       // $48
    if (islower(0xC8)) return 13;       // uppercase H
    if (!isupper(0xC8)) return 14;
    if (isupper('h')) return 15;

    // toupper / tolower
    if (toupper('h') != 0xC8) return 16;
    if (tolower(0xC8) != 'h') return 17;

    // isprint
    if (!isprint(' ')) return 18;
    if (!isprint('a')) return 19;
    if (isprint(0x00)) return 20;

    return 0;
}
