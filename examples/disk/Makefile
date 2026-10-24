# disk — the examples on two D81 disk images with a menu each (disk45, basic45)
#
# Usage:
#   make              Build the examples, then games.d81 and demos.d81
#   make disks        Only the disks (from the examples already built)
#   make clean        Remove the disk images and the menus
#   make run          Start xemu with the games disk in drive 8 and its menu
#   make run-demos    The same with the demos disk
#
# games.txt and demos.txt list each disk's programs: the file, its name on
# the disk and its line in the menu. A disk's menu is menu.bas (basic45
# source) with the disk's title and programs as DATA (<disk>_menu.bas, made
# here); it is saved as AUTOBOOT.C65, which the MEGA65 runs when it starts
# with the disk mounted, and as MENU.

ROOT = ../..
BASIC45 = $(ROOT)/bin/basic45
DISK45 = $(ROOT)/bin/disk45
EX = ..

DISKS = games.d81 demos.d81
LABEL_games = CC45 GAMES
LABEL_demos = CC45 DEMOS
TITLE_games = cc45 games for the mega65
TITLE_demos = cc45 demos for the mega65

# a list's files: its lines' first fields
files = $(addprefix $(EX)/,$(shell grep -v '^\#' $(1) | awk -F'|' 'NF {gsub(/ /, "", $$1); print $$1}'))

.PHONY: all clean run run-demos examples disks

all: examples disks

disks: $(DISKS)

examples:
	$(MAKE) -C $(EX) all

# a listed program not built yet (a fresh tree): built in its directory
$(sort $(call files,games.txt) $(call files,demos.txt)):
	$(MAKE) -C $(dir $@)

# the menu: menu.bas and, at entries, the title and the programs that have a
# line in the menu (name, line)
%_menu.bas: %.txt menu.bas
	{ echo "' $@ - made from $< by the Makefile, do not edit"; \
	  echo '#include "menu.bas"'; \
	  echo; echo 'entries:'; \
	  echo '  data "$(TITLE_$*)"'; \
	  grep -v '^#' $< | awk -F'|' 'NF >= 3 { \
	      n = $$2; d = $$3; gsub(/^ +| +$$/, "", n); gsub(/^ +| +$$/, "", d); \
	      if (d != "") printf "  data \"%s\", \"%s\"\n", n, d }'; \
	  echo '  data "*"'; } > $@

# (kept: make would remove them as intermediate files)
.SECONDARY: games_menu.bas demos_menu.bas games_menu.prg demos_menu.prg

%_menu.prg: %_menu.bas menu.bas
	$(BASIC45) $< -o $@

.SECONDEXPANSION:
%.d81: %_menu.prg %.txt $$(call files,%.txt)
	rm -f $@
	$(DISK45) create $@ -n "$(LABEL_$*)" -i "45"
	$(DISK45) add $@ $*_menu.prg "AUTOBOOT.C65"
	$(DISK45) add $@ $*_menu.prg "MENU"
	@grep -v '^#' $*.txt | while IFS='|' read -r prg name desc; do \
		prg=$$(echo $$prg); name=$$(echo $$name); \
		[ -n "$$prg" ] || continue; \
		echo "$(DISK45) add $@ $(EX)/$$prg \"$$name\""; \
		$(DISK45) add $@ "$(EX)/$$prg" "$$name" >/dev/null || exit 1; \
	done
	$(DISK45) validate $@
	$(DISK45) list $@

clean:
	rm -f $(DISKS) games_menu.bas demos_menu.bas games_menu.prg demos_menu.prg examples.d81 menu.prg

run: games.d81
	xemu-xmega65 -prgmode 65 -8 games.d81 -prg games_menu.prg

run-demos: demos.d81
	xemu-xmega65 -prgmode 65 -8 demos.d81 -prg demos_menu.prg
