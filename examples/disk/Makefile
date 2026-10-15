# disk — all the examples on one D81 disk image with a menu (disk45, basic45)
#
# Usage:
#   make              Build the examples, then examples.d81
#   make examples.d81 Only the disk (from the examples already built)
#   make clean        Remove the disk image and the menu
#   make run          Start xemu with the disk in drive 8 and the menu
#
# programs.txt lists the programs and their names on the disk; menu.bas
# (basic45 source) is saved as AUTOBOOT.C65, which the MEGA65 runs when it
# starts with the disk mounted, and as MENU.

ROOT = ../..
BASIC45 = $(ROOT)/bin/basic45
DISK45 = $(ROOT)/bin/disk45
EX = ..

DISK = examples.d81
# the programs listed in programs.txt (the disk is rebuilt when one changes)
PRGS := $(addprefix $(EX)/,$(shell grep -v '^\#' programs.txt | awk 'NF {print $$1}'))

.PHONY: all clean run examples

all: examples $(DISK)

examples:
	$(MAKE) -C $(EX) all

menu.prg: menu.bas
	$(BASIC45) menu.bas -o menu.prg

$(DISK): menu.prg programs.txt $(PRGS)
	rm -f $(DISK)
	$(DISK45) create $(DISK) -n "CC45 EXAMPLES" -i "45"
	$(DISK45) add $(DISK) menu.prg "AUTOBOOT.C65"
	$(DISK45) add $(DISK) menu.prg "MENU"
	@grep -v '^#' programs.txt | while read -r prg name; do \
		[ -n "$$prg" ] || continue; \
		echo "$(DISK45) add $(DISK) $(EX)/$$prg \"$$name\""; \
		$(DISK45) add $(DISK) "$(EX)/$$prg" "$$name" >/dev/null || exit 1; \
	done
	$(DISK45) validate $(DISK)
	$(DISK45) list $(DISK)

clean:
	rm -f $(DISK) menu.prg

run: $(DISK)
	xemu-xmega65 -prgmode 65 -8 $(DISK) -prg menu.prg
