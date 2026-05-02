// Bitfield mmemu test — results at $4000
struct flags {
    unsigned char active : 1;
    unsigned char mode : 3;
    unsigned char priority : 4;
};

struct wide_flags {
    unsigned int counter : 10;
    unsigned int channel : 6;
};

volatile unsigned char *res = (unsigned char*)0x4000;

void main() {
    struct flags f;
    f.active = 1;
    f.mode = 5;
    f.priority = 12;

    res[0] = f.active;     // 01
    res[1] = f.mode;       // 05
    res[2] = f.priority;   // 0C

    f.mode++;
    res[3] = f.mode;       // 06

    struct wide_flags wf;
    wf.counter = 500;
    wf.channel = 30;
    res[4] = (unsigned char)wf.counter;  // F4 (500 & 0xFF)
    res[5] = (unsigned char)wf.channel;  // 1E

    __asm__("brk");
}
