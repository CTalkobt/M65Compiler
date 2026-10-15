# rogue — a mini rogue-like (random dungeons, monsters, items)
#
# Usage:
#   make              Build rogue.prg
#   make check        Build the test, run it in xemu and check its results
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = rogue.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M rogue.map -o $(TARGET) main.o45 $(STDLIB)

roguetest.prg: main.c $(STDLIB)
	$(CC45) -O2 -DROGUE_TEST $(INCLUDES) main.c -o roguetest.prg

# results at $C0C0: 01 01 01 01 01 01 AA
check: roguetest.prg
	rm -f roguetest.mem
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem roguetest.mem -prg roguetest.prg </dev/null >/dev/null 2>&1; \
	r=$$(od -An -tx1 -j $$((0xC0C0)) -N 7 roguetest.mem | tr -d ' \n'); \
	echo "results: $$r"; [ "$$r" = "010101010101aa" ]

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) rogue.map roguetest.prg roguetest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
