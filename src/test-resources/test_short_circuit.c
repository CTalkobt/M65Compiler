// Test: Short-circuit evaluation of && and ||
// Validates that RHS is not evaluated when LHS determines the result.
// Uses a global flag set by a function to detect evaluation.

int flag;

int set_flag() {
    flag = 1;
    return 1;
}

int set_flag_zero() {
    flag = 1;
    return 0;
}

int main() {
    int result;

    // --- && short-circuit ---

    // false && set_flag() should NOT call set_flag()
    flag = 0;
    result = 0 && set_flag();
    if (result != 0) return 1;
    if (flag != 0) return 2;  // set_flag() must not have been called

    // true && set_flag() SHOULD call set_flag()
    flag = 0;
    result = 1 && set_flag();
    if (result != 1) return 3;
    if (flag != 1) return 4;  // set_flag() must have been called

    // --- || short-circuit ---

    // true || set_flag() should NOT call set_flag()
    flag = 0;
    result = 1 || set_flag();
    if (result != 1) return 5;
    if (flag != 0) return 6;  // set_flag() must not have been called

    // false || set_flag() SHOULD call set_flag()
    flag = 0;
    result = 0 || set_flag();
    if (result != 1) return 7;
    if (flag != 1) return 8;  // set_flag() must have been called

    // --- chained && ---
    // 1 && 1 && 0 && set_flag() -> false, set_flag() not called
    flag = 0;
    result = 1 && 1 && 0 && set_flag();
    if (result != 0) return 9;
    if (flag != 0) return 10;

    // --- chained || ---
    // 0 || 0 || 1 || set_flag() -> true, set_flag() not called
    flag = 0;
    result = 0 || 0 || 1 || set_flag();
    if (result != 1) return 11;
    if (flag != 0) return 12;

    // --- mixed && || ---
    // (1 && 0) || set_flag() -> false || set_flag() -> calls set_flag()
    flag = 0;
    result = (1 && 0) || set_flag();
    if (result != 1) return 13;
    if (flag != 1) return 14;

    // (0 || 1) && set_flag_zero() -> true && set_flag_zero() -> calls set_flag_zero(), result 0
    flag = 0;
    result = (0 || 1) && set_flag_zero();
    if (result != 0) return 15;
    if (flag != 1) return 16;

    return 0;
}
