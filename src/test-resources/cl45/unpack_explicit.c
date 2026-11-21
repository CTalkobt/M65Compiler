/* mega65-unpack-format (#666) with the formats chosen: format 2 linked,
 * format 1 not (UNPACK_EXPLICIT_FORMATS). The format 1 stream comes from
 * #embed (ufmt1.raw beside it). */
#define UNPACK_EXPLICIT_FORMATS
#include <unpack.h>

UNPACK_FORMAT2;

static const unsigned char tiles[] = {2, 0, 0, 4, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0};
static const unsigned char level[] = {
#embed "ufmt1.raw"
};

int main(void) {
    unpack((unsigned long)(unsigned int)tiles);                   /* format 2: linked */
    unpack((unsigned long)(unsigned int)level);                   // EXPECT mega65-unpack-format
    return 0;
}
