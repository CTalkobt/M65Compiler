/* mega65-unpack-format (#666): streams whose format is visible, in the file
 * with main(), against the decoders the file links. <unpack.h> links format
 * 1 by default; format 2 needs UNPACK_FORMAT2, which this file lacks.
 * (ufmt2.prg beside it: a format 2 stream with its load address) */
#include <unpack.h>
#include <load.h>

static const unsigned char tiles[] = {2, 0, 0, 4, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0};    /* format 2 */
static const unsigned char level[] = {1, 0, 0, 4, 0, 0x40, 0x55, 0xC0, 0, 0};       /* format 1 */
static const unsigned char odd[] = {7, 0, 0, 4, 0};                                  /* no format */
unsigned char runtime[16];

int main(void) {
    unpack((unsigned long)(unsigned int)tiles);                   // EXPECT mega65-unpack-format
    unpack_to((unsigned long)&tiles[0], 0x50000UL);               // EXPECT mega65-unpack-format
    unpack((unsigned long)(unsigned int)level);                   /* format 1: linked by default */
    unpack((unsigned long)(unsigned int)odd);                     /* not a format: not guessed */
    unpack((unsigned long)(unsigned int)runtime);                 /* its bytes are not known */
    unpack((unsigned long)(unsigned int)tiles);                   // NOLINT(mega65-unpack-format)
    load_unpack("ufmt2", 5, 0, 0x41000UL);                        // EXPECT mega65-unpack-format
    load_unpack("missing", 7, 0, 0x41000UL);                      /* no such file: not known */
    return 0;
}
