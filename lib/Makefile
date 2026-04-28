# Makefile for cc45 standard library and runtime
#
# Builds:
#   build/crt0.o45         — default C runtime startup
#   build/crt0_mega65.o45  — MEGA65 platform init (optional)
#   build/stdlib.lib       — standard library archive

CA45 = ../bin/ca45
AR45 = ../bin/ar45
BUILD = build

# CRT0 variants
CRT0_SOURCES = crt0.s crt0_mega65.s
CRT0_OBJECTS = $(patsubst %.s,$(BUILD)/%.o45,$(CRT0_SOURCES))

# Stdlib modules
STDLIB_SOURCES = $(wildcard stdlib/*.s)
STDLIB_OBJECTS = $(patsubst stdlib/%.s,$(BUILD)/%.o45,$(STDLIB_SOURCES))

.PHONY: all clean

all: $(CRT0_OBJECTS) $(BUILD)/stdlib.lib

$(BUILD)/%.o45: %.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/stdlib.lib: $(STDLIB_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(STDLIB_OBJECTS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
