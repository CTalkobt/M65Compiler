# tgi_demo — cc65's TGI on the MEGA65: the drawing functions, then a
# double-buffered animation
#
# Usage:
#   make              Build tgi_demo.prg
#   make check        Build tgicheck.prg (draws, animates 50 frames, then reports)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = tgi_demo.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M tgi_demo.map -o $(TARGET) main.o45 $(STDLIB)

check: main.c $(STDLIB)
	$(CC45) -O2 -DTGI_CHECK $(INCLUDES) main.c -o tgicheck.prg

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) tgi_demo.map tgicheck.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
