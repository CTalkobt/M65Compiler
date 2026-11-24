/* hw_raster_cb.c — hw_raster_set_callback (<hw_raster.h>, #645): a plain
 * function at a raster line, called from __hw_raster_cbisr, the library's
 * __interrupt handler for those lines (hw_raster.s45 enters it as any
 * line's __interrupt function). Its call through a pointer is followed by
 * ln45 from an interrupt: the callbacks need stack frames. Linked only
 * when hw_raster_set_callback is called. */

#include <hw_raster.h>

/* the line's function in hw_raster.s45's table (0: none) */
typedef void (*hwr_fn_t)(void);
__fastcall__ hwr_fn_t __hw_raster_fn(int line);

static struct {
    int line;
    raster_callback_t fn;
    void *user;
} cbs[HW_RASTER_MAX + 1];

__interrupt void __hw_raster_cbisr(void) {
    int line = hw_raster_current();
    unsigned char i;
    for (i = 0; i < HW_RASTER_MAX + 1; i++) {
        if (cbs[i].fn && cbs[i].line == line) {
            cbs[i].fn(line, cbs[i].user);
            return;
        }
    }
}

__fastcall__ int hw_raster_set_callback(int line, raster_callback_t callback, void *user_data) {
    unsigned char i, slot = 0xFF;
    if (!callback) {
        hw_raster_clear_callback(line);
        return 1;
    }
    for (i = 0; i < HW_RASTER_MAX + 1; i++) {
        /* (a slot whose line has been cleared or given another function) */
        if (cbs[i].fn && __hw_raster_fn(cbs[i].line) != __hw_raster_cbisr) cbs[i].fn = 0;
        if (cbs[i].fn && cbs[i].line == line) {
            slot = i;
            break;
        }
        if (!cbs[i].fn && slot == 0xFF) slot = i;
    }
    if (slot == 0xFF) return 0;
    /* (the function off while its entry changes) */
    cbs[slot].fn = 0;
    cbs[slot].line = line;
    cbs[slot].user = user_data;
    cbs[slot].fn = callback;
    if (!hw_raster_set_isr(line, __hw_raster_cbisr)) {
        cbs[slot].fn = 0;
        return 0;
    }
    return 1;
}
