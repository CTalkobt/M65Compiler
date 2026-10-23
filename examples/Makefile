# Build all examples
#
# Usage:
#   make              Build all examples
#   make clean        Clean all examples

SUBDIRS = \
	c/hello_world \
	c/hello_linked \
	c/multi_module \
	c/palette_fade \
	c/unit_convert \
	c/bitfield_registers \
	c/game_of_life \
	c/memset_screen \
	c/text_scroller \
	c/vic4_colors \
	c/far_scroller \
	c/invaders \
	c/mandelbrot \
	c/musicgen \
	c/calc_graph \
	c/forth \
	c/rtc_clock \
	c/keyboard_viewer \
	c/sample_player \
	c/rogue \
	c/hanger \
	c/fcm_graphics \
	c/tunes \
	c/overlays \
	c/mapped_overlays \
	c/far_functions \
	c/stdlib/string_h \
	c/stdlib/stdlib_h \
	c/stdlib/ctype_h \
	basic/c_routines


.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	@echo "--- Building $@ ---"
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do \
		echo "--- Cleaning $$dir ---"; \
		$(MAKE) -C $$dir clean; \
	done
