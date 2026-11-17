// cl45 fixture: a program with nothing to report (and nothing reported from
// the system headers it includes).
#include <stdio.h>
#include <string.h>
#include <mega65.h>
#include <far_ptr.h>

static unsigned char line[40];

static unsigned int sum(const unsigned char *p, unsigned char n) {
    unsigned int s = 0;
    while (n--) s += *p++;
    return s;
}

int main(void) {
    memset(line, 1, sizeof line);
    vic4->border = 2;
    printf("%u\n", sum(line, (unsigned char)sizeof line));
    return 0;
}
