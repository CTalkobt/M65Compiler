# drums — a SID band and a sampled drum kit (digi voices), played by <audio_tune.h>
#
# Usage:
#   make              Build drums.prg (tune45 convert makes music.c of music.txt,
#                     the samples too)
#   make check        Build drumscheck.prg (plays 240 frames, reports)
#   make play         Play the music on the PC (tune45 play)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
TUNE45 = $(ROOT)/bin/tune45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = drums.prg

.PHONY: all clean run check play

all: $(TARGET)

music.c music.h: music.txt samples/lucia_kit_013.mp3 $(TUNE45)
	$(TUNE45) convert music.txt -o music.c

main.o45: main.c music.h
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

music.o45: music.c music.h
	$(CC45) -O2 $(INCLUDES) -c music.c -o music.o45

$(TARGET): main.o45 music.o45 $(STDLIB)
	$(LN45) -basic -M drums.map -o $(TARGET) main.o45 music.o45 $(STDLIB)

check: main.c music.o45 $(STDLIB)
	$(CC45) -O2 -DDRUMS_CHECK $(INCLUDES) -c main.c -o check.o45
	$(LN45) -basic -o drumscheck.prg check.o45 music.o45 $(STDLIB)

play: music.txt
	$(TUNE45) play music.txt --loops 4

clean:
	rm -f main.o45 music.o45 check.o45 music.c music.h $(TARGET) drums.map drumscheck.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
