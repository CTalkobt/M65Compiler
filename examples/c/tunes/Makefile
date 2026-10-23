# tunes — the pfest game's music, played by <audio_tune.h> from a raster interrupt
#
# Usage:
#   make              Build tunes.prg (tune45 convert makes music.c of music.txt)
#   make check        Build tunescheck.prg (plays from the interrupt, reports)
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

TARGET = tunes.prg

.PHONY: all clean run check play

all: $(TARGET)

music.c music.h: music.txt $(TUNE45)
	$(TUNE45) convert music.txt -o music.c

main.o45: main.c music.h
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

music.o45: music.c music.h
	$(CC45) -O2 $(INCLUDES) -c music.c -o music.o45

$(TARGET): main.o45 music.o45 $(STDLIB)
	$(LN45) -basic -M tunes.map -o $(TARGET) main.o45 music.o45 $(STDLIB)

check: main.c music.o45 $(STDLIB)
	$(CC45) -O2 -DTUNES_CHECK $(INCLUDES) -c main.c -o check.o45
	$(LN45) -basic -o tunescheck.prg check.o45 music.o45 $(STDLIB)

play: music.txt
	$(TUNE45) play music.txt

clean:
	rm -f main.o45 music.o45 check.o45 main.o45.s45 music.c music.h $(TARGET) tunes.map tunescheck.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
