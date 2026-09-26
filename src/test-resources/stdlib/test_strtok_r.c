// test_strtok_r: validate reentrant tokenizer
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>
#include <stdlib.h>

int main() {
    char str1[32];
    char str2[32];
    char *saveptr1, *saveptr2;
    char *token;

    // Test 1: Basic tokenization
    strcpy(str1, "hello world test");
    token = strtok_r(str1, " ", &saveptr1);
    if (strcmp(token, "hello") != 0) return 1;

    token = strtok_r(NULL, " ", &saveptr1);
    if (strcmp(token, "world") != 0) return 2;

    token = strtok_r(NULL, " ", &saveptr1);
    if (strcmp(token, "test") != 0) return 3;

    token = strtok_r(NULL, " ", &saveptr1);
    if (token != NULL) return 4;  // Should be NULL at end

    // Test 2: Multiple concurrent tokenizations (reentrance)
    strcpy(str1, "a:b:c");
    strcpy(str2, "1-2-3");

    token = strtok_r(str1, ":", &saveptr1);
    if (strcmp(token, "a") != 0) return 5;

    token = strtok_r(str2, "-", &saveptr2);
    if (strcmp(token, "1") != 0) return 6;

    // Continue with str1
    token = strtok_r(NULL, ":", &saveptr1);
    if (strcmp(token, "b") != 0) return 7;

    // Continue with str2
    token = strtok_r(NULL, "-", &saveptr2);
    if (strcmp(token, "2") != 0) return 8;

    // Finish str1
    token = strtok_r(NULL, ":", &saveptr1);
    if (strcmp(token, "c") != 0) return 9;

    token = strtok_r(NULL, ":", &saveptr1);
    if (token != NULL) return 10;

    // Finish str2
    token = strtok_r(NULL, "-", &saveptr2);
    if (strcmp(token, "3") != 0) return 11;

    // Test 3: String with consecutive delimiters (empty tokens skipped)
    strcpy(str1, "a::b");
    token = strtok_r(str1, ":", &saveptr1);
    if (strcmp(token, "a") != 0) return 12;

    token = strtok_r(NULL, ":", &saveptr1);
    if (strcmp(token, "b") != 0) return 13;

    // Test 4: String with no delimiters
    strcpy(str1, "single");
    token = strtok_r(str1, ":", &saveptr1);
    if (strcmp(token, "single") != 0) return 14;

    token = strtok_r(NULL, ":", &saveptr1);
    if (token != NULL) return 15;

    // Test 5: Multiple delimiters
    strcpy(str1, "a,b;c:d");
    token = strtok_r(str1, ",;:", &saveptr1);
    if (strcmp(token, "a") != 0) return 16;

    token = strtok_r(NULL, ",;:", &saveptr1);
    if (strcmp(token, "b") != 0) return 17;

    token = strtok_r(NULL, ",;:", &saveptr1);
    if (strcmp(token, "c") != 0) return 18;

    token = strtok_r(NULL, ",;:", &saveptr1);
    if (strcmp(token, "d") != 0) return 19;

    return 0;  // Success
}
