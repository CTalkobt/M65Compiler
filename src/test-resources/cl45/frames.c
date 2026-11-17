// cl45 fixture: mega65-recursion-frame and mega65-stack-frame-limit. A
// function on a call cycle gets a stack frame; a frame over the limit (256
// bytes by default) is a lot of a 4 KB stack.
unsigned char sink(char *p);

unsigned int depth(unsigned int n) {                            // EXPECT mega65-recursion-frame
    return n ? depth(n - 1) + 1 : 0;
}

unsigned int pong(unsigned int n);
unsigned int ping(unsigned int n) {                             // EXPECT mega65-recursion-frame
    return n ? pong(n - 1) : 0;
}
unsigned int pong(unsigned int n) {                             // EXPECT mega65-recursion-frame
    return n ? ping(n - 1) : 1;
}

// meant: the pragma says so
#pragma cc45 recurse
unsigned int meant(unsigned int n) {
    return n ? meant(n - 1) + 2 : 0;
}

#pragma cc45 recurse
unsigned char big(unsigned char n) {                            // EXPECT mega65-stack-frame-limit
    char buf[300];
    buf[0] = n;
    return sink(buf) + (n ? big(n - 1) : 0);
}

#pragma cc45 recurse
unsigned char small(unsigned char n) {
    char buf[40];
    buf[0] = n;
    return sink(buf) + (n ? small(n - 1) : 0);
}

// static allocation: 300 bytes of BSS, no frame
unsigned char flat(unsigned char n) {
    char buf[300];
    buf[0] = n;
    return sink(buf);
}
