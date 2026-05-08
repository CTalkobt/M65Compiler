# Palette fade example for cc45 / MEGA65
#
# Demonstrates C+Assembly calling conventions with VIC-IV palette manipulation.
#
# Usage:
#   make              Build palette_fade.prg
#   make clean        Remove build artifacts
#   make disasm       Generate annotated disassembly listings
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
CA45 = $(ROOT)/bin/ca45
LN45 = $(ROOT)/bin/ln45
OBJDUMP = $(ROOT)/bin/objdump45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TARGET = palette_fade.prg
DISASM_C = palette_fade.asm
DISASM_ASM = palette_fade_asm.asm

.PHONY: all clean run disasm

all: $(TARGET)

palette_fade.o45: palette_fade.c
	$(CC45) -fzpcall $(INCLUDES) -c palette_fade.c -o palette_fade.o45

palette_fade_asm.o45: palette_fade_asm.s
	$(CA45) -c palette_fade_asm.s -o palette_fade_asm.o45

$(TARGET): palette_fade.o45 palette_fade_asm.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $(TARGET) $(CRT0) palette_fade.o45 palette_fade_asm.o45 $(STDLIB)

disasm: palette_fade.o45 palette_fade_asm.o45
	@echo "=== Disassembly of palette_fade.o45 (C) ===" > $(DISASM_C)
	@echo "Symbols:" >> $(DISASM_C)
	$(OBJDUMP) -t palette_fade.o45 >> $(DISASM_C)
	@echo "" >> $(DISASM_C)
	@echo "=== Code ===" >> $(DISASM_C)
	$(OBJDUMP) -d palette_fade.o45 >> $(DISASM_C)
	@echo "" >> $(DISASM_C)
	@echo "=== Disassembly of palette_fade_asm.o45 (Assembly) ===" > $(DISASM_ASM)
	@echo "Symbols:" >> $(DISASM_ASM)
	$(OBJDUMP) -t palette_fade_asm.o45 >> $(DISASM_ASM)
	@echo "" >> $(DISASM_ASM)
	@echo "=== Code ===" >> $(DISASM_ASM)
	$(OBJDUMP) -d palette_fade_asm.o45 >> $(DISASM_ASM)
	@echo "Disassembly written to $(DISASM_C) and $(DISASM_ASM)"

clean:
	rm -f palette_fade.o45 palette_fade.o45.s palette_fade_asm.o45 $(TARGET) $(DISASM_C) $(DISASM_ASM)

run: $(TARGET)
	xemu-xmega65 -prg $(TARGET)
