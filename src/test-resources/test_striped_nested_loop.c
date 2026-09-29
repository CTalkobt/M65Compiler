// Phase 92.3 Test: Nested loop with striped arrays
// Tests that striped arrays optimize nested loop patterns

__striped int sprite[16][16];
int screen[16][16];

void render_sprite() {
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            screen[row][col] = sprite[row][col];
        }
    }
}

int main() {
    // Initialize sprite
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            sprite[i][j] = i * 16 + j;
        }
    }

    // Render
    render_sprite();

    return 0;
}
