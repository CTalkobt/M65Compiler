// Example: strcpy — validate string copy
// Uses c45.lib string.h implementation.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

char buf[16];

int main() {
    // Copy and verify contents
    strcpy(buf, "hello");
    if (strcmp(buf, "hello") != 0) return 1;
    if (strlen(buf) != 5) return 2;

    // Copy empty string
    strcpy(buf, "");
    if (strlen(buf) != 0) return 3;

    return 0;
}
