// Test: Pointer operator precedence
// Validates *p++ vs (*p)++ and related expressions.

int main() {
    int val = 42;
    int *p = &val;

    // *p dereference
    int a = *p;
    if (a != 42) return 1;

    // (*p)++ — postfix inc of dereferenced value
    int b = (*p)++;
    if (b != 42) return 2;   // old value
    if (val != 43) return 3; // val incremented

    // ++*p — prefix inc of dereferenced value
    int c = ++*p;
    if (c != 44) return 4;   // new value
    if (val != 44) return 5;

    // (*p)-- — postfix dec of dereferenced value
    int d = (*p)--;
    if (d != 44) return 6;   // old value
    if (val != 43) return 7;

    // --*p — prefix dec of dereferenced value
    int e = --*p;
    if (e != 42) return 8;
    if (val != 42) return 9;

    // *p++ — dereference then advance pointer (separate from *p)
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    int *q = arr;
    int f = *q++;
    if (f != 10) return 10;  // read arr[0]

    // arr[i]++ — array element increment
    arr[0] = 5;
    arr[0]++;
    if (arr[0] != 6) return 11;

    // char* dereference inc
    char cv = 100;
    char *cp = &cv;
    (*cp)++;
    if (cv != 101) return 12;

    char g = ++*cp;
    if (g != 102) return 13;
    if (cv != 102) return 14;

    return 0;
}
