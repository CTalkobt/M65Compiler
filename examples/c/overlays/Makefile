# overlays — code overlays: a program larger than its memory
#
# Each overlay is a file with #pragma cc45 overlay(name). ln45 links them to
# run in one region and writes their code to ovldemo.ovl, which the program
# loads into far memory at start: the disk image holds both.
#
# Usage:
#   make              Build ovldemo.prg, ovldemo.ovl and ovldemo.d81
#   make OVERLAY_MAP=1  The same with mapped overlays (ln45 --overlay-map:
#                     the window is mapped to each overlay, not copied)
#   make clean        Remove build artifacts
#   make run          Run in xemu with the disk in drive 8

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
DISK45 = $(ROOT)/bin/disk45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = ovldemo.prg
SOURCES = main.c primes.c hanoi.c words.c
OBJECTS = $(SOURCES:.c=.o45)

.PHONY: all clean run

all: ovldemo.d81

%.o45: %.c demo.h
	$(CC45) -O2 $(INCLUDES) -c $< -o $@

# (ln45 writes ovldemo.ovl next to the program; the program loads it as
# OVLDEMO.OVL from the drive it was loaded from)
LNFLAGS = $(if $(OVERLAY_MAP),--overlay-map)

$(TARGET): $(OBJECTS) $(STDLIB)
	$(LN45) -basic $(LNFLAGS) -M ovldemo.map -o $(TARGET) $(OBJECTS) $(STDLIB)

ovldemo.d81: $(TARGET)
	rm -f ovldemo.d81
	$(DISK45) create ovldemo.d81 -n "OVERLAYS" -i "45"
	$(DISK45) add ovldemo.d81 $(TARGET) "OVLDEMO"
	$(DISK45) add ovldemo.d81 ovldemo.ovl "OVLDEMO.OVL"

clean:
	rm -f $(OBJECTS) $(TARGET) ovldemo.ovl ovldemo.map ovldemo.d81

run: ovldemo.d81
	xemu-xmega65 -prgmode 65 -8 ovldemo.d81 -prg $(TARGET)
