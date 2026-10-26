struct Point {
    int x;
    int y;
    char color;
};

int main() {
    volatile int s_int = sizeof(int);      // Expected: 2
    volatile int s_char = sizeof(char);    // Expected: 1
    volatile int s_ptr = sizeof(int*);     // Expected: 2
    
    struct Point p;
    volatile int s_struct = sizeof(struct Point); // Expected: 5 (structs are packed)
    volatile int s_expr = sizeof p;               // Expected: 5
    
    int arr[10];
    volatile int s_arr = sizeof arr;              // Expected: 20
    
    volatile int x = 0;
    goto start;
    
bad:
    return 1;

start:
    x = 10;
    if (x == 10) goto next;
    goto bad;

next:
    if (s_int == 2 && s_char == 1 && s_ptr == 2 && s_struct == 5 && s_expr == 5 && s_arr == 20) {
        return 0;
    }
    
    return 2;
}
