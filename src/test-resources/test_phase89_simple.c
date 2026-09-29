// Simple test for Phase 89 address template optimization
// Tests basic row * 40 + col pattern (text screen addressing)

#include <stdio.h>

#define SCREEN_WIDTH 40

unsigned char screen[1000];

// Simple text screen addressing - should trigger template
void put_char_at(int row, int col, unsigned char ch) {
    screen[row * SCREEN_WIDTH + col] = ch;
}

// Multiple accesses to same address pattern
void put_string(int row, int col, const char* str) {
    int c = col;
    for (const char* p = str; *p; p++) {
        screen[row * SCREEN_WIDTH + c] = *p;
        c++;
    }
}

// Array of addresses - more complex pattern
void fill_line(int row, unsigned char ch) {
    for (int col = 0; col < SCREEN_WIDTH; col++) {
        screen[row * SCREEN_WIDTH + col] = ch;
    }
}

int main() {
    put_char_at(0, 0, 'H');
    put_char_at(0, 1, 'i');
    put_char_at(1, 0, 'T');
    put_char_at(1, 1, 'e');
    put_char_at(1, 2, 's');
    put_char_at(1, 3, 't');

    put_string(2, 0, "Address");
    put_string(3, 0, "Templates");

    fill_line(5, 32);
    fill_line(6, 42);

    printf("Phase 89 test complete\n");
    return 0;
}
