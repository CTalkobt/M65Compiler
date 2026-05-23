// Test __naked function attribute

__naked void fast_nop(void) {
    __asm__("clv");
    __asm__("rts");
}

int main() {
    fast_nop();
    return 0;
}
