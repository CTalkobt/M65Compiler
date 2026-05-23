// Test __interrupt function attribute

volatile int tick_count = 0;

__interrupt void timer_isr(void) {
    tick_count++;
}

int main() {
    return 0;
}
