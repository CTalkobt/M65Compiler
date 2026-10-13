# BASIC 65 calling C: basic45 --link
#
# Usage:
#   make              Build menu.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
BASIC45 = $(ROOT)/bin/basic45

TARGET = menu.prg

.PHONY: all clean run

all: $(TARGET)

$(TARGET): menu.bas routines.c
	$(BASIC45) --link routines.c menu.bas -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	$(BASIC45) --link routines.c menu.bas -o $(TARGET) --emulator
