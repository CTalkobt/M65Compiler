ROOT := ../..
CA45 := $(ROOT)/bin/ca45
CC45 := $(ROOT)/bin/cc45
LN45 := $(ROOT)/bin/ln45
NM45 := $(ROOT)/bin/nm45
OBJDUMP45 := $(ROOT)/bin/objdump45

PRG := bug_move_attic.prg
OBJS := main.o45 bug_move_attic.o45

all: $(PRG)

$(PRG): $(OBJS)
	$(LN45) -basic -M map.txt -m -o $@ $(OBJS)

main.o45: main.c
	$(CC45) -c $< -o $@

bug_move_attic.o45: bug_move_attic.s
	$(CA45) -c $< -o $@

inspect: bug_move_attic.o45
	@echo "=== Object symbols ==="
	$(NM45) -a $<
	@echo ""
	@echo "=== Object relocations ==="
	$(NM45) -r $<

disasm: $(PRG)
	$(OBJDUMP45) -d -m map.txt $(PRG)

clean:
	rm -f $(OBJS) $(PRG) map.txt main.o45.s

.PHONY: all clean inspect disasm
