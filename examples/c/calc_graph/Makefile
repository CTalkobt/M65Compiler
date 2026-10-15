# calc_graph — a scientific calculator that plots, on a raster-split screen
#
# Usage:
#   make              Build calc_graph.prg
#   make check        Build the test (-DCALC_TEST), run it in xemu, check $C0C0
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = calc_graph.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M calc_graph.map -o $(TARGET) main.o45 $(STDLIB)

calctest.prg: main.c $(STDLIB)
	$(CC45) -O2 -DCALC_TEST $(INCLUDES) main.c -o calctest.prg

# every result byte at $C0C0 is 01 up to the $AA sentinel
check: calctest.prg
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem calctest.mem -prg calctest.prg </dev/null >/dev/null 2>&1; \
	python3 -c "import sys; d = open('calctest.mem', 'rb').read()[0xC0C0:0xC100]; n = d.index(0xAA) if 0xAA in d else -1; \
ok = n > 0 and all(b == 1 for b in d[:n]); print('calc_graph check: %d of %d passed' % (sum(b == 1 for b in d[:max(n, 0)]), n) if n > 0 else 'calc_graph check: no result'); sys.exit(0 if ok else 1)"

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) calc_graph.map calctest.prg calctest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
