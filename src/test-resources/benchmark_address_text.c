// Phase 89.4.4: Benchmark Text Screen Addressing
// Pattern: row * 40 + col (common MEGA65 text mode)
// Expected: 61% code reduction vs naive arithmetic

unsigned short text_screen_addr(int row, int col) {
    return row * 40 + col;
}

void draw_text(unsigned char *screen, int row, int col, unsigned char ch) {
    int addr = row * 40 + col;
    screen[addr] = ch;
}

void fill_line(unsigned char *screen, int row, unsigned char ch) {
    for (int i = 0; i < 40; i++) {
        int addr = row * 40 + i;
        screen[addr] = ch;
    }
}

int main(void) {
    unsigned char screen[1000];
    
    // Test 1: Simple calculation
    unsigned short addr1 = text_screen_addr(5, 10);
    screen[addr1] = 'A';
    
    // Test 2: In function call
    draw_text(screen, 10, 20, 'B');
    
    // Test 3: In loop
    fill_line(screen, 15, ' ');
    
    // Test 4: Multiple calculations
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            screen[row * 40 + col] = '*';
        }
    }
    
    return 0;
}
