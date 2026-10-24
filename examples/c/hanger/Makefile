# hanger — hangman with a different animal on the gallows each level
#
# Usage:
#   make              Build hanger.prg (tune45 convert makes sounds.c of sounds.txt)
#   make demo         Build hangerdemo.prg, which plays by itself (DEMO_LEVEL=0-5: the first level)
#   make gfx          Regenerate gfx.h and words.h (gen_gfx.py; needs Python's PIL)
#   make preview      preview.png: every stage of every animal
#   make sounds       Play the sounds on the PC (tune45 play)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
TUNE45 = $(ROOT)/bin/tune45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65
DEMO_LEVEL ?= 0

TARGET = hanger.prg

.PHONY: all clean run demo gfx preview sounds

all: $(TARGET)

sounds.c sounds.h: sounds.txt $(TUNE45)
	$(TUNE45) convert sounds.txt -o sounds.c

main.o45: main.c gfx.h words.h sounds.h
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

sounds.o45: sounds.c sounds.h
	$(CC45) -O2 $(INCLUDES) -c sounds.c -o sounds.o45

$(TARGET): main.o45 sounds.o45 $(STDLIB)
	$(LN45) -basic -M hanger.map -o $(TARGET) main.o45 sounds.o45 $(STDLIB)

demo: main.c gfx.h words.h sounds.o45 $(STDLIB)
	$(CC45) -O2 -DDEMO -DDEMO_LEVEL=$(DEMO_LEVEL) $(INCLUDES) -c main.c -o demo.o45
	$(LN45) -basic -o hangerdemo.prg demo.o45 sounds.o45 $(STDLIB)

sounds: sounds.txt
	$(TUNE45) play sounds.txt

gfx:
	python3 gen_gfx.py

preview:
	python3 gen_gfx.py --preview

clean:
	rm -f main.o45 main.o45.s45 sounds.o45 demo.o45 sounds.c sounds.h $(TARGET) hanger.map hangerdemo.prg preview.png

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
