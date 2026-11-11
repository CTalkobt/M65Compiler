/* font_load.c — <font.h>'s font_load (#466): a font file font45 wrote,
 * loaded by <load.h> and unpacked if it is a pack45 stream. A member of its
 * own: only a program that loads fonts links load.c. */
#include <font.h>
#include <load.h>
#include <unpack.h>

static font_t loaded;

const font_t *font_load(const char *name, unsigned char len, unsigned long lin) {
    const unsigned char __far *p = (const unsigned char __far *)lin;
    if (!load_far(name, len, 0, lin)) return 0;
    if (p[0] == 1) {
        /* a pack45 stream (format 1): unpacked to the address in its header */
        unsigned long dest = p[1] | ((unsigned long)p[2] << 8) | ((unsigned long)p[3] << 16) | ((unsigned long)p[4] << 24);
        if (!unpack(lin)) return 0;
        lin = dest;
    }
    return font_from(&loaded, lin);
}
