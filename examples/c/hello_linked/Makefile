# hello_linked — Minimal linked C program for cc45 / MEGA65
#
# Shows the standard relocatable build workflow:
#   1. cc45 -c  → compile C to .o45 object
#   2. ln45     → link objects + stdlib into .prg
#
# Usage:
#   make              Build hello.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = hello.prg

.PHONY: all clean run

all: $(TARGET)

main.o45: main.c
	$(CC45) $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
