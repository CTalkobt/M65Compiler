// test_strcat: validate strcat() and strncat()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char buf[64];

    // Test 1: Basic strcat
    strcpy(buf, "hello");
    strcat(buf, " world");
    if (strcmp(buf, "hello world") != 0) return 1;

    // Test 2: Empty source
    strcpy(buf, "test");
    strcat(buf, "");
    if (strcmp(buf, "test") != 0) return 2;

    // Test 3: Empty destination
    strcpy(buf, "");
    strcat(buf, "hello");
    if (strcmp(buf, "hello") != 0) return 3;

    // Test 4: Multiple concatenations
    strcpy(buf, "a");
    strcat(buf, "b");
    strcat(buf, "c");
    if (strcmp(buf, "abc") != 0) return 4;

    // Test 5: strncat with limit
    strcpy(buf, "hello");
    strncat(buf, "world", 3);
    if (strcmp(buf, "hellowor") != 0) return 5;

    // Test 6: strncat with limit larger than source
    strcpy(buf, "test");
    strncat(buf, "ing", 10);
    if (strcmp(buf, "testing") != 0) return 6;

    // Test 7: strncat with zero limit
    strcpy(buf, "abc");
    strncat(buf, "def", 0);
    if (strcmp(buf, "abc") != 0) return 7;

    // Test 8: Longer strings
    strcpy(buf, "the quick ");
    strcat(buf, "brown fox");
    if (strcmp(buf, "the quick brown fox") != 0) return 8;

    return 0;
}
