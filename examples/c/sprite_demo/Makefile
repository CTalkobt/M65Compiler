# sprite_demo — "The Farmer and the Fish": a short movie with the VIC-IV's
# 8 hardware sprites (<sprites.h>) over a TGI picture
#
# Usage:
#   make              Build sprite_demo.prg
#   make check        Build spritecheck.prg (the movie once, then reports)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = sprite_demo.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M sprite_demo.map -o $(TARGET) main.o45 $(STDLIB)

check: main.c $(STDLIB)
	$(CC45) -O2 -DSPRITE_CHECK $(INCLUDES) main.c -o spritecheck.prg

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) sprite_demo.map spritecheck.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
