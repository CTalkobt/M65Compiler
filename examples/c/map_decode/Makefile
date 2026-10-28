# map_decode — what the CPU sees in its 64 KB for a MAP / $01 / $D030
# configuration you set up, drawn with TGI
#
# Usage:
#   make              Build map_decode.prg
#   make check        Build the test (-DMAP_CHECK), run it in xemu, check $C0C0
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = map_decode.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M map_decode.map -o $(TARGET) main.o45 $(STDLIB)

mapcheck.prg: main.c $(STDLIB)
	$(CC45) -O2 -DMAP_CHECK $(INCLUDES) main.c -o mapcheck.prg

# every result byte at $C0C0 is 01 up to the $AA sentinel
check: mapcheck.prg
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem mapcheck.mem -prg mapcheck.prg </dev/null >/dev/null 2>&1; \
	python3 -c "import sys; d = open('mapcheck.mem', 'rb').read()[0xC0C0:0xC100]; n = d.index(0xAA) if 0xAA in d else -1; \
ok = n > 0 and all(b == 1 for b in d[:n]); print('map_decode check: %d of %d passed' % (sum(b == 1 for b in d[:max(n, 0)]), n) if n > 0 else 'map_decode check: no result'); sys.exit(0 if ok else 1)"

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) map_decode.map mapcheck.prg mapcheck.mem

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
