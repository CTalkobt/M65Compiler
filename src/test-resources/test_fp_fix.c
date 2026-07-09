// Test function pointer indirect calls
int test_result = 0;

int my_func(int x) {
    test_result = 0x55;
    return x + 1;
}

int call_fp(int (*fp)(int), int val) {
    test_result = 0x00;
    int result = fp(val);
    return result;
}

int main() {
    int result = call_fp(my_func, 42);
    // If test_result is 0x55, the function was called
    // If result is 43, the return value is correct
    return (test_result == 0x55 && result == 43) ? 0 : 1;
}
