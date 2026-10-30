# tgi_demo — cc65's TGI on the MEGA65: the drawing functions, then a
# double-buffered animation, at 320x200 in 256 colours and 640x200 in 16
#
# Usage:
#   make              Build tgi_demo.prg
#   make check        Build tgicheck.prg (both modes, 50 frames each, then reports)
#   make demo640      Build tgi_demo640.prg (the 640x200 part only)
#   make patterns     Build tgi_patterns.prg / tgi_patterns640.prg (the fill
#                     patterns' page alone, at 320x200 / 640x200)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = tgi_demo.prg

.PHONY: all clean run check demo640 patterns

all: $(TARGET) tgi_demo640.prg tgi_patterns.prg tgi_patterns640.prg

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M tgi_demo.map -o $(TARGET) main.o45 $(STDLIB)

check: main.c $(STDLIB)
	$(CC45) -O2 -DTGI_CHECK $(INCLUDES) main.c -o tgicheck.prg

demo640: tgi_demo640.prg

tgi_demo640.prg: main.c $(STDLIB)
	$(CC45) -O2 -DTGI_640_ONLY $(INCLUDES) main.c -o tgi_demo640.prg

patterns: tgi_patterns.prg tgi_patterns640.prg

tgi_patterns.prg: main.c $(STDLIB)
	$(CC45) -O2 -DTGI_PATTERNS $(INCLUDES) main.c -o tgi_patterns.prg

tgi_patterns640.prg: main.c $(STDLIB)
	$(CC45) -O2 -DTGI_PATTERNS -DTGI_640_ONLY $(INCLUDES) main.c -o tgi_patterns640.prg

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) tgi_demo.map tgicheck.prg tgi_demo640.prg tgi_patterns.prg tgi_patterns640.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
