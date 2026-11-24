/* hw_raster_exit.c — hw_raster_init's atexit(hw_raster_done) (<hw_raster.h>,
 * #645): in C, so the call takes atexit's argument as the library's
 * convention has it (ZP or stack); hw_raster.s45 calls this (no
 * parameters: the same in both) */

#include <stdlib.h>
#include <hw_raster.h>

void __hw_raster_atexit(void) {
    atexit(hw_raster_done);
}
