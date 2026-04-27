// Test: Multiple function calls in expressions and assignments
// Stress test for ZP temp management and evaluation ordering.

int get_ten() {
    return 10;
}

int get_five() {
    return 5;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int arr[6];

    // Multiple function calls assigned to array elements
    arr[0] = get_ten();
    arr[1] = get_five();
    arr[2] = multiply(get_ten(), get_five());
    arr[3] = multiply(arr[0], 2);
    arr[4] = multiply(get_five(), get_five());
    arr[5] = arr[2] + arr[4];

    if (arr[0] != 10) return 1;
    if (arr[1] != 5) return 2;
    if (arr[2] != 50) return 3;
    if (arr[3] != 20) return 4;
    if (arr[4] != 25) return 5;
    if (arr[5] != 75) return 6;

    // Chain of operations using function results
    int total = multiply(get_ten(), 2) + multiply(get_five(), 3);
    if (total != 35) return 7;

    return 0;
}
