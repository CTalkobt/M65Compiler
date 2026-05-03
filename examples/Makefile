# Build all examples
#
# Usage:
#   make              Build all examples
#   make clean        Clean all examples

SUBDIRS = \
	c/hello_world \
	c/unit_convert \
	c/stdlib/string_h \
	c/stdlib/stdlib_h \
	c/stdlib/ctype_h

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
