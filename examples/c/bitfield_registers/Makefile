# bitfield_registers — Hardware register access via bitfield structs
#
# Demonstrates using C bitfield structs to map MEGA65 VIC-IV
# control registers, sprite enables, and screen attributes.
# Clean alternative to manual bit masking.
#
# Usage:
#   make              Build bitfield_demo.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = bitfield_demo.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s45 $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
