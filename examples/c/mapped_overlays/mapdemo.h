/* mapdemo.h — what the resident part and the three overlays share */
#pragma once

/* What an overlay saw while it ran (filled in by look(), resident) */
struct seen {
    unsigned int cpu;         /* a byte of the overlay's code, as the CPU addresses it */
    unsigned char map_lo;     /* the MAP registers' lower half (A, X) at that moment, */
    unsigned char map_hi;     /*   as the KERNAL records them at $011C/$011D */
    unsigned char current;    /* __ovl_current: the overlay the runtime thinks is in */
};

/* resident (main.c): record what the calling overlay sees */
void look(struct seen *s, unsigned int cpu);

/* overlay red (red.c) */
void red_where(struct seen *s);
void red_poke(unsigned char v);       /* writes its own code byte through the window */
unsigned char red_peek(void);         /* reads it back through the window */

/* overlay green (green.c) */
void green_where(struct seen *s);
void green_then_blue(struct seen *before, struct seen *inside, struct seen *after);

/* overlay blue (blue.c) */
void blue_where(struct seen *s);
