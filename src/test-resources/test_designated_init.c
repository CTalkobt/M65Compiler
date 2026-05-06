// Test: C99 designated initializers
struct Point {
    int x;
    int y;
};

struct Color {
    char r;
    char g;
    char b;
};

// Global struct with designators
struct Point origin = {.x = 0, .y = 0};

// Global array with designators
int lookup[5] = {[0] = 10, [2] = 30, [4] = 50};

int main() {
    // Local struct designated init (reordered)
    struct Point p = {.y = 20, .x = 10};

    // Local struct partial designated init
    struct Color c = {.r = 255, .b = 128};

    // Local array designated init (sparse)
    int arr[4] = {[1] = 100, [3] = 300};

    // Mixed positional and designated
    int mix[4] = {1, 2, [3] = 99};

    // Verify values
    int result = p.x + p.y;        // 10 + 20 = 30
    result = result + c.r;          // 30 + 255 = 285
    result = result + arr[1];       // 285 + 100 = 385
    result = result + arr[3];       // 385 + 300 = 685
    result = result + mix[3];       // 685 + 99 = 784
    result = result + lookup[2];    // 784 + 30 = 814

    return result;
}
