#include <far_ptr.h>

/* far_attic_enable: far_attic_init with a 4 KB granule and its table here
 * (a program that gives its own, far_attic_init, does not link it), and
 * far_malloc / far_free & co. reaching it (far_attic_hook) */
static unsigned char g_meta[FAR_ATTIC_META_SIZE(12)];

unsigned long far_attic_enable(unsigned long first, unsigned long last) {
    far_attic_hook();
    return far_attic_init(first, last, 12, g_meta, sizeof g_meta);
}
