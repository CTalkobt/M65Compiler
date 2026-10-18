/* noescape/warn_fmt.c — the other overlay the calls can reach */
#pragma cc45 overlay(fmt)
unsigned int vfmt(const char *f, const char *s) { unsigned int n = 0; while (*f++) n++; while (*s++) n++; return n; }
