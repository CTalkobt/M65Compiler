// Test case ranges (GCC extension: case <expr> ... <expr>)

int classify_char(char c) {
    switch (c) {
        case 'A' ... 'Z':
            return 1;  // uppercase
        case 'a' ... 'z':
            return 2;  // lowercase
        case '0' ... '9':
            return 3;  // digit
        default:
            return 0;  // other
    }
}

int classify_int(int val) {
    switch (val) {
        case 0 ... 9:
            return 1;    // small
        case 10 ... 99:
            return 2;    // medium
        case 100 ... 999:
            return 3;    // large
        case 1000:
            return 4;    // exactly 1000
        default:
            return 0;    // out of range
    }
}

// Mixed single and range cases
int mixed_cases(int x) {
    switch (x) {
        case 0:
            return 10;
        case 1 ... 5:
            return 20;
        case 6:
            return 30;
        case 7 ... 10:
            return 40;
        default:
            return 50;
    }
}

int main() {
    // Test char ranges
    if (classify_char('A') != 1) return 1;
    if (classify_char('M') != 1) return 2;
    if (classify_char('Z') != 1) return 3;
    if (classify_char('a') != 2) return 4;
    if (classify_char('m') != 2) return 5;
    if (classify_char('z') != 2) return 6;
    if (classify_char('0') != 3) return 7;
    if (classify_char('5') != 3) return 8;
    if (classify_char('9') != 3) return 9;
    if (classify_char(' ') != 0) return 10;

    // Test int ranges
    if (classify_int(0) != 1) return 11;
    if (classify_int(9) != 1) return 12;
    if (classify_int(10) != 2) return 13;
    if (classify_int(99) != 2) return 14;
    if (classify_int(100) != 3) return 15;
    if (classify_int(999) != 3) return 16;
    if (classify_int(1000) != 4) return 17;
    if (classify_int(1001) != 0) return 18;

    // Test mixed single and range cases
    if (mixed_cases(0) != 10) return 19;
    if (mixed_cases(1) != 20) return 20;
    if (mixed_cases(3) != 20) return 21;
    if (mixed_cases(5) != 20) return 22;
    if (mixed_cases(6) != 30) return 23;
    if (mixed_cases(7) != 40) return 24;
    if (mixed_cases(10) != 40) return 25;
    if (mixed_cases(11) != 50) return 26;

    return 0;
}
