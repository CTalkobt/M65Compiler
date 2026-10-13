# Makefile for cc45 standard library and runtime
#
# Builds:
#   build/c45.lib        — combined CRT + stdlib archive (ZP calling, cc45's default)
#   build/c45_zp.lib     — same archive under its earlier name
#   build/c45_stack.lib  — combined CRT + stdlib archive (stack calling, -fno-zpcall)

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

# Stdlib modules. The C sources (stdlib/) are compiled once per calling
# convention; the hand-written assembly (stdlib_asm/) uses the ZP convention
# and is shared by both libraries — stack-convention callers reach it through
# ln45's stack→ZP bridge thunks. A C file with an assembly version is skipped.
STDLIB_ASM_SOURCES = $(wildcard stdlib_asm/*.s45)
STDLIB_ASM_NAMES = $(basename $(notdir $(STDLIB_ASM_SOURCES)))
STDLIB_C_SOURCES = $(filter-out $(addprefix stdlib/,$(addsuffix .c,$(STDLIB_ASM_NAMES))),$(wildcard stdlib/*.c))
STDLIB_ASM_OBJECTS = $(patsubst stdlib_asm/%.s45,$(BUILD)/asm_%.o45,$(STDLIB_ASM_SOURCES))
STDLIB_OBJECTS = $(patsubst stdlib/%.c,$(BUILD)/%.o45,$(STDLIB_C_SOURCES)) \
                 $(STDLIB_ASM_OBJECTS) $(FLOAT_ROM_OBJECTS)
STDLIB_ZP_OBJECTS = $(patsubst stdlib/%.c,$(BUILD)/zp_%.o45,$(STDLIB_C_SOURCES)) \
                    $(STDLIB_ASM_OBJECTS)

# Library C sources are written as standard C (signed int/long) with unsigned
# char for PETSCII; cc45's own default makes int/long unsigned too.
LIB_CFLAGS = -O2 -fimplicit-function-declaration -fsigned-default -funsigned-char

# Rebuild objects when the tools or library headers change
C_TOOL_DEPS = $(CC45) $(CA45) $(wildcard include/*.h)

.PHONY: all clean

all: $(BUILD)/c45.lib $(BUILD)/c45_zp.lib $(BUILD)/c45_stack.lib

# --- Rules ---

$(BUILD)/%.o45: %.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: src/%.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/asm_%.o45: stdlib_asm/%.s45 $(CA45) | $(BUILD)
	$(CA45) -c $< -o $@

$(BUILD)/%.o45: stdlib/%.c $(C_TOOL_DEPS) | $(BUILD)
	$(CC45) -c -fno-zpcall $(LIB_CFLAGS) $< -o $@

$(BUILD)/zp_%.o45: stdlib/%.c $(C_TOOL_DEPS) | $(BUILD)
	$(CC45) -c -fzpcall $(LIB_CFLAGS) $< -o $@

$(BUILD)/c45_stack.lib: $(CRT_OBJECTS) $(STDLIB_OBJECTS) | $(BUILD)
	rm -f $@
	$(AR45) c $@ $(CRT_OBJECTS) $(STDLIB_OBJECTS)

$(BUILD)/c45.lib: $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS) $(FLOAT_ROM_OBJECTS) | $(BUILD)
	rm -f $@
	$(AR45) c $@ $(CRT_ZP_OBJECTS) $(STDLIB_ZP_OBJECTS) $(FLOAT_ROM_OBJECTS)

$(BUILD)/c45_zp.lib: $(BUILD)/c45.lib
	cp $< $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
