# game_of_life — Conway's Game of Life for cc45 / MEGA65
#
# Runs on the default 40x25 text screen with wrapping edges.
# Demonstrates 2D arrays, nested loops, and direct screen RAM access.
#
# Usage:
#   make              Build life.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib

TARGET = life.prg
MAP = life.map

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M $(MAP) -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s $(TARGET) $(MAP) $(TARGET).debug.json

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
