// cl45 fixture: suppression. NOLINT on the line, NOLINTNEXTLINE before it,
// #pragma cc45 lint(ignore, id) to #pragma cc45 lint(restore, id).
unsigned int n;
unsigned char c;

void f(void) {
    c = n;                              // EXPECT mega65-implicit-narrowing
    c = n;                              // NOLINT
    c = n;                              /* NOLINT(mega65-implicit-narrowing): the low byte is the value */
    c = n;                              // NOLINT(mega65-signedness) EXPECT mega65-implicit-narrowing
    // NOLINTNEXTLINE(mega65-implicit-narrowing)
    c = n;
#pragma cc45 lint(ignore, mega65-implicit-narrowing)
    c = n;
    c = n + 2;
#pragma cc45 lint(restore, mega65-implicit-narrowing)
    c = n;                              // EXPECT mega65-implicit-narrowing
}
