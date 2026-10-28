# map_decode — what the CPU sees in its 64 KB for a MAP / $01 / $D030
# configuration you set up, drawn with TGI
#
# Usage:
#   make              Build map_decode.prg
#   make check        Build the tests (-DMAP_CHECK: the decoder and the drawing;
#                     -DMAP_KEYS: the editor run by pressed keys), run them in
#                     xemu, check $C0C0
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

mapkeys.prg: main.c $(STDLIB)
	$(CC45) -O2 -DMAP_KEYS $(INCLUDES) main.c -o mapkeys.prg

# for each test, every result byte at $C0C0 is 01 up to the $AA sentinel
check: mapcheck.prg mapkeys.prg
	@ok=0; for t in mapcheck mapkeys; do \
	    timeout 40 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem $$t.mem -prg $$t.prg </dev/null >/dev/null 2>&1; \
	    python3 -c "import sys; d = open('$$t.mem', 'rb').read()[0xC0C0:0xC100]; n = d.index(0xAA) if 0xAA in d else -1; \
ok = n > 0 and all(b == 1 for b in d[:n]); print('map_decode $$t: %d of %d passed' % (sum(b == 1 for b in d[:max(n, 0)]), n) if n > 0 else 'map_decode $$t: no result'); sys.exit(0 if ok else 1)" || ok=1; \
	done; exit $$ok

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) map_decode.map mapcheck.prg mapcheck.mem mapkeys.prg mapkeys.mem

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
