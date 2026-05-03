# Unit converter example for cc45 / MEGA65
#
# Demonstrates long type, sprintf, sscanf, puts.
#
# Usage:
#   make              Build convert.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/stdlib45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = convert.prg

.PHONY: all clean run

all: $(TARGET)

convert.o45: convert.c
	$(CC45) -O0 $(INCLUDES) -c convert.c -o convert.o45

$(TARGET): convert.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(CRT0) convert.o45 $(STDLIB)

clean:
	rm -f convert.o45 convert.o45.s $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
