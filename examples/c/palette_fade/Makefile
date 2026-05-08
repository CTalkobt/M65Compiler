# Palette fade example for cc45 / MEGA65
#
# Demonstrates C+Assembly calling conventions with VIC-IV palette manipulation.
#
# Usage:
#   make              Build palette_fade.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
CA45 = $(ROOT)/bin/ca45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = palette_fade.prg

.PHONY: all clean run

all: $(TARGET)

palette_fade.o45: palette_fade.c
	$(CC45) -fzpcall $(INCLUDES) -c palette_fade.c -o palette_fade.o45

palette_fade_asm.o45: palette_fade_asm.s
	$(CA45) -c palette_fade_asm.s -o palette_fade_asm.o45

$(TARGET): palette_fade.o45 palette_fade_asm.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(CRT0) palette_fade.o45 palette_fade_asm.o45 $(STDLIB)

clean:
	rm -f palette_fade.o45 palette_fade.o45.s palette_fade_asm.o45 $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
