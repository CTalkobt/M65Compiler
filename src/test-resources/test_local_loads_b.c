// The other file of test_local_loads_xemu.c (#659): what changes a struct
// behind the caller's back (not inlined: another file)
struct line { unsigned start, y; unsigned char h, asc; };
struct line G;
static struct line *kept;
void ll_get(unsigned l, struct line *L) { L->start = l; L->y = l + 1; L->h = (unsigned char)(l + 2); L->asc = 0; kept = L; }
void ll_bump(void) { kept->y += 10; kept->h += 1; }
void ll_touch_g(void) { G.y += 100; }
void ll_nothing(void) { }
