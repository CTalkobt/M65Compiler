# forth — a tiny Forth (a programmable calculator) with a computed-goto VM
#
# Usage:
#   make              Build forth.prg
#   make check        Build the test (a script of lines), run it in xemu, check $C0C0
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
CFLAGS = -O2
XEMU = xemu-xmega65

TARGET = forth.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) $(CFLAGS) $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M forth.map -o $(TARGET) main.o45 $(STDLIB)

forthtest.prg: main.c $(STDLIB)
	$(CC45) $(CFLAGS) -DFORTH_TEST $(INCLUDES) main.c -o forthtest.prg

# 21 lines of Forth and their output: 01 for each, then 01 (bye) and $AA
check: forthtest.prg
	rm -f forthtest.mem
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem forthtest.mem -prg forthtest.prg </dev/null >/dev/null 2>&1; \
	python3 -c "import sys; d = open('forthtest.mem', 'rb').read()[0xC0C0:0xC0C0 + 22]; \
	ok = d == bytes([1] * 21 + [0xAA]); print('forth checks: ' + ('all passed' if ok else 'FAILED ' + d.hex())); sys.exit(0 if ok else 1)"

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) forth.map forthtest.prg forthtest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
