/* game_of_life — Conway's Game of Life on MEGA65
 *
 * Runs on the 40x25 default text screen. Each cell is one character
 * position: '*' for alive, ' ' for dead.
 *
 * Demonstrates: 2D arrays, nested loops, pointer arithmetic,
 * direct screen RAM writes, frame delay via busy loop.
 *
 * Rules:
 *   - Live cell with 2 or 3 neighbors survives
 *   - Dead cell with exactly 3 neighbors becomes alive
 *   - All other cells die or stay dead
 */

#include <string.h>
#include <mega65.h>

#define WIDTH  40
#define HEIGHT 25
#define ALIVE  0x2A   /* '*' in PETSCII */
#define DEAD   0x20   /* ' ' in PETSCII */

/* Double buffer: current and next generation */
static char grid[HEIGHT][WIDTH];
static char next[HEIGHT][WIDTH];

/* Screen RAM pointer (default C64/MEGA65 location) */
volatile char *screen = (char *)0x0400;
/* Color RAM */
volatile char *color = (char *)0xD800;

static void clear_grid() {
    memset(grid, 0, WIDTH * HEIGHT);
    memset(next, 0, WIDTH * HEIGHT);
}

/* Count live neighbors with wrapping */
static int count_neighbors(int row, int col) {
    int count;
    int r;
    int c;
    int nr;
    int nc;

    count = 0;
    for (r = -1; r <= 1; r++) {
        for (c = -1; c <= 1; c++) {
            if (r == 0 && c == 0) continue;
            nr = row + r;
            nc = col + c;
            /* Wrap around edges */
            if (nr < 0) nr = HEIGHT - 1;
            if (nr >= HEIGHT) nr = 0;
            if (nc < 0) nc = WIDTH - 1;
            if (nc >= WIDTH) nc = 0;
            if (grid[nr][nc]) count++;
        }
    }
    return count;
}

static void step() {
    int r;
    int c;
    int n;

    for (r = 0; r < HEIGHT; r++) {
        for (c = 0; c < WIDTH; c++) {
            n = count_neighbors(r, c);
            if (grid[r][c]) {
                /* Alive: survive with 2 or 3 neighbors */
                if (n == 2 || n == 3)
                    next[r][c] = 1;
                else
                    next[r][c] = 0;
            } else {
                /* Dead: born with exactly 3 neighbors */
                if (n == 3)
                    next[r][c] = 1;
                else
                    next[r][c] = 0;
            }
        }
    }

    /* Copy next → grid */
    memcpy(grid, next, WIDTH * HEIGHT);
}

static void draw() {
    int r;
    int c;
    int offset;

    for (r = 0; r < HEIGHT; r++) {
        for (c = 0; c < WIDTH; c++) {
            offset = r * WIDTH + c;
            if (grid[r][c])
                screen[offset] = ALIVE;
            else
                screen[offset] = DEAD;
        }
    }
}

static void delay() {
    int i;
    for (i = 0; i < 2000; i++) {
        /* busy wait */
    }
}

/* Seed: R-pentomino — a classic long-lived pattern */
static void seed_r_pentomino(int row, int col) {
    grid[row][col + 1]     = 1;
    grid[row][col + 2]     = 1;
    grid[row + 1][col]     = 1;
    grid[row + 1][col + 1] = 1;
    grid[row + 2][col + 1] = 1;
}

/* Seed: Glider */
static void seed_glider(int row, int col) {
    grid[row][col + 1]     = 1;
    grid[row + 1][col + 2] = 1;
    grid[row + 2][col]     = 1;
    grid[row + 2][col + 1] = 1;
    grid[row + 2][col + 2] = 1;
}

int main() {
    int gen;

    VREG_BORDER=COLOR_BLACK;
    clear_grid();
    VREG_BORDER=COLOR_RED;

    /* Place some patterns */
    VREG_BORDER=COLOR_CYAN;
    seed_r_pentomino(10, 18);
    VREG_BORDER=COLOR_PURPLE;
    seed_glider(2, 2);
    VREG_BORDER=COLOR_GREEN;
    seed_glider(3, 30);
    VREG_BORDER=COLOR_BLUE;

    /* Set all color RAM to green */

    memset(color, (char)5, WIDTH * HEIGHT);
    VREG_BORDER=COLOR_YELLOW;
    for (gen = 0; gen < 500; gen++) {
    VREG_BORDER=COLOR_ORANGE;
        draw();
        delay();
        step();
    }
    VREG_BORDER=COLOR_BROWN;

    return 0;
}
