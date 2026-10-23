# fcm_graphics — full-colour characters: a background per character and a
# palette-cycled plasma
#
# Usage:
#   make              Build fcm_graphics.prg
#   make check        Build fcmcheck.prg (runs 100 frames, then reports)
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = fcm_graphics.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M fcm_graphics.map -o $(TARGET) main.o45 $(STDLIB)

check: main.c $(STDLIB)
	$(CC45) -O2 -DFCM_CHECK $(INCLUDES) main.c -o fcmcheck.prg

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) fcm_graphics.map fcmcheck.prg

run: $(TARGET)
	$(XEMU) -prgmode 65 -prg $(TARGET)
