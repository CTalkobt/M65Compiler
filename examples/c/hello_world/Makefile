# Hello World example for cc45 / MEGA65
#
# Usage:
#   make              Build hello.prg
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/stdlib.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = hello.prg

.PHONY: all clean run

all: $(TARGET)

hello.o45: hello.c
	$(CC45) $(INCLUDES) -c hello.c -o hello.o45

$(TARGET): hello.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(CRT0) hello.o45 $(STDLIB)

clean:
	rm -f hello.o45 hello.o45.s $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
