// Phase 97.1: Address Space Qualifiers - Basic ZP Test
// Tests: __zp keyword recognition and basic variable declarations

__zp int counter;              // Zero-page variable
__zp int *ptr;                 // Zero-page pointer
__abs int value;               // Absolute address space (standard)
__far int *bank_data;          // Far pointer (banking)

int main(void) {
    // ZP variable access
    counter = 0;
    counter++;
    ptr = &counter;
    *ptr = 42;
    
    // Absolute variable access
    value = 100;
    
    return 0;
}
