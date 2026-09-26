ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib

TARGET = memset_screen.prg

.PHONY: all clean run

all: $(TARGET)

$(STDLIB):
	$(MAKE) -C $(ROOT)/lib

main.o45: main.c
	$(CC45) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M memset_screen.map -o $(TARGET) main.o45 $(STDLIB)

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) memset_screen.map

run: $(TARGET)
	xemu-xmega65 -prgexit $(TARGET)

inspect: $(TARGET)
	../../../bin/objdump45 -d *.prg

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
