# invaders — a Space Invaders game (objects, VIC-IV sprites, SID, input)
#
# Usage:
#   make              Build invaders.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = invaders.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c gfx.h
	$(CC45) $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M invaders.map -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) invaders.map

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
