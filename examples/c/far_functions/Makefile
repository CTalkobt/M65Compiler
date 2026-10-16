# far_functions — __far functions: code kept in far memory, mapped in when called
#
# The __far functions of main.c, math.c and text.c go to automatic mapped
# overlays (ln45); their code is written to fardemo.ovl, which the program
# loads into bank 5 at start: the disk image holds both.
#
# Usage:
#   make          Build fardemo.prg, fardemo.ovl and fardemo.d81
#   make clean    Remove build artifacts
#   make run      Run in xemu with the disk in drive 8

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
DISK45 = $(ROOT)/bin/disk45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = fardemo.prg
SOURCES = main.c math.c text.c
OBJECTS = $(SOURCES:.c=.o45)

.PHONY: all clean run

all: fardemo.d81

%.o45: %.c fardemo.h
	$(CC45) -O2 $(INCLUDES) -c $< -o $@

# (no option needed for __far: ln45 maps the overlays; -M: the map file)
$(TARGET): $(OBJECTS) $(STDLIB)
	$(LN45) -basic -M fardemo.map -o $(TARGET) $(OBJECTS) $(STDLIB)

fardemo.d81: $(TARGET)
	rm -f fardemo.d81
	$(DISK45) create fardemo.d81 -n "FAR FUNCTIONS" -i "45"
	$(DISK45) add fardemo.d81 $(TARGET) "FARDEMO"
	$(DISK45) add fardemo.d81 fardemo.ovl "FARDEMO.OVL"

clean:
	rm -f $(OBJECTS) $(TARGET) fardemo.ovl fardemo.map fardemo.d81

run: fardemo.d81
	xemu-xmega65 -prgmode 65 -8 fardemo.d81 -prg $(TARGET)
