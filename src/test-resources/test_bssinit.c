// Test: BSS initialization zeroes uninitialized globals before main runs.
// x is uninitialized (BSS) — should be 0 after BSS init.
// We read x before and after assignment, store results to $4000/$4001.
// Expected: $4000=00 (BSS zeroed), $4001=2A (x after assignment)
char *out = (char *)0x4000;
int x;
void main() {
    *out = (char)x;       // x should be 0 from BSS init
    x = 42;
    out = (char *)0x4001;
    *out = (char)x;       // x is now 42
    __asm__("brk");
}
