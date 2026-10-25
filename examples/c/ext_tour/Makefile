# ext_tour — a tour of cc45's extensions to C (one per line on the screen)
#
# Usage:
#   make              Build ext_tour.prg
#   make check        Build it with both calling conventions, run both in
#                     xemu, check the screen says every extension works
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
XEMU = xemu-xmega65

TARGET = ext_tour.prg

.PHONY: all clean run check

all: $(TARGET)

$(TARGET): main.c
	$(CC45) -O2 main.c -o $(TARGET)

ext_tour_stack.prg: main.c
	$(CC45) -O2 -fno-zpcall main.c -o ext_tour_stack.prg

# the screen's last line: "N/N EXTENSIONS WORK" (N the same twice), and
# each build names its own convention (__CC45_ZPCALL__ / __CC45_STACK__)
check: $(TARGET) ext_tour_stack.prg
	@for pc in $(TARGET):ZP ext_tour_stack.prg:STACK; do \
	    p=$${pc%:*}; c=$${pc#*:}; \
	    rm -f check.mem; \
	    timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem check.mem -prg $$p </dev/null >/dev/null 2>&1; \
	    python3 -c "import re, sys; d = open('check.mem', 'rb').read()[0x800:0x800 + 2000]; \
t = ''.join(chr(b + 64) if 1 <= b <= 26 else chr(b) if 32 <= b < 64 or 65 <= b <= 90 else ' ' for b in d); \
m = re.search(r'(\d+)/(\d+) EXTENSIONS WORK', t); \
ok = m and m.group(1) == m.group(2) and '$$c CONVENTION BUILD' in t; print('$$p: ' + (m.group(0) if m else 'no result') + ('' if '$$c CONVENTION BUILD' in t else ', not the $$c convention')); sys.exit(0 if ok else 1)" || exit 1; \
	done

clean:
	rm -f $(TARGET) ext_tour_stack.prg check.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
