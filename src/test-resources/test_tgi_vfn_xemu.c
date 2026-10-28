/* test_tgi_vfn_xemu.c — <tgi.h>'s vector font files (cc65's .vfn)
 *
 * Run with an empty D81 in drive 8: the files are written, then loaded.
 * Results at $C0C0 (01 = pass):
 *   [0] the files written (a font, one with a bad magic)
 *   [1] a missing file: NULL, TGI_ERR_CANNOT_LOAD; a bad magic: NULL,
 *       TGI_ERR_INV_FONT
 *   [2] tgi_load_vectorfont: the widths, the character pointers made from
 *       the file's offsets
 *   [3] the loaded font drawn (PETSCII text as ASCII characters), the
 *       cursor; tgi_free_vectorfont
 *   [4] 01
 *   [5] $AA sentinel
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <stdio.h>

#define OUT ((volatile unsigned char *)0xC0C0)

/* A vector font: every character a move to (0,0) and a line to (4,8),
 * 6 wide; as in a file, the character pointers are offsets from the
 * operations */
static struct {
    unsigned char top, baseline, bottom;
    unsigned char widths[TGI_VF_CCOUNT];
    unsigned char *chars[TGI_VF_CCOUNT];
    unsigned char ops[4];
} vf;

static unsigned char write_font(const char *name, unsigned char magic) {
    unsigned char hdr[6];
    unsigned int size = sizeof vf;
    FILE *o = fopen(name, "wb");
    if (!o) return 0;
    hdr[0] = magic;
    hdr[1] = 0x43;
    hdr[2] = 0x48;
    hdr[3] = TGI_VF_VERSION;
    hdr[4] = (unsigned char)size;
    hdr[5] = (unsigned char)(size >> 8);
    if (fwrite(hdr, 1, 6, o) != 6 || fwrite(&vf, 1, size, o) != size) {
        fclose(o);
        return 0;
    }
    fclose(o);
    return 1;
}

static unsigned char t_write(void) {
    unsigned char i;
    vf.top = 8;
    vf.bottom = 2;
    vf.ops[2] = 0x84;           /* draw to 4, 8; the last */
    vf.ops[3] = 0x88;
    for (i = 0; i < TGI_VF_CCOUNT; i++) vf.widths[i] = 6;
    vf.widths[0x69 - TGI_VF_FIRSTCHAR] = 3;     /* ASCII 'i' (the file's characters) */
    return write_font("font.vfn", 0x54) && write_font("bad.vfn", 0x58);
}

static unsigned char t_errors(void) {
    if (tgi_load_vectorfont("none.vfn") || tgi_geterror() != TGI_ERR_CANNOT_LOAD) return 0;
    if (tgi_load_vectorfont("bad.vfn") || tgi_geterror() != TGI_ERR_INV_FONT) return 0;
    return 1;
}

static const tgi_vectorfont *f;

static unsigned char t_load(void) {
    f = tgi_load_vectorfont("font.vfn");
    if (!f || tgi_geterror() != TGI_ERR_OK) return 0;
    if (f->top != 8 || f->widths[0] != 6 || f->widths[0x69 - TGI_VF_FIRSTCHAR] != 3) return 0;
    return f->chars[0] == f->vec_ops && f->chars[TGI_VF_CCOUNT - 1] == f->vec_ops;
}

static unsigned char t_draw(void) {
    if (tgi_install(tgi_static_stddrv) != TGI_ERR_OK) return 0;
    tgi_init();
    tgi_install_vectorfont(f);
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_VECTOR);
    if (tgi_gettextwidth("Hi") != 9) return 0;  /* 'H' 6, 'i' 3 */
    tgi_clear();
    tgi_outtextxy(10, 50, "Hi");
    if (tgi_getpixel(10, 50) != TGI_COLOR_WHITE || tgi_getpixel(14, 42) != TGI_COLOR_WHITE) return 0;
    if (tgi_getpixel(16, 50) != TGI_COLOR_WHITE || tgi_getpixel(20, 42) != TGI_COLOR_WHITE) return 0;
    if (tgi_curx != 19 || tgi_cury != 50) return 0;
    tgi_uninstall();
    tgi_free_vectorfont(f);
    return 1;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 5; i++) OUT[i] = 0;
    OUT[5] = 0xAA;
    OUT[0] = t_write();
    OUT[1] = t_errors();
    OUT[2] = t_load();
    OUT[3] = t_draw();
    OUT[4] = 1;
    return 0;
}
