# Bug: Issue #46 — Linker Relocation Offset Error for .o45 Objects

## Original Issue
GitHub Issue #46: Programs using `cbm_getin()` with local variables or multiple conditions drop to the MEGA65 monitor when run on real hardware.

## Root Causes Found and Fixed (commits 71fa660 through 6e737db)

### 1. chknonzero/chkzero/select branch displacement (FIXED)
- BNE/BEQ displacement was +3, should be +4
- Misaligned branch landed on byte $02 (CLE opcode on 45GS02)
- CLE enabled 16-bit stack, masking other bugs by allowing stack overflow

### 2. Numerous IR codegen and assembler bugs (FIXED)
- ZP save moved from PHA-based (stack) to BSS buffer
- LDZ base-page encoding (no ZP mode on 45GS02)
- cmp.32/cmp.s32 byte order and branch placeholder address tracking
- Optimizer Y-indexed load elimination after INY
- Struct compound literal initialization
- MemberAccess p->x vs s.x
- Stack push order (right-to-left)
- va_start/va_arg
- zpCall parameter passing
- Constant folder ++/-- preservation
- 16-bit shift loops
- I32 frame operand scratch conflict
- VReg ZP slot reuse
- Quad instruction BASE_PAGE promotion

## Current Remaining Bug: Linker Symbol Offset

### Symptom
User hogboon rebuilt at commit 816c55d. Programs compiled with the separate compilation flow (`cc45 -c` + `ca45 -c` + `ln45`) still drop to the MEGA65 monitor. The standalone flow (`cc45 file.c -o file.prg`) works correctly.

### Reproduction
```bash
# Create if1.c (from issue #46)
cat > if1.c << 'EOF'
#include <cbm.h>
int ch;
int main() {
    while (1) {
        ch = cbm_getin();
        if (ch == 0x41) { cbm_chrout(ch); }
        if (ch == 0x51) { cbm_chrout(ch); }
    }
}
EOF

# Build with separate compilation (FAILS on real MEGA65)
cc45 -c if1.c -o if1.o45
ca45 -c lib/crt0.s -o crt0.o45
ln45 -basic -o if1.prg crt0.o45 if1.o45
```

### Root Cause Analysis

The linker places `_main` at the wrong address. In the linked binary, `jsr _main` jumps 2 bytes INTO a `PHW #$0000` instruction, executing byte $00 (BRK) → monitor drop.

#### .o45 Structure
```
nm45 if1.o45:
    U __sp_base
    00000000 D _ch        ← DATA segment, 2 bytes
    000001f8 T _main      ← TEXT segment

objdump45 -h if1.o45:
    TEXT  size=$0433  VMA=$0002   ← starts at address 2 (after DATA)
    DATA  size=$0002  VMA=$0000   ← 2 bytes at address 0
```

#### The Offset Error

The `_main` export offset in the .o45 is **$01F8**. The O45Emitter computes this as:

```
offset = sym->value - segBase = $01FA - $0002 = $01F8
```

Where `sym->value = $01FA` (assembler's absolute address of _main) and `segBase = $0002` (TEXT segment start address).

The linker then computes:
```
finalAddr = textBase + objOffset + exp.offset = $2025 + 0 + $01F8 = $221D
```

But the actual `PHW` instruction (first instruction of _main) in the linked binary is at **$221B**, not $221D. The 2-byte discrepancy matches the DATA section size.

#### Where the Mismatch Occurs

In the .o45 disassembly, VMA $01F8 shows `PHW #$0000` — this is correct. The objdump displays VMA addresses ($0002-based). The export offset $01F8 is section-relative.

In the linked binary, TEXT from if1.o45 is placed at $2025 (after crt0's 24 bytes). Byte $01F8 from the section body maps to $2025 + $01F8 = $221D.

But the section body is extracted from the full assembler binary starting at VMA $0002. If the section body byte at index 0 corresponds to VMA $0002, then:
- Section body index $01F6 corresponds to VMA $01F8 (where PHW is)
- Section body index $01F8 corresponds to VMA $01FA

So the **export offset $01F8 is the VMA**, not the body index. The linker adds $01F8 to the text base, but it should add $01F6 (the body index).

### Investigation Status

#### Confirmed via Debug Output
Added temporary `fprintf` to `O45Emitter.cpp` at the export computation:
```
DEBUG _main: seg='code' symVal=$01FA segBase=$0002 offset=$01F8
```

The emitter computes `$01FA - $0002 = $01F8`. This matches the VMA, not the body index.

### Resolution (FIXED)

The issue was traced to `O45Emitter::emit()`. Previously, to extract the segment bodies for the `.o45` file, `O45Emitter` generated the entire AST into a single flat `std::vector<uint8_t> fullBinary` using `AssemblerGenerator::generate()`. 
However, `AssemblerGenerator` processes statements segment by segment (in `code`, `data`, `bss` order). Because `M65Emitter::emitByte` simply appended to the vector using `push_back()`, the `code` segment was placed at the very beginning of the vector (index 0). When the `data` segment (e.g., `_ch: .res 2`) was processed later, it appended its bytes to the end of the vector. 

Despite the vector containing `[CODE bytes, DATA bytes]`, `O45Emitter` assumed the vector was a literal memory image mapped from address 0. It extracted the `code` segment starting at index `2` (the actual VMA of the `code` segment). Because the vector was out of order, this resulted in `O45Emitter` mis-slicing the `code` segment by starting 2 bytes too late, shifting all instructions by 2 bytes and leaving garbage (`03 00 03 00` from the relocation padding logic) at the beginning.

**Fix:** Refactored `O45Emitter::emit()` to instantiate a fresh `M65Emitter` for *each* segment and call `AssemblerGenerator::generate` passing `singleSegment` to generate only that segment's body. Relocation entries (`spBaseRelocs_` and `symbolRelocs_`) are now aggregated across all segment emitters. The exported binary is now perfectly aligned and matches the directly compiled standalone flow.

### Files Involved
- `src/main/O45Emitter.cpp` — Segment body extraction and relocation aggregation
- `include/AssemblerGenerator.hpp` — Added `singleSegment` parameter
- `src/main/AssemblerGenerator.cpp` — Scoped generation to specific segment
