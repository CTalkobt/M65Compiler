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
CRT_SOURCES = crt0.s45 crt0_mega65.s45 crt_heap.s45 crt_bssinit.s45 sac_debug.s45
CRT_OBJECTS = $(patsubst %.s45,$(BUILD)/%.o45,$(CRT_SOURCES))

# CRT0 modules (ZP convention)
CRT_ZP_SOURCES = crt0_zp.s45 crt0_mega65_zp.s45 crt_heap_zp.s45 crt_bssinit_zp.s45 sac_debug.s45
CRT_ZP_OBJECTS = $(patsubst %.s45,$(BUILD)/%.o45,$(CRT_ZP_SOURCES))

# Float ROM runtime (shared between stack and ZP conventions)
# Only include float_rom.s45 (direct ZP access — faster).
# float_rom_indirect.s45 is an alternative that uses MOVFM/MOVMF (not needed).
FLOAT_ROM_SOURCES = src/float_rom.s45
FLOAT_ROM_OBJECTS = $(patsubst src/%.s45,$(BUILD)/%.o45,$(FLOAT_ROM_SOURCES))

# Stdlib modules — stack convention (assembly + C)
STDLIB_ASM_SOURCES = $(wildcard stdlib/*.s45)
STDLIB_C_SOURCES = $(wildcard stdlib/*.c)
STDLIB_OBJECTS = $(patsubst stdlib/%.s45,$(BUILD)/%.o45,$(STDLIB_ASM_SOURCES)) \
                 $(patsubst stdlib/%.c,$(BUILD)/%.o45,$(STDLIB_C_SOURCES)) \
                 $(FLOAT_ROM_OBJECTS)

# Stdlib modules — ZP convention (assembly + C)
STDLIB_ZP_ASM_SOURCES = $(wildcard stdlib_zp/*.s45)
STDLIB_ZP_C_SOURCES = $(wildcard stdlib_zp/*.c)
STDLIB_ZP_OBJECTS = $(patsubst stdlib_zp/%.s45,$(BUILD)/zp_%.o45,$(STDLIB_ZP_ASM_SOURCES)) \
                    $(patsubst stdlib_zp/%.c,$(BUILD)/zp_%.o45,$(STDLIB_ZP_C_SOURCES))

# Library C sources are written as standard C (signed int/long) with unsigned
# char for PETSCII; cc45's own default makes int/long unsigned too.
LIB_CFLAGS = -fimplicit-function-declaration -fsigned-default -funsigned-char

# Rebuild objects when the tools or library headers change
C_TOOL_DEPS = $(CC45) $(CA45) $(wildcard include/*.h)

.PHONY: all clean

all: $(BUILD)/c45.lib $(BUILD)/c45_zp.lib

# --- Stack convention rules ---

$(BUILD)/%.o45: %.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: src/%.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.c $(C_TOOL_DEPS) | $(BUILD)
	$(CC45) -c $(LIB_CFLAGS) $< -o $@

$(BUILD)/c45.lib: $(CRT_OBJECTS) $(STDLIB_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_OBJECTS) $(STDLIB_OBJECTS)

# --- ZP convention rules ---

$(BUILD)/zp_%.o45: stdlib_zp/%.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/zp_%.o45: stdlib_zp/%.c $(C_TOOL_DEPS) | $(BUILD)
	$(CC45) -c -fzpcall $(LIB_CFLAGS) $< -o $@

$(BUILD)/c45_zp.lib: $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS) $(FLOAT_ROM_OBJECTS) | $(BUILD)
	$(AR45) c $@ $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS) $(FLOAT_ROM_OBJECTS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
