# sample_player — a drum machine on the audio DMA (synthesized samples)
#
# Usage:
#   make              Build sample_player.prg
#   make check        Build the sample test, run it in xemu and check the result
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = sample_player.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M sample_player.map -o $(TARGET) main.o45 $(STDLIB)

sptest.prg: main.c $(STDLIB)
	$(CC45) -O2 -DSP_TEST $(INCLUDES) main.c -o sptest.prg

# $C0C0: kick, snare/hi-hat and tone sample data, four channels playing,
# the tone's rate, stopped, $AA
check: sptest.prg
	rm -f sptest.mem
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem sptest.mem -prg sptest.prg </dev/null >/dev/null 2>&1; \
	test "$$(od -An -tx1 -j $$((0xC0C0)) -N 7 sptest.mem | tr -d ' \n')" = "010101010101aa" && echo "sample check: pass" || { echo "sample check: FAIL"; exit 1; }

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) sample_player.map sptest.prg sptest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
