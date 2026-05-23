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
