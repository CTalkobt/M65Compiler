# 45GS02 Instruction Reference

The instructions `ca45` assembles for the 45GS02, with the encodings from its opcode table (`src/main/AssemblerOpcodeDatabase.cpp`). Simulated (multi-instruction) instructions are summarised at the end and described in [ca45.md](../bin/ca45.md#simulated-instructions).

---

## Addressing Modes

| Mode | Syntax | Operand bytes |
|------|--------|---------------|
| `imp` | `inx` | 0 |
| `A` (accumulator) | `asl`, `asl a`, `inc` | 0 |
| `#imm` | `lda #$12` | 1 |
| `#imm16` (`phw` only) | `phw #$1234` | 2 |
| `bp` (base page) | `lda $12` | 1 |
| `bp,X` / `bp,Y` | `lda $12,x` | 1 |
| `abs` | `lda $1234` | 2 |
| `abs,X` / `abs,Y` | `lda $1234,y` | 2 |
| `(bp,X)` | `lda ($12,x)` | 1 |
| `(bp),Y` | `lda ($12),y` | 1 |
| `(bp),Z` | `lda ($12),z` | 1 |
| `[bp],Z` (32-bit flat pointer) | `lda [$12],z` | 1, preceded by `EOM` (`$EA`) |
| `(bp,SP),Y` (stack-relative indirect) | `lda ($12,sp),y` | 1 |
| `(abs)` / `(abs,X)` | `jmp ($1234)`, `jsr ($1234,x)` | 2 |
| `rel` / `rel16` | `bne label` | 1 / 2 |
| `bp,rel` | `bbr3 $12, label` | 2 (base page, 8-bit offset) |

- **Base page.** `bp` modes address the 256-byte page selected by the B register (`TAB`), not necessarily page 0.
- **Size selection.** For `addr`, `addr,X` and `addr,Y`, ca45 uses the base-page form when the operand value is below `$100`, the instruction has that form, and the operand is not a relocatable (`.bss`, `.extern`) symbol; otherwise it uses the absolute form. `jmp`/`jsr` are always absolute, and their parenthesised operands are always `(abs)`/`(abs,X)`.
- **No base-page form.** When a value below `$100` has to be assembled absolute (`ldz $12`, `ldz $12,x`, `lda $12,y`, `sta $12,y`, `phw $12`, …) ca45 warns `'ldz $12' has no base-page mode: assembled as absolute $0012, which ignores the base page register`.
- **No plain `(bp)`.** The 45GS02 has no unindexed `(bp)` mode; `lda ($12)` is rejected — write `lda ($12),z` with Z = 0.
- **Branches.** `BCC BCS BEQ BNE BMI BPL BVC BVS BRA` are assembled in the 8-bit form when the target is within −128…127 of the next instruction and in the 16-bit form otherwise (re-evaluated until addresses settle). `BSR` only has the 16-bit form. The offset of a 16-bit branch is relative to the instruction address + 2.
- **Instruction size** = 1 opcode byte + operand bytes, + 1 for `[bp],Z` (`EOM`), + 2 for Quad instructions (`$42 $42`).
- **Mode errors.** An unsupported mode is reported with the list of valid ones, e.g. `'asl' does not support addressing mode '#imm'; supported modes: A, zp, zp,X, abs, abs,X`.

---

## Opcode Table

| Mnemonic | Modes and opcodes |
|----------|-------------------|
| `ADC` | #imm `$69`, bp `$65`, bp,X `$75`, abs `$6D`, abs,X `$7D`, abs,Y `$79`, (bp,X) `$61`, (bp),Y `$71`, (bp),Z `$72`, [bp],Z `EA 72` |
| `AND` | #imm `$29`, bp `$25`, bp,X `$35`, abs `$2D`, abs,X `$3D`, abs,Y `$39`, (bp,X) `$21`, (bp),Y `$31`, (bp),Z `$32`, [bp],Z `EA 32` |
| `ASL` | A `$0A`, bp `$06`, bp,X `$16`, abs `$0E`, abs,X `$1E` |
| `ASR` | A `$43`, bp `$44`, bp,X `$54` |
| `ASW` | abs `$CB` |
| `BBR0`–`BBR7` | bp,rel `$0F` `$1F` `$2F` `$3F` `$4F` `$5F` `$6F` `$7F` |
| `BBS0`–`BBS7` | bp,rel `$8F` `$9F` `$AF` `$BF` `$CF` `$DF` `$EF` `$FF` |
| `BCC` | rel `$90`, rel16 `$93` |
| `BCS` | rel `$B0`, rel16 `$B3` |
| `BEQ` | rel `$F0`, rel16 `$F3` |
| `BIT` | #imm `$89`, bp `$24`, bp,X `$34`, abs `$2C`, abs,X `$3C` |
| `BMI` | rel `$30`, rel16 `$33` |
| `BNE` | rel `$D0`, rel16 `$D3` |
| `BPL` | rel `$10`, rel16 `$13` |
| `BRA` | rel `$80`, rel16 `$83` |
| `BRK` | imp `$00` |
| `BSR` | rel16 `$63` |
| `BVC` | rel `$50`, rel16 `$53` |
| `BVS` | rel `$70`, rel16 `$73` |
| `CLC` `CLD` `CLE` `CLI` `CLV` | imp `$18` `$D8` `$02` `$58` `$B8` |
| `CMP` | #imm `$C9`, bp `$C5`, bp,X `$D5`, abs `$CD`, abs,X `$DD`, abs,Y `$D9`, (bp,X) `$C1`, (bp),Y `$D1`, (bp),Z `$D2`, [bp],Z `EA D2` |
| `CPX` | #imm `$E0`, bp `$E4`, abs `$EC` |
| `CPY` | #imm `$C0`, bp `$C4`, abs `$CC` |
| `CPZ` | #imm `$C2`, bp `$D4`, abs `$DC` |
| `DEC` | A `$3A`, bp `$C6`, bp,X `$D6`, abs `$CE`, abs,X `$DE` |
| `DEW` | bp `$C3` |
| `DEX` `DEY` `DEZ` | imp `$CA` `$88` `$3B` |
| `EOM` | imp `$EA` |
| `EOR` | #imm `$49`, bp `$45`, bp,X `$55`, abs `$4D`, abs,X `$5D`, abs,Y `$59`, (bp,X) `$41`, (bp),Y `$51`, (bp),Z `$52`, [bp],Z `EA 52` |
| `INC` | A `$1A`, bp `$E6`, bp,X `$F6`, abs `$EE`, abs,X `$FE` |
| `INW` | bp `$E3` |
| `INX` `INY` `INZ` | imp `$E8` `$C8` `$1B` |
| `JMP` | abs `$4C`, (abs) `$6C`, (abs,X) `$7C` |
| `JSR` | abs `$20`, (abs) `$22`, (abs,X) `$23` |
| `LDA` | #imm `$A9`, bp `$A5`, bp,X `$B5`, abs `$AD`, abs,X `$BD`, abs,Y `$B9`, (bp,X) `$A1`, (bp),Y `$B1`, (bp),Z `$B2`, [bp],Z `EA B2`, (bp,SP),Y `$E2` |
| `LDX` | #imm `$A2`, bp `$A6`, bp,Y `$B6`, abs `$AE`, abs,Y `$BE` |
| `LDY` | #imm `$A0`, bp `$A4`, bp,X `$B4`, abs `$AC`, abs,X `$BC` |
| `LDZ` | #imm `$A3`, abs `$AB`, abs,X `$BB` |
| `LSR` | A `$4A`, bp `$46`, bp,X `$56`, abs `$4E`, abs,X `$5E` |
| `MAP` | imp `$5C` |
| `NEG` | A `$42` |
| `ORA` | #imm `$09`, bp `$05`, bp,X `$15`, abs `$0D`, abs,X `$1D`, abs,Y `$19`, (bp,X) `$01`, (bp),Y `$11`, (bp),Z `$12`, [bp],Z `EA 12` |
| `PHA` `PHP` `PHX` `PHY` `PHZ` | imp `$48` `$08` `$DA` `$5A` `$DB` |
| `PHW` | #imm16 `$F4`, abs `$FC` |
| `PLA` `PLP` `PLX` `PLY` `PLZ` | imp `$68` `$28` `$FA` `$7A` `$FB` |
| `RMB0`–`RMB7` | bp `$07` `$17` `$27` `$37` `$47` `$57` `$67` `$77` |
| `ROL` | A `$2A`, bp `$26`, bp,X `$36`, abs `$2E`, abs,X `$3E` |
| `ROR` | A `$6A`, bp `$66`, bp,X `$76`, abs `$6E`, abs,X `$7E` |
| `ROW` | abs `$EB` |
| `RTI` | imp `$40` |
| `RTS`, `RTN` | imp `$60`, #imm `$62` (`rtn #n`/`rts #n` return and drop `n` bytes; `rtn #0` is assembled as `$60`) |
| `SBC` | #imm `$E9`, bp `$E5`, bp,X `$F5`, abs `$ED`, abs,X `$FD`, abs,Y `$F9`, (bp,X) `$E1`, (bp),Y `$F1`, (bp),Z `$F2`, [bp],Z `EA F2` |
| `SEC` `SED` `SEE` `SEI` | imp `$38` `$F8` `$03` `$78` |
| `SMB0`–`SMB7` | bp `$87` `$97` `$A7` `$B7` `$C7` `$D7` `$E7` `$F7` |
| `STA` | bp `$85`, bp,X `$95`, abs `$8D`, abs,X `$9D`, abs,Y `$99`, (bp,X) `$81`, (bp),Y `$91`, (bp),Z `$92`, [bp],Z `EA 92`, (bp,SP),Y `$82` |
| `STX` | bp `$86`, bp,Y `$96`, abs `$8E`, abs,Y `$9B` |
| `STY` | bp `$84`, bp,X `$94`, abs `$8C`, abs,X `$8B` |
| `STZ` | bp `$64`, bp,X `$74`, abs `$9C`, abs,X `$9E` — stores the **Z register** (not zero) |
| `TAB` `TAX` `TAY` `TAZ` `TBA` | imp `$5B` `$AA` `$A8` `$4B` `$7B` |
| `TRB` | bp `$14`, abs `$1C` |
| `TSB` | bp `$04`, abs `$0C` |
| `TSX` `TSY` `TXA` `TXS` `TYA` `TYS` `TZA` | imp `$BA` `$0B` `$8A` `$9A` `$98` `$2B` `$6B` |

`NOP` is rejected by ca45 (`nop disallowed`) because `$EA` is the `EOM` prefix; write `eom` for that byte, or `clv` (`$B8`) as a one-byte no-op.

### Quad (32-bit Q) instructions

The Q register is A (bits 0–7), X, Y, Z (bits 24–31). ca45 assembles a Quad instruction as `$42 $42` followed by the base instruction:

| Mnemonic | Base | Example |
|----------|------|---------|
| `LDQ`, `STQ` | `LDA`, `STA` (modes bp, abs, (bp),Z, [bp],Z) | `ldq $20` → `42 42 A5 20`; `ldq [$20],z` → `42 42 EA B2 20` |
| `ADCQ` `SBCQ` `ANDQ` `ORAQ` `EORQ` `CMPQ` | `ADC` … `CMP` | `adcq $1234` → `42 42 6D 34 12` |
| `ASLQ` `LSRQ` `ROLQ` `RORQ` | `ASL` … `ROR` | `aslq` → `42 42 0A` |
| `INCQ`, `DECQ` | `INC A`, `DEC A` | `incq` → `42 42 1A` |
| `NEGQ` | `NEG` | `42 42 42` |

ca45 accepts every mode of the base instruction after the prefix; use the forms the 45GS02 defines for Q (base page, absolute, `(bp),Z`/`[bp],Z` indirect, and the accumulator form of the shifts). `INQ`, `DEQ`, `ASRQ`, `BITQ` and `CPQ` are not recognised.

---

## Flags

`N` negative, `V` overflow, `Z` zero, `C` carry, `I` interrupt disable, `D` decimal, `E` emulation (8-bit stack).

| Instructions | Flags changed |
|--------------|---------------|
| `LDA LDX LDY LDZ`, `TAX TAY TAZ TXA TYA TZA TBA TSX TSY`, `PLA PLX PLY PLZ`, `AND ORA EOR`, `INC DEC INX INY INZ DEX DEY DEZ`, `NEG` | N, Z |
| `ADC SBC` | N, V, Z, C |
| `CMP CPX CPY CPZ` | N, Z, C |
| `ASL LSR ROL ROR ASR`, `ASW ROW` | N, Z, C |
| `INW DEW` | N, Z (of the 16-bit result) |
| `BIT` | N, V (bits 7, 6 of memory), Z |
| `TSB TRB` | Z |
| `CLC SEC`, `CLI SEI`, `CLD SED`, `CLV`, `CLE SEE` | the named flag |
| `PLP`, `RTI` | all |
| `STA STX STY STZ STQ`, `TXS TYS TAB`, `PHA PHP PHX PHY PHZ PHW`, `SMBn RMBn`, branches, `JMP JSR BSR RTS RTN`, `MAP EOM` | none |

`MAP` loads the memory mapper from A/X/Y/Z and inhibits interrupts until the next `EOM`. `TAB` sets the base page; `TYS`/`TXS` set the stack pointer high/low byte (`CLE` enables the 16-bit stack). Quad instructions set the flags from the 32-bit result.

---

## Simulated Instructions (summary)

Expanded by ca45 into native code; operand forms, register usage and current limitations are in [ca45.md](../bin/ca45.md#simulated-instructions).

| Group | Instructions |
|-------|--------------|
| 16-bit loads/stores | `ldax lday ldaz ldxy`, `stax stay staz stxy`, `ldw`/`ldw.sp`, `stw`/`stw.sp` |
| Frame access (`offset, sp`, via the frame pointer `$FD/$FE`) | `lda ldx ldy ldz sta stx sty stz offset, sp`, `lda.sp`, `sta.sp`, `inc dec inw dew offset, sp`, `phw.sp`, `ptrstack` |
| Frame/parameter access (compiler) | `lda.fp sta.fp ldax.fp stax.fp lday.fp stay.fp ldaz.fp staz.fp ldaxyz.fp staxyz.fp move.fp`, `inc.16f dec.16f`, `ld*/st*/leax .param` and `.local`, `struct_elem` |
| 16-bit arithmetic/logic | `add.16 sub.16 add.s16 sub.s16`, `and.16 ora.16 eor.16`, `cmp.16`/`cpw`, `cmp.s16`, `neg.16 not.16 neg.s16`, `abs.16 abs.s16`, `lsl.16 lsr.16 asr.16 rol.16 ror.16` (+ `.s16`), `asw`/`row .ax` |
| 32-bit | `add.32 sub.32` (+`.s32`), `and.32 ora.32 eor.32`, `cmp.32 cmp.s32`, `neg.32 not.32 abs.32` (+`.s32`), `lsl.32 lsr.32 asr.32 rol.32 ror.32` (+`.s32`), `sxt.16` |
| Hardware math (`$D768`–`$D77F`) | `mul`/`div` `.8 .16 .24 .32`, `mul.s16 div.s16 mod.16 mod.s16`, `mul.s32 div.s32 mod.32 mod.s32` |
| Bit fields | `bfext bfext16 bfext32`, `bfins bfins16 bfins32` and their `.sp`/`.ind` forms |
| Registers | `swap`, `push`, `pop`, `zero`, `sxt.8`, `chkzero.8/16`, `chknonzero.8/16`, `select`, `ptrderef` |
| Expressions | `expr target, expression` |
| Procedures | `proc`, `endproc`, `call` |
| Not functional at present | `fill`/`fill.sp`, `move`/`move.sp` (DMA), `ldw.f stw.f inc.f dec.f`, `branch.16` |
