
char *results = 0;

int test_switch(int val) {
    int result = 0;
    switch (val) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            // fallthrough
        case 3:
            result = result + 5;
            break;
        default:
            result = 100;
            break;
    }
    return result;
}

void main() {
    int i;
    int sum;
    
    results = 0x4000;

    // 1. If/Else & Logical operators
    if (1 && 1) {
        results[1] = 1;
    } else {
        results[1] = 0xFF;
    }
    
    if (0 || 1) {
        results[2] = 1;
    } else {
        results[2] = 0xFF;
    }

    // 2. While loop
    i = 0;
    sum = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }
    results[3] = sum; // 0+1+2+3+4 = 10

    // 3. Do-While loop
    i = 0;
    sum = 0;
    do {
        sum = sum + i;
        i = i + 1;
    } while (i < 5);
    results[4] = sum; // 0+1+2+3+4 = 10

    // 4. For loop with break/continue
    sum = 0;
    for (i = 0; i < 10; i = i + 1) {
        if (i < 3) continue;
        if (i > 6) break;
        sum = sum + i;
    }
    results[5] = sum; // 3+4+5+6 = 18

    // 5. Switch
    results[6] = test_switch(1);  // 10
    results[7] = test_switch(2);  // 25
    results[8] = test_switch(4);  // 100

    // 6. Ternary
    results[9] = (1 ? 0x11 : 0x22);
    results[10] = (0 ? 0x11 : 0x22);

    // Final Success Flag
    results[0] = 0xAA;
    
    while(1) {
        i = i;
    }
}
