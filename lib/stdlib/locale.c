/* locale.c — setlocale / localeconv (<locale.h>): the "C" locale only */

#include <locale.h>
#include <string.h>

static char c_name[] = "C";
static char empty[] = "";
static char point[] = ".";

static struct lconv c_lconv = {
    point, empty, empty, empty, empty, empty, empty, empty, empty, empty,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

char *setlocale(int category, const char *locale) {
    if (category < LC_ALL || category > LC_TIME) return NULL;
    if (!locale || !*locale || !strcmp(locale, "C") || !strcmp(locale, "POSIX")) return c_name;
    return NULL;
}

struct lconv *localeconv(void) {
    return &c_lconv;
}
