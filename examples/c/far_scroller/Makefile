# far_scroller — a landscape map in bank 4 scrolled with DMA (parallax)
#
# Usage:
#   make              Build far_scroller.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = far_scroller.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M far_scroller.map -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) far_scroller.map

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
