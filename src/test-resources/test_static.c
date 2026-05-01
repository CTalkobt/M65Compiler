// Test: static keyword support
// Tests static global variables, static functions, and static local variables

// Static global variable — file-scope linkage, not exported
static int sg_count = 10;
static char sg_flag;

// Static function — file-scope linkage, not exported
static int add_one(int x) {
    return x + 1;
}

// Regular function using static globals and static function
int use_statics() {
    sg_count = sg_count + 5;
    sg_flag = 1;
    return add_one(sg_count);
}

// Function with a static local variable
int counter() {
    static int calls = 0;
    calls = calls + 1;
    return calls;
}

// Another function with its own static local (no name collision)
int other_counter() {
    static int calls = 0;
    calls = calls + 1;
    return calls;
}

int main() {
    int a;
    int b;
    int c;

    // Test static global
    a = sg_count; // should be 10

    // Test static function
    b = add_one(a); // should be 11

    // Test use_statics (modifies sg_count)
    c = use_statics(); // sg_count becomes 15, returns 16

    // Test static locals (each call increments independently)
    a = counter();       // 1
    b = counter();       // 2
    c = other_counter(); // 1 (independent from counter's static)

    return 0;
}
