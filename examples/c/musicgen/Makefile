# musicgen — a random music generator (styles, tempo, SID instruments)
#
# Usage:
#   make              Build musicgen.prg
#   make check        Build the generator test, run it in xemu and check its songs
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = musicgen.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M musicgen.map -o $(TARGET) main.o45 $(STDLIB)

mgtest.prg: main.c $(STDLIB)
	$(CC45) -O2 -DMG_TEST $(INCLUDES) main.c -o mgtest.prg

check: mgtest.prg
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem mgtest.mem -prg mgtest.prg </dev/null >/dev/null 2>&1; \
	python3 check_songs.py mgtest.mem

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) musicgen.map mgtest.prg mgtest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
