# Makefile for cc45 standard library and runtime
#
# Builds:
#   build/crt45.lib        — modular C runtime archive
#   build/stdlib45.lib     — standard library archive

CA45 = ../bin/ca45
AR45 = ../bin/ar45
BUILD = build

# CRT0 modules
CRT_SOURCES = crt0.s crt0_mega65.s crt_heap.s
CRT_OBJECTS = $(patsubst %.s,$(BUILD)/%.o45,$(CRT_SOURCES))

# Stdlib modules
STDLIB_SOURCES = $(wildcard stdlib/*.s)
STDLIB_OBJECTS = $(patsubst stdlib/%.s,$(BUILD)/%.o45,$(STDLIB_SOURCES))

.PHONY: all clean

all: $(BUILD)/crt45.lib $(BUILD)/stdlib45.lib

$(BUILD)/%.o45: %.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/crt45.lib: $(CRT_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_OBJECTS)

$(BUILD)/stdlib45.lib: $(STDLIB_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(STDLIB_OBJECTS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
