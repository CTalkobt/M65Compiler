# multi_module — Multi-file C project for cc45 / MEGA65
#
# Demonstrates the standard multi-module workflow:
#   1. Each .c file compiles independently to a .o45 object
#   2. ln45 links all objects + stdlib into a single .prg
#   3. Shared declarations live in util.h
#
# Usage:
#   make              Build program.prg
#   make clean        Remove build artifacts
#   make disasm       Generate annotated disassembly listings
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
OBJDUMP = $(ROOT)/bin/objdump45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = program.prg
SOURCES = main.c vec.c format.c
OBJECTS = $(SOURCES:.c=.o45)

.PHONY: all clean run disasm

all: $(TARGET)

%.o45: %.c util.h
	$(CC45) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJECTS) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(OBJECTS) $(STDLIB)

disasm: $(OBJECTS)
	@for obj in $(OBJECTS); do \
		echo "=== $$obj ===" ; \
		$(OBJDUMP) -d $$obj ; \
		echo ; \
	done

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o45=.o45.s) $(TARGET)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
