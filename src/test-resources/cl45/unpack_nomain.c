/* mega65-unpack-format (#666): a file without main() is not checked (another
 * file of the program may link the decoder: ln45 checks the whole program) */
#include <unpack.h>

static const unsigned char tiles[] = {2, 0, 0, 4, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0};

unsigned long show(void) {
    return unpack((unsigned long)(unsigned int)tiles);
}
