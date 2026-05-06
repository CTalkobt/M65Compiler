ROOT := ../..
CA45 := $(ROOT)/bin/ca45
CC45 := $(ROOT)/bin/cc45
LN45 := $(ROOT)/bin/ln45
OBJDUMP45 := $(ROOT)/bin/objdump45

PRG := bug_jmp_reloc.prg
OBJS := main_jmp.o45 bug_jmp_reloc.o45

all: $(PRG)

$(PRG): $(OBJS)
	$(LN45) -basic -M map.txt -m -o $@ $(OBJS)

main_jmp.o45: main_jmp.c
	$(CC45) -c $< -o $@

bug_jmp_reloc.o45: bug_jmp_reloc.s
	$(CA45) -c $< -o $@

clean:
	rm -f $(OBJS) $(PRG) map.txt main_jmp.o45.s

disasm:
	$(OBJDUMP45) -d -m map.txt $(PRG)

.PHONY: all clean
