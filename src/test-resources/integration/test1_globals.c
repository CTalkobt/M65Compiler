// Integration Test 1: Separate compilation with global variables
// Tests DATA+TEXT segment layout through cc45 -c → ln45 pipeline.
// Expected: $4000 = $2A $05 $AA
volatile char *r = (char *)0x4000;
int g_value = 42;
int g_count;

int main() {
    r[0] = (char)g_value;       // 42 = $2A
    g_count = 5;
    r[1] = (char)g_count;       // 5 = $05
    r[2] = 0xAA;                // sentinel
    return 0;
}
