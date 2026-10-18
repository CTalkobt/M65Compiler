# mapped_overlays — mapped code overlays (ln45 --overlay-map), shown at work
#
# red.c, green.c and blue.c are overlays (#pragma cc45 overlay(name)).
# ln45 --overlay-map links them to run in one window and writes them to
# mapdemo.ov1; the program loads that into bank 5 at start, and each call
# into an overlay maps the window to it. main.c checks it really is mapped.
#
# Usage:
#   make          Build mapdemo.prg, mapdemo.ov1 and mapdemo.d81
#   make clean    Remove build artifacts
#   make run      Run in xemu with the disk in drive 8

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
DISK45 = $(ROOT)/bin/disk45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = mapdemo.prg
SOURCES = main.c red.c green.c blue.c
OBJECTS = $(SOURCES:.c=.o45)

.PHONY: all clean run

all: mapdemo.d81

%.o45: %.c mapdemo.h
	$(CC45) -O2 $(INCLUDES) -c $< -o $@

# --overlay-map: map the window to each overlay instead of copying it in;
# -M: the map file
$(TARGET): $(OBJECTS) $(STDLIB)
	$(LN45) -basic --overlay-map -M mapdemo.map -o $(TARGET) $(OBJECTS) $(STDLIB)

mapdemo.d81: $(TARGET)
	rm -f mapdemo.d81
	$(DISK45) create mapdemo.d81 -n "MAPPED OVERLAYS" -i "45"
	$(DISK45) add mapdemo.d81 $(TARGET) "MAPDEMO"
	$(DISK45) add mapdemo.d81 mapdemo.ov1 "MAPDEMO.OV1"

clean:
	rm -f $(OBJECTS) $(TARGET) mapdemo.ov1 mapdemo.ovd mapdemo.map mapdemo.d81

run: mapdemo.d81
	xemu-xmega65 -prgmode 65 -8 mapdemo.d81 -prg $(TARGET)
