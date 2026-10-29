/* tgi/tgi-vectorfont.h — TGI vector fonts (cc65's .vfn files)
 *
 * A .vfn file is the header below, then the font data (struct
 * tgi_vectorfont): the character pointers hold offsets from vec_ops, which
 * tgi_load_vectorfont makes into pointers. Each character is a list of
 * operations, two bytes each, 7-bit signed X and Y offsets from the
 * character's origin (Y up): bit 7 of the Y byte draws a line from the
 * previous point (else only moves), bit 7 of the X byte ends the character.
 * Characters are ASCII $20-$7E; tgi_outtext converts PETSCII to ASCII.
 */

#pragma once

#define TGI_VF_FIRSTCHAR        0x20            /* First char in file */
#define TGI_VF_LASTCHAR         0x7E            /* Last char in file */
#define TGI_VF_CCOUNT           (TGI_VF_LASTCHAR - TGI_VF_FIRSTCHAR + 1)

#define TGI_VF_VERSION          0x00            /* File version number */

/* TCH file header */
typedef struct tgi_vectorfont_header tgi_vectorfont_header;
struct tgi_vectorfont_header {
    unsigned char       magic[3];               /* "TCH" */
    unsigned char       version;                /* Version number */
    unsigned            size;                   /* Font data size */
};

/* Font data loaded directly from file */
struct tgi_vectorfont {
    unsigned char       top;                    /* Height of char */
    unsigned char       baseline;               /* Character baseline */
    unsigned char       bottom;                 /* Descender */
    unsigned char       widths[TGI_VF_CCOUNT];  /* Char widths */
    unsigned char*      chars[TGI_VF_CCOUNT];   /* Pointer to character defs */
    unsigned char       vec_ops[1];             /* Actually dynamic */
};

/* Draw one character of the vector font at the graphics cursor, at the
 * current text scale and direction (the cursor does not move) */
__fastcall__ void tgi_vectorchar(char c);
