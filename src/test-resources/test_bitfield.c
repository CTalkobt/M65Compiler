// Test bitfield support
struct flags {
    unsigned char active : 1;
    unsigned char mode : 3;
    unsigned char priority : 4;
};

struct wide_flags {
    unsigned int counter : 10;
    unsigned int channel : 6;
};

unsigned char result;
unsigned int result16;

int main() {
    struct flags f;

    // Test write
    f.active = 1;
    f.mode = 5;
    f.priority = 12;

    // Test read
    result = f.active;
    result = f.mode;
    result = f.priority;

    // Test increment
    f.mode++;

    // Test 16-bit bitfield
    struct wide_flags wf;
    wf.counter = 500;
    wf.channel = 30;
    result16 = wf.counter;
    result16 = wf.channel;

    return 0;
}
