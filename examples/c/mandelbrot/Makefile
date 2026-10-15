# mandelbrot — a Mandelbrot set viewer (FCM bitmap, palette cycling, zoom)
#
# Usage:
#   make              Build mandelbrot.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
CA45 = $(ROOT)/bin/ca45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = mandelbrot.prg
OBJS = main.o45 mandel_asm.o45

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) $(INCLUDES) -c main.c -o main.o45

mandel_asm.o45: mandel_asm.s45
	$(CA45) -c mandel_asm.s45 -o mandel_asm.o45

$(TARGET): $(OBJS) $(STDLIB)
	$(LN45) -basic -M mandelbrot.map -o $(TARGET) $(OBJS) $(STDLIB)

clean:
	rm -f $(OBJS) main.o45.s45 $(TARGET) mandelbrot.map

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
