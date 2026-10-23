# hanger — hangman with a different animal on the gallows each level
#
# Usage:
#   make              Build hanger.prg
#   make demo         Build hangerdemo.prg, which plays by itself (DEMO_LEVEL=0-5: the first level)
#   make gfx          Regenerate gfx.h and words.h (gen_gfx.py; needs Python's PIL)
#   make preview      preview.png: every stage of every animal
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65
DEMO_LEVEL ?= 0

TARGET = hanger.prg

.PHONY: all clean run demo gfx preview

all: $(TARGET)

main.o45: main.c gfx.h words.h
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M hanger.map -o $(TARGET) main.o45 $(STDLIB)

demo: main.c gfx.h words.h $(STDLIB)
	$(CC45) -O2 -DDEMO -DDEMO_LEVEL=$(DEMO_LEVEL) $(INCLUDES) main.c -o hangerdemo.prg

gfx:
	python3 gen_gfx.py

preview:
	python3 gen_gfx.py --preview

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) hanger.map hangerdemo.prg preview.png

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
