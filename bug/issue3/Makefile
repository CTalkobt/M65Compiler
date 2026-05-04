ROOT := ../..
CA45 := $(ROOT)/bin/ca45
CC45 := $(ROOT)/bin/cc45
LN45 := $(ROOT)/bin/ln45
OBJDUMP45 := $(ROOT)/bin/objdump45

PRG := bug_open.prg
OBJS := main.o45 bug_open.o45

all: $(PRG)

$(PRG): $(OBJS)
	$(LN45) -basic -M map.txt -m -o $@ $(OBJS)

main.o45: main.c
	$(CC45) -c $< -o $@

bug_open.o45: bug_open.s
	$(CA45) -c $< -o $@

clean:
	rm -f $(OBJS) $(PRG) map.txt main.o45.s

disasm:
	$(OBJDUMP45) -d $(PRG)

.PHONY: all clean
