// Phase 89 Benchmark: Text Screen Address Calculations
// Tests: row * 40 + col pattern (common in text mode)
// Expected: 40-50% code reduction for address calculations

#include <stdio.h>

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE 1000

static unsigned char screen[SCREEN_SIZE];
static unsigned char colors[SCREEN_SIZE];

// Basic character write with address calculation
void put_char(int row, int col, unsigned char ch) {
    screen[row * SCREEN_WIDTH + col] = ch;
}

// Write character with color
void put_char_colored(int row, int col, unsigned char ch, unsigned char color) {
    int addr = row * SCREEN_WIDTH + col;
    screen[addr] = ch;
    colors[addr] = color;
}

// Clear a line with character and color
void clear_line(int row, unsigned char ch, unsigned char color) {
    for (int col = 0; col < SCREEN_WIDTH; col++) {
        int addr = row * SCREEN_WIDTH + col;
        screen[addr] = ch;
        colors[addr] = color;
    }
}

// Clear entire screen
void clear_screen(unsigned char ch, unsigned char color) {
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            int addr = row * SCREEN_WIDTH + col;
            screen[addr] = ch;
            colors[addr] = color;
        }
    }
}

// Copy a region
void copy_region(int src_row, int src_col, int dst_row, int dst_col,
                 int width, int height) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            int src_addr = (src_row + r) * SCREEN_WIDTH + (src_col + c);
            int dst_addr = (dst_row + r) * SCREEN_WIDTH + (dst_col + c);
            screen[dst_addr] = screen[src_addr];
            colors[dst_addr] = colors[src_addr];
        }
    }
}

// Swap two characters
void swap_chars(int row1, int col1, int row2, int col2) {
    int addr1 = row1 * SCREEN_WIDTH + col1;
    int addr2 = row2 * SCREEN_WIDTH + col2;
    unsigned char tmp = screen[addr1];
    screen[addr1] = screen[addr2];
    screen[addr2] = tmp;
}

// Print a string at position
void print_string(int row, int col, const char* str, unsigned char color) {
    int c = col;
    for (const char* p = str; *p; p++) {
        if (c >= SCREEN_WIDTH) {
            row++;
            c = 0;
        }
        int addr = row * SCREEN_WIDTH + c;
        screen[addr] = *p;
        colors[addr] = color;
        c++;
    }
}

// Fill a rectangle
void fill_rect(int row, int col, int width, int height, unsigned char ch) {
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            int addr = (row + r) * SCREEN_WIDTH + (col + c);
            screen[addr] = ch;
        }
    }
}

// Draw a border
void draw_box(int row, int col, int width, int height) {
    // Top row
    for (int c = 0; c < width; c++) {
        screen[row * SCREEN_WIDTH + (col + c)] = (c == 0 || c == width - 1) ? '+' : '-';
    }
    // Middle rows
    for (int r = 1; r < height - 1; r++) {
        int addr_l = (row + r) * SCREEN_WIDTH + col;
        int addr_r = (row + r) * SCREEN_WIDTH + (col + width - 1);
        screen[addr_l] = '|';
        screen[addr_r] = '|';
    }
    // Bottom row
    for (int c = 0; c < width; c++) {
        screen[(row + height - 1) * SCREEN_WIDTH + (col + c)] =
            (c == 0 || c == width - 1) ? '+' : '-';
    }
}

int main() {
    // Initialize screen
    for (int i = 0; i < SCREEN_SIZE; i++) {
        screen[i] = 32;  // Space
        colors[i] = 1;   // White
    }

    // Test basic operations
    put_char(0, 0, 'H');
    put_char(0, 1, 'i');

    put_char_colored(2, 0, 'C', 2);
    put_char_colored(2, 1, 'o', 2);
    put_char_colored(2, 2, 'l', 2);
    put_char_colored(2, 3, 'o', 2);

    clear_line(4, 32, 1);
    clear_line(5, 32, 2);

    clear_screen(32, 1);

    fill_rect(5, 5, 10, 5, '*');
    draw_box(10, 10, 15, 8);

    copy_region(5, 5, 15, 15, 10, 5);

    swap_chars(0, 0, 5, 5);

    print_string(12, 12, "Test", 3);

    printf("Screen operations complete\n");
    return 0;
}
