# ctype.h demo for cc45 / MEGA65
#
# Usage:
#   make              Build ctype_demo.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/stdlib45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = ctype_demo.prg

.PHONY: all clean run

all: $(TARGET)

ctype_demo.o45: ctype_demo.c
	$(CC45) $(INCLUDES) -c ctype_demo.c -o ctype_demo.o45

$(TARGET): ctype_demo.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(CRT0) ctype_demo.o45 $(STDLIB)

clean:
	rm -f ctype_demo.o45 ctype_demo.o45.s $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
