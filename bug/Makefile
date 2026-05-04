SUBDIRS := $(sort $(dir $(wildcard */Makefile)))

all:
	@for d in $(SUBDIRS); do echo "=== Building $$d ===" && $(MAKE) -C $$d || exit 1; done

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

.PHONY: all clean
