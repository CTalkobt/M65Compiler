# mandelbrot — a Mandelbrot set viewer (FCM bitmap, palette cycling, zoom)
#
# Usage:
#   make              Build mandelbrot.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)
#   make check        A test build: renders two views, checks the sums of
#                     their pixels and status lines

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
CA45 = $(ROOT)/bin/ca45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = mandelbrot.prg
OBJS = main.o45 mandel_asm.o45

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

mandel_asm.o45: mandel_asm.s45
	$(CA45) -c mandel_asm.s45 -o mandel_asm.o45

$(TARGET): $(OBJS) $(STDLIB)
	$(LN45) -basic -M mandelbrot.map -o $(TARGET) $(OBJS) $(STDLIB)

# the sums of the two views' pixels and status lines (see MB_CHECK in main.c)
CHECK_SUMS = AD4C B8B8 2D9C D0D1
check: mandel_asm.o45
	$(CC45) -O2 $(INCLUDES) -DMB_CHECK -c main.c -o check.o45
	$(LN45) -basic -o mbcheck.prg check.o45 mandel_asm.o45 $(STDLIB)
	@rm -f check.mem
	@timeout 120 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem check.mem -prg mbcheck.prg </dev/null >/dev/null 2>&1; \
	python3 -c "import struct; d = open('check.mem', 'rb').read(); s = struct.unpack('<4H', d[0xC0C0:0xC0C8]); \
r = ' '.join('%04X' % v for v in s); print('mandelbrot check: ' + r); import sys; sys.exit(0 if r == '$(CHECK_SUMS)' else 1)"

clean:
	rm -f $(OBJS) main.o45.s45 $(TARGET) mandelbrot.map check.o45 mbcheck.prg check.mem

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
