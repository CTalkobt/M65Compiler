int main() {
    // Test 1: Register read/write
    __cpu.A = 0x42;
    if (__cpu.A != 0x42) return 1;
    
    __cpu.X = 0x10;
    if (__cpu.X != 0x10) return 2;
    
    __cpu.AX = 0x1234;
    if (__cpu.AX != 0x1234) return 3;
    
    // Test 2: Flag read/write
    __flags.Carry = 1;
    if (!__flags.Carry) return 4;
    
    __flags.Carry = 0;
    if (__flags.Carry) return 5;
    
    __flags.Zero = 1;
    if (!__flags.Zero) return 6;

    // (#609: Zero read as 1 and Negative as 0 whatever they were, and Zero
    // was written the wrong way round)
    __flags.Zero = 0;
    if (__flags.Zero) return 10;
    {
        volatile unsigned char one = 1, none = 0;
        __flags.Zero = one;
        if (!__flags.Zero) return 11;
        __flags.Zero = none;
        if (__flags.Zero) return 12;
        __flags.Negative = one;
        if (!__flags.Negative) return 13;
        __flags.Negative = none;
        if (__flags.Negative) return 14;
    }
    __flags.Negative = 1;
    if (!__flags.Negative) return 15;
    __flags.Negative = 0;
    if (__flags.Negative) return 16;
    __flags.Overflow = 0;
    if (__flags.Overflow) return 17;
    __cpu.A = 0x80;                 // (a load sets N and clears Z)
    if (!__flags.Negative) return 18;
    __cpu.A = 0;
    if (!__flags.Zero) return 19;
    
    // Test 3: Complex expressions
    __cpu.A = 10;
    __cpu.X = 20;
    int sum = __cpu.A + __cpu.X;
    if (sum != 30) return 7;
    
    // Test 4: Assignment chain
    int a = __cpu.A = 50;
    if (a != 50) return 8;
    if (__cpu.A != 50) return 9;

    return 0; // PASS
}
