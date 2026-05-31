// test_strtok: validate strtok() - string tokenization
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Basic tokenization with single delimiter
    char str1[16] = "hello:world";
    char copy1[16];
    strcpy(copy1, str1);
    char *tok = strtok(copy1, ":");
    if (strcmp(tok, "hello") != 0) return 1;
    tok = strtok(0, ":");
    if (strcmp(tok, "world") != 0) return 2;
    tok = strtok(0, ":");
    if (tok != 0) return 3;

    // Test 2: Tokenization with multiple delimiters
    char str2[16] = "a:b,c;d";
    char copy2[16];
    strcpy(copy2, str2);
    tok = strtok(copy2, ":,;");
    if (strcmp(tok, "a") != 0) return 4;
    tok = strtok(0, ":,;");
    if (strcmp(tok, "b") != 0) return 5;
    tok = strtok(0, ":,;");
    if (strcmp(tok, "c") != 0) return 6;
    tok = strtok(0, ":,;");
    if (strcmp(tok, "d") != 0) return 7;
    tok = strtok(0, ":,;");
    if (tok != 0) return 8;

    // Test 3: Empty tokens (consecutive delimiters)
    char str3[16] = "a::b";
    char copy3[16];
    strcpy(copy3, str3);
    tok = strtok(copy3, ":");
    if (strcmp(tok, "a") != 0) return 9;
    tok = strtok(0, ":");
    if (strcmp(tok, "b") != 0) return 10;

    // Test 4: Token at start
    char str4[16] = ":hello";
    char copy4[16];
    strcpy(copy4, str4);
    tok = strtok(copy4, ":");
    if (strcmp(tok, "hello") != 0) return 11;

    // Test 5: Token at end
    char str5[16] = "hello:";
    char copy5[16];
    strcpy(copy5, str5);
    tok = strtok(copy5, ":");
    if (strcmp(tok, "hello") != 0) return 12;
    tok = strtok(0, ":");
    if (tok != 0) return 13;

    // Test 6: Single character token
    char str6[16] = "a:b:c";
    char copy6[16];
    strcpy(copy6, str6);
    tok = strtok(copy6, ":");
    if (strcmp(tok, "a") != 0) return 14;
    tok = strtok(0, ":");
    if (strcmp(tok, "b") != 0) return 15;
    tok = strtok(0, ":");
    if (strcmp(tok, "c") != 0) return 16;

    // Test 7: Space as delimiter
    char str7[16] = "hello world test";
    char copy7[16];
    strcpy(copy7, str7);
    tok = strtok(copy7, " ");
    if (strcmp(tok, "hello") != 0) return 17;
    tok = strtok(0, " ");
    if (strcmp(tok, "world") != 0) return 18;
    tok = strtok(0, " ");
    if (strcmp(tok, "test") != 0) return 19;

    // Test 8: Change delimiter mid-parse
    char str8[16] = "a:b,c";
    char copy8[16];
    strcpy(copy8, str8);
    tok = strtok(copy8, ":");
    if (strcmp(tok, "a") != 0) return 20;
    tok = strtok(0, ",");  // Change delimiter
    if (strcmp(tok, "b") != 0) return 21;
    tok = strtok(0, ",");
    if (strcmp(tok, "c") != 0) return 22;

    // Test 9: All delimiters (no tokens)
    char str9[8] = ":::";
    char copy9[8];
    strcpy(copy9, str9);
    tok = strtok(copy9, ":");
    if (tok != 0) return 23;

    // Test 10: Longer delimiters
    char str10[16] = "a--b--c";
    char copy10[16];
    strcpy(copy10, str10);
    tok = strtok(copy10, "-");
    if (strcmp(tok, "a") != 0) return 24;
    tok = strtok(0, "-");
    if (strcmp(tok, "b") != 0) return 25;
    tok = strtok(0, "-");
    if (strcmp(tok, "c") != 0) return 26;

    return 0;
}
