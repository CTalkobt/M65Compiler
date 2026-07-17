# vic4_colors — Set screen/border colors via VIC-IV register struct
#
# Maps VIC-IV registers and sets border/screen color to white.
#
# Usage:
#   make              Build vic4_colors.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = vic4_colors.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M vic4_colors.map -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) vic4_colors.map

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
