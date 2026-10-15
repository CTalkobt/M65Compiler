# keyboard_viewer — the MEGA65 keyboard on screen; held keys light up
#
# Usage:
#   make              Build keyboard_viewer.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (windowed: press keys)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = keyboard_viewer.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M keyboard_viewer.map -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 $(TARGET) keyboard_viewer.map

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
