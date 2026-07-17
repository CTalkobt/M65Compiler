# Makefile for cc45 standard library and runtime
#
# Builds:
#   build/c45.lib      — combined CRT + stdlib archive (stack calling)
#   build/c45_zp.lib   — combined CRT + stdlib archive (ZP calling)

CC45 = ../bin/cc45
CA45 = ../bin/ca45
AR45 = ../bin/ar45
BUILD = build

# CRT0 modules (stack convention)
CRT_SOURCES = crt0.s45 crt0_mega65.s45 crt_heap.s45 crt_bssinit.s45
CRT_OBJECTS = $(patsubst %.s45,$(BUILD)/%.o45,$(CRT_SOURCES))

# CRT0 modules (ZP convention)
CRT_ZP_SOURCES = crt0_zp.s45 crt0_mega65_zp.s45 crt_heap_zp.s45 crt_bssinit_zp.s45
CRT_ZP_OBJECTS = $(patsubst %.s45,$(BUILD)/%.o45,$(CRT_ZP_SOURCES))

# Stdlib modules — stack convention (assembly + C)
STDLIB_ASM_SOURCES = $(wildcard stdlib/*.s45)
STDLIB_C_SOURCES = $(wildcard stdlib/*.c)
STDLIB_OBJECTS = $(patsubst stdlib/%.s45,$(BUILD)/%.o45,$(STDLIB_ASM_SOURCES)) \
                 $(patsubst stdlib/%.c,$(BUILD)/%.o45,$(STDLIB_C_SOURCES))

# Stdlib modules — ZP convention (assembly + C)
STDLIB_ZP_ASM_SOURCES = $(wildcard stdlib_zp/*.s45)
STDLIB_ZP_C_SOURCES = $(wildcard stdlib_zp/*.c)
STDLIB_ZP_OBJECTS = $(patsubst stdlib_zp/%.s45,$(BUILD)/zp_%.o45,$(STDLIB_ZP_ASM_SOURCES)) \
                    $(patsubst stdlib_zp/%.c,$(BUILD)/zp_%.o45,$(STDLIB_ZP_C_SOURCES))

.PHONY: all clean

all: $(BUILD)/c45.lib $(BUILD)/c45_zp.lib

# --- Stack convention rules ---

$(BUILD)/%.o45: %.s45 | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.s45 | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.c | $(BUILD)
	$(CC45) -c $< -o $@

$(BUILD)/c45.lib: $(CRT_OBJECTS) $(STDLIB_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_OBJECTS) $(STDLIB_OBJECTS)

# --- ZP convention rules ---

$(BUILD)/zp_%.o45: stdlib_zp/%.s45 | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/zp_%.o45: stdlib_zp/%.c | $(BUILD)
	$(CC45) -c -fzpcall $< -o $@

$(BUILD)/c45_zp.lib: $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
