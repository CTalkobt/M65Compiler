// test_strcpy: validate strcpy()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char buf[16];

    strcpy(buf, "hello");
    if (strcmp(buf, "hello") != 0) return 1;

    strcpy(buf, "");
    if (buf[0] != 0) return 2;

    strcpy(buf, "a");
    if (strcmp(buf, "a") != 0) return 3;

    return 0;
}
