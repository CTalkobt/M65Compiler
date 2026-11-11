# fonts — font45 and <font.h>: a TrueType title in full-colour characters, a
# proportional 2 bpp paragraph, a 16-segment counter and an 8x8 charset
#
# Usage:
#   make              Build fonts.prg (font45 converts the fonts first)
#   make check        Build fontscheck.prg (runs 50 frames, then reports)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
FONT45 = $(ROOT)/bin/font45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = fonts.prg
TTF = RussoOne-Regular.ttf
FONTS = title body score term
FONT_OBJS = $(addsuffix .o45,$(FONTS))

.PHONY: all clean run check

all: $(TARGET)

# the title: lower case f-t (the glyph set holds every code in its range), 30
# pixels, two ramps down the glyphs (palette entries 32-34, 35-37), a shadow (38)
title.c: $(TTF) $(FONT45)
	$(FONT45) convert $(TTF) --size 30 --bpp 2 --fcm --palette 32,3 --gradient 2 --shadow 38 --proportional --range 70-84 -o title.c
# the paragraph: 11 pixels, 2 bpp, proportional
body.c: $(TTF) $(FONT45)
	$(FONT45) convert $(TTF) --size 11 --bpp 2 --proportional -o body.c
# the counter: a 16-segment display's digits, 24 pixels (unlit 48, lit 50)
score.c: $(FONT45)
	$(FONT45) convert --segments 16 --size 24 --bpp 2 --fcm --palette 48,3 --range 48-57 -o score.c
# conio's text: an 8x8 charset from the same font, in screen-code order, centred in the cells
term.c: $(TTF) $(FONT45)
	$(FONT45) convert $(TTF) --size 8 --charset screencode --center -o term.c

%.o45: %.c
	$(CC45) -O2 $(INCLUDES) -c $< -o $@

main.o45: main.c title.c body.c score.c term.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(FONT_OBJS) $(STDLIB)
	$(LN45) -basic -M fonts.map -o $(TARGET) main.o45 $(FONT_OBJS) $(STDLIB)

check: title.c body.c score.c term.c $(FONT_OBJS) $(STDLIB)
	$(CC45) -O2 -DFONTS_CHECK $(INCLUDES) -c main.c -o check.o45
	$(LN45) -basic -o fontscheck.prg check.o45 $(FONT_OBJS) $(STDLIB)

clean:
	rm -f *.o45 *.s45 $(TARGET) fontscheck.prg fonts.map $(addsuffix .c,$(FONTS)) $(addsuffix .h,$(FONTS))

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
