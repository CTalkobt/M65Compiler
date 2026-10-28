/* tgi_vfload.c — <tgi.h>'s tgi_load_vectorfont / tgi_free_vectorfont
 *
 * A .vfn file (cc65's): "TCH", the version, the data's size, the data (a
 * struct tgi_vectorfont whose character pointers are offsets from vec_ops).
 * Read with fopen into malloc'd memory.
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <tgi/tgi-vectorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include "tgi_drv.h"

const tgi_vectorfont *tgi_load_vectorfont(const char *name) {
    tgi_vectorfont_header h;
    tgi_vectorfont *font = 0;
    FILE *f;
    unsigned char i;
    unsigned v;
    tgi_error = TGI_ERR_CANNOT_LOAD;
    f = fopen(name, "rb");
    if (!f) return 0;
    if (fread(&h, 1, sizeof h, f) != sizeof h) goto fail;
    if (h.magic[0] != 0x54 || h.magic[1] != 0x43 || h.magic[2] != 0x48 ||
        h.version != TGI_VF_VERSION) {
        tgi_error = TGI_ERR_INV_FONT;
        goto fail;
    }
    font = (tgi_vectorfont *)malloc(h.size);
    if (!font) {
        tgi_error = TGI_ERR_NO_RES;
        goto fail;
    }
    if (fread(font, 1, h.size, f) != h.size) {
        free(font);
        font = 0;
        goto fail;
    }
    fclose(f);
    v = (unsigned)font->vec_ops;
    for (i = 0; i < TGI_VF_CCOUNT; ++i)
        font->chars[i] = (unsigned char *)((unsigned)font->chars[i] + v);
    tgi_error = TGI_ERR_OK;
    return font;
fail:
    fclose(f);
    return 0;
}

void tgi_free_vectorfont(const tgi_vectorfont *font) {
    if (font == __tgi_vfont) __tgi_vfont = 0;
    free((void *)font);
}
