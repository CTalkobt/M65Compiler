/* noescape/warn_main.c — resident: msgf reaches overlay fmt, show does not */
unsigned int vfmt(const char *f, const char *s);
void a_run(unsigned char i, unsigned char b);
extern void (*hook)(const char *);
static unsigned int last;
void msgf(const char *f, const char *s) { last = vfmt(f, s); }
void show(const char *s) { last = *s; }
static void hook_fmt(const char *s) { last = vfmt(s, s); }
int main(void) { hook = hook_fmt; a_run(0, 1); return 0; }
