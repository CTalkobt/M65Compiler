// test_switch_range: switch with case ranges and fallthrough
// Returns 0 (A=$00) on success, non-zero on failure.

int classify(int ch) {
    switch (ch) {
        case '0' ... '9':
            return 1;  // digit
        case 'a' ... 'z':
            return 2;  // lowercase (PETSCII $41-$5A)
        case ' ':
            return 3;  // space
        case 0:
            return 4;  // null
        default:
            return 0;  // other
    }
}

int main() {
    if (classify('5') != 1) return 1;
    if (classify('0') != 1) return 2;
    if (classify('9') != 1) return 3;
    if (classify('a') != 2) return 4;
    if (classify('z') != 2) return 5;
    if (classify(' ') != 3) return 6;
    if (classify(0) != 4) return 7;
    if (classify(0xFF) != 0) return 8;

    // Switch with multiple cases per value and fallthrough
    int val = 0;
    switch (3) {
        case 1: val = 10; break;
        case 2: val = 20; break;
        case 3: val = 30; break;
        case 4: val = 40; break;
        default: val = -1; break;
    }
    if (val != 30) return 9;

    // Default case
    switch (99) {
        case 1: val = 1; break;
        default: val = 99; break;
    }
    if (val != 99) return 10;

    return 0;
}
