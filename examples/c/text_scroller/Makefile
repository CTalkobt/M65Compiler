# Text Scroller example for cc45 / MEGA65
#
# Demonstrates mixed C and assembly:
#   main.c       — scroll logic, string handling, game loop
#   scroll_hw.s  — hardware screen manipulation (byte shift, VSYNC, colour)
#
# Usage:
#   make              Build scroller.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
CA45 = $(ROOT)/bin/ca45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = scroller.prg
OBJS = main.o45 scroll_hw.o45

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) $(INCLUDES) -c main.c -o main.o45

scroll_hw.o45: scroll_hw.s
	$(CA45) -c scroll_hw.s -o scroll_hw.o45

$(TARGET): $(OBJS) $(CRT0) $(STDLIB)
	$(LN45) -m -basic -o $(TARGET) $(CRT0) $(OBJS) $(STDLIB) > map.txt

clean:
	rm -f $(OBJS) main.o45.s $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
