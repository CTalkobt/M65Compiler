# Makefile for cc45 standard library and runtime
#
# Builds:
#   build/crt45.lib        — modular C runtime archive (stack calling)
#   build/stdlib45.lib     — standard library archive (stack calling)
#   build/crt45_zp.lib     — modular C runtime archive (ZP calling)
#   build/stdlib45_zp.lib  — standard library archive (ZP calling)

CC45 = ../bin/cc45
CA45 = ../bin/ca45
AR45 = ../bin/ar45
BUILD = build

# CRT0 modules (stack convention)
CRT_SOURCES = crt0.s crt0_mega65.s crt_heap.s crt_bssinit.s
CRT_OBJECTS = $(patsubst %.s,$(BUILD)/%.o45,$(CRT_SOURCES))

# CRT0 modules (ZP convention)
CRT_ZP_SOURCES = crt0_zp.s crt0_mega65_zp.s crt_heap_zp.s crt_bssinit_zp.s
CRT_ZP_OBJECTS = $(patsubst %.s,$(BUILD)/%.o45,$(CRT_ZP_SOURCES))

# Stdlib modules — stack convention (assembly + C)
STDLIB_ASM_SOURCES = $(wildcard stdlib/*.s)
STDLIB_C_SOURCES = $(wildcard stdlib/*.c)
STDLIB_OBJECTS = $(patsubst stdlib/%.s,$(BUILD)/%.o45,$(STDLIB_ASM_SOURCES)) \
                 $(patsubst stdlib/%.c,$(BUILD)/%.o45,$(STDLIB_C_SOURCES))

# Stdlib modules — ZP convention (assembly + C)
STDLIB_ZP_ASM_SOURCES = $(wildcard stdlib_zp/*.s)
STDLIB_ZP_C_SOURCES = $(wildcard stdlib_zp/*.c)
STDLIB_ZP_OBJECTS = $(patsubst stdlib_zp/%.s,$(BUILD)/zp_%.o45,$(STDLIB_ZP_ASM_SOURCES)) \
                    $(patsubst stdlib_zp/%.c,$(BUILD)/zp_%.o45,$(STDLIB_ZP_C_SOURCES))

.PHONY: all clean

all: $(BUILD)/crt45.lib $(BUILD)/stdlib45.lib $(BUILD)/crt45_zp.lib $(BUILD)/stdlib45_zp.lib

# --- Stack convention rules ---

$(BUILD)/%.o45: %.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.c | $(BUILD)
	$(CC45) -c $< -o $@

$(BUILD)/crt45.lib: $(CRT_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_OBJECTS)

$(BUILD)/stdlib45.lib: $(STDLIB_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(STDLIB_OBJECTS)

# --- ZP convention rules ---

$(BUILD)/zp_%.o45: stdlib_zp/%.s | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/zp_%.o45: stdlib_zp/%.c | $(BUILD)
	$(CC45) -c -fzpcall $< -o $@

$(BUILD)/crt45_zp.lib: $(CRT_ZP_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_ZP_OBJECTS)

$(BUILD)/stdlib45_zp.lib: $(STDLIB_ZP_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(STDLIB_ZP_OBJECTS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
