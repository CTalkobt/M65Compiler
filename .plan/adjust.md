# Adjustments for Real MEGA65 VHDL Hardware Correctness

This document outlines the detailed steps and code changes required to make the `cc45` compiler suite and its libraries correct according to the official VHDL code of the MEGA65 processor core and peripheral modules.

**Status: ALL ITEMS COMPLETE** — Fixed in PR #267 (commit c6bc3c2b), closes #264.

---

## 1. Math Accelerator: Simulator vs. Hardware Reality ✅ FIXED

**Fixed in:** commit af3849cd (prior to #264)

The compiler suite and standard libraries previously targeted an emulation model from the `mmsim` simulator. Register layout corrected to match `gs4510.vhdl`:

- Dividend/Multiplier A at `$D770-$D773`
- Divisor/Multiplier B at `$D774-$D777`
- Integer quotient at `$D76C-$D76F`
- Fractional quotient at `$D768-$D76B`
- 64-bit product at `$D778-$D77F`
- No hardware remainder register — computed as `dividend - quotient * divisor`
- Division/modulo assembly libraries (div.s45, ldiv.s45, itoa.s45, ltoa.s45) all use correct addresses

### Memory Map Comparison

| Register Address | Emulator (`mmsim`) Assumption | VHDL Hardware Reality (`gs4510.vhdl`) |
| :--- | :--- | :--- |
| **`$D760`–`$D763`** | Dividend (Numerator) input | **Unmapped.** Reads return floating bus values. Writes do nothing. |
| **`$D764`–`$D767`** | Divisor (Denominator) input | **Unmapped.** Reads return floating bus values. Writes do nothing. |
| **`$D768`–`$D76B`** | 32-bit Integer Quotient | **32-bit Fractional Part** of the division result (`div_q[31:0]`). |
| **`$D76C`–`$D76F`** | Reserved / Unused | **32-bit Integer Quotient** of the division result (`div_q[63:32]`). |
| **`$D770`–`$D773`** | 32-bit Remainder (read) / Multiplier A (write) | **Dividend / Multiplier Input A** (`reg_mult_a`/`div_n`). Writing triggers division. |
| **`$D774`–`$D777`** | Multiplier Input B | **Divisor / Multiplier Input B** (`reg_mult_b`/`div_d`). Writing triggers division. |
| **`$D778`–`$D77F`** | 32-bit Product (lower half) | **64-bit Product** of multiplication (`reg_mult_p[63:0]`). |
| **`$D70F`** | Math status (`bit 7` = busy) | Math status (`bit 7` = divider busy, `bit 6` = multiplier busy). |

### Important Hardware Behavior
1. **Division Triggering**: Writing to *any* byte in `$D770`–`$D777` sets the `div_start_over` signal to `'1'`, which initiates/restarts the division.
2. **Modulo / Remainder**: The hardware divider does **not** provide a remainder register. The remainder must be calculated in software:
   $$\text{Remainder} = \text{Dividend} - (\text{Quotient} \times \text{Divisor})$$
   Because the hardware multiplier at `$D770` / `$D774` $\rightarrow$ `$D778$–`$D77F$ is purely combinational in VHDL, this subtraction-based calculation is extremely fast and can be performed with zero cycle overhead on the multiplication.

---

## 2. DMA Controller: Simulator vs. Hardware Reality ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b)

**Changes applied:**
- `include/Mega65Registers.hpp`: Renamed `DMA_CONTROL` → `DMA_ADDR_LSB_TRIG`, `DMA_ADDR_LO` → `DMA_ADDR_MSB`, `DMA_ADDR_MI` → `DMA_ADDR_BANK`, `DMA_ADDR_HI` → `DMA_MODE`. Added `DMA_ADDR_LSB_NOTRIG` ($D70E).
- `src/main/AssemblerSimulatedOps.cpp`: Fixed 3 DMA trigger sequences — bank+MSB first, LSB ($D700) last.
- `src/main/IRBuilder.cpp`: Fixed `__dma_copy` and `__dma_fill` intrinsics — same trigger order.
- `lib/include/mega65.h`: Replaced `struct dma_regs` with correct field names and trigger documentation.

### Memory Map Comparison

| Register Address | Emulator (`mmsim`) Assumption | VHDL Hardware Reality (`gs4510.vhdl`) |
| :--- | :--- | :--- |
| **`$D700`** | DMA Trigger (`control` - write `0` to execute) | **DMA List Address bits 0–7** (`DMA:ADDRLSBTRIG`). Writing triggers DMA. |
| **`$D701`** | DMA list address low byte (`addr_lo`) | **DMA List Address bits 8–15** (`DMA:ADDRMSB`). |
| **`$D702`** | DMA list address mid byte (`addr_mid`) | **DMA List Address Bank bits 16–22** (`DMA:ADDRBANK`). |
| **`$D703`** | DMA list address high byte (`addr_hi`) | **DMA Mode/Control** (`DMA:EN018B` - bit 0 = F018B mode). |
| **`$D704`** | DMA list address bank byte (`addr_bank`) | **DMA List Address Megabyte bits 20–27** (`DMA:ADDRMB`). |
| **`$D705`** | Enhanced trigger (`etrig`) | **Enhanced trigger** (`DMA:ETRIG` - LSB of flat 28-bit address). |
| **`$D70E`** | Unused | **DMA List Address LSB WITHOUT triggering** (`DMA:ADDRLSB`). |

---

## 3. Keyboard Handling: ASCIIKEY, PETSCIIKEY & Modifiers ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b)

**Changes applied:**
- `lib/include/mega65.h`: Added `ASCII_KEY_QUEUE` ($D610), `PETSCII_KEY_QUEUE` ($D619), `KEY_MODIFIERS_REG` ($D611) defines. Added `KEY_MOD_*` bit constants and `getkey_ascii()`/`getkey_petscii()` inline helper functions.

### Keyboard Event Queue Mappings

| Register Address | Name | VHDL Signal / Behavior |
| :--- | :--- | :--- |
| **`$D610`** | `ASCIIKEY` | **Typing Event Queue (ASCII)**. Read returns the ASCII code of the oldest key in the queue. Writing any value pulses `porth_write_strobe` (`ascii_key_next`) to advance the queue. Returns `$00` when empty. |
| **`$D619`** | `PETSCIIKEY` | **Typing Event Queue (PETSCII)**. Read returns the PETSCII code of the oldest key in the queue. Writing any value pulses `porto_write_strobe` (`petscii_key_next`) to advance the queue. Returns `$FF` when empty. |
| **`$D611`** | `KEY_MODIFIERS`| **Modifier Key State** (immediate; read-only for bits 0–6). |

### Keyboard Modifier Bit Allocation (`$D611`)
The modifiers map to the bits of `$D611` as follows:
*   **`Bit 0`**: Left Shift (`MLSHFT`)
*   **`Bit 1`**: Right Shift (`MRSHFT`)
*   **`Bit 2`**: Control (`MCTRL`)
*   **`Bit 3`**: Mega / Commodore (`MMEGA`)
*   **`Bit 4`**: Alternate (`MALT`)
*   **`Bit 5`**: No Scroll (`MSCRL`)
*   **`Bit 6`**: Caps Lock (`MCAPS`)
*   **`Bit 7`**: Disable Modifier key mapping (`MDISABLE` - writeable)

---

## 4. Video Chip (VIC-IV) Extended Registers ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b)

**Changes applied:**
- `lib/include/mega65.h`: Replaced entire `struct vic4_regs` $D040-$D07F section (~40 fields) with correct VHDL `viciv.vhdl` mappings. Fixed $D03E-$D03F from `_reserved1[2]` to named `bitplanes_x_start`/`bitplanes_y_start`.

### Offset Mapping Discrepancy Table (for reference)

| Address Offset | Old Library Header Definition | Correct VHDL Register (`viciv.vhdl`) |
| :--- | :--- | :--- |
| **`$D040`–`$D047`** | `screen_x_lo`/`hi`, `screen_y_lo`/`hi`, `xpos_lo`/`hi`, `raster_hi` | **Unimplemented C65 DACs** (read-only, returns `$FF`). |
| **`$D048`** | `border_left_lo` | **`border_y_top` LSB**: Top border position (low 8 bits). |
| **`$D049`** | `border_left_hi` | **`border_y_top` MSB** & **`sprite_bitplane_enables`** (bits 0–3). |
| **`$D04A`** | `border_right_lo` | **`border_y_bottom` LSB**: Bottom border position (low 8 bits). |
| **`$D04B`** | `border_right_hi` | **`border_y_bottom` MSB** & **`sprite_bitplane_enables`** (bits 4–7). |
| **`$D04C`** | `border_top_lo` | **`x_chargen_start` LSB**: Text/Character generator horizontal offset LSB. |
| **`$D04D`** | `border_top_hi` | **`x_chargen_start` MSB** & **`sprite_horizontal_tile_enables`** (bits 0–3). |
| **`$D04E`** | `border_bot_lo` | **`y_chargen_start` LSB**: Text/Character generator vertical offset LSB. |
| **`$D04F`** | `border_bot_hi` | **`y_chargen_start` MSB** & **`sprite_horizontal_tile_enables`** (bits 4–7). |
| **`$D050`** | `charstep_lo` | **`xcounter_drive` LSB**: Read horizontal raster coordinate LSB. |
| **`$D051`** | `charstep_hi` | **`xcounter_drive` MSB** & **Double Raster flags** (`DBLRR`, `NORRDEL`). |
| **`$D052`** | `chrwidth` | **`ycounter_drive` LSB**: Read physical raster coordinate LSB. |
| **`$D053`** | `chrheight` | **`ycounter_drive` MSB** & **`UPSCALE`**, `BOLDISALT` enabled flags. |
| **`$D054`** | `ctrl_c` | **VIC-IV Control C**: Compositor, FastIO, PAL simulation, Sprite H640, 16bit charset. |
| **`$D055`** | `sprite_extheight_en` | **`sprite_extended_height_enables`**: Extended height enables. |
| **`$D056`** | `sprite_extheight` | **`sprite_extended_height_size`**: Extended height size. |
| **`$D057`** | `sprite_extwidth` | **`sprite_extended_width_enables`**: Extended width enables. |
| **`$D058`** | `screen_addr_lo` | **`virtual_row_width` LSB**: Memory width / line step in bytes LSB. |
| **`$D059`** | `screen_addr_mid` | **`virtual_row_width` MSB**: Memory width / line step in bytes MSB. |
| **`$D05A`** | `screen_addr_hi` | **`chargen_x_scale_drive`**: Character generator X scale factor. |
| **`$D05B`** | `screen_addr_bank` | **`chargen_y_scale`**: Character generator Y scale factor. |
| **`$D05C`** | `colour_addr_lo` | **`single_side_border` LSB**: Left side border position LSB. |
| **`$D05D`** | `colour_addr_hi` | **`single_side_border` MSB** & **`vicii_hot_regs_enable`**, `enable_raster_delay`. |
| **`$D05E`** | `charset_addr_lo` | **`display_row_width` LSB**: Active display row character width LSB. |
| **`$D05F`** | `charset_addr_mid` | **`sprite_h640_msbs`**: Sprite horizontal positions in H640 mode. |
| **`$D060`** | `charset_addr_hi` | **`screen_ram_base` LSB**: Screen RAM base address (bits 0–7). |
| **`$D061`** | `screen_row_lo` | **`screen_ram_base` byte 1**: Screen RAM base address (bits 8–15). |
| **`$D062`** | `screen_row_hi` | **`screen_ram_base` byte 2**: Screen RAM base address (bits 16–23). |
| **`$D063`** | `colour_row_lo` | **`screen_ram_base` MSB** (bits 24–27), **`display_row_width` MSB** (bits 8–9), `fullcolour_mcm`. |
| **`$D064`** | `colour_row_hi` | **`colour_ram_base` LSB**: Color RAM base address (bits 0–7). |
| **`$D065`** | `_reserved3[0]` | **`colour_ram_base` MSB**: Color RAM base address (bits 8–15). |
| **`$D066`** | `_reserved3[1]` | **`xcounter_delay` / test pattern**. |
| **`$D067`** | `_reserved3[2]` | **`sprite_first_x` LSB**: Sprite start position X coordinate. |
| **`$D068`** | `screen_rows` | **`character_set_address` LSB**: Character generator base address (bits 0–7). |
| **`$D069`** | `palette_ctrl` | **`character_set_address` byte 1**: Character generator base address (bits 8–15). |
| **`$D06A`** | `_reserved4[0]` | **`character_set_address` byte 2**: Character generator base address (bits 16–23). |
| **`$D06B`** | `_reserved4[1]` | **`sprite_sixteen_colour_enables`**: Enables 16-color mode for sprites. |
| **`$D06C`** | `sprite_y_msb` | **`vicii_sprite_pointer_address` LSB**: Sprite pointer base address (bits 0–7). |
| **`$D06D`** | `_reserved5[0]` | **`vicii_sprite_pointer_address` byte 1**: Sprite pointer base address (bits 8–15). |
| **`$D06E`** | `_reserved5[1]` | **`vicii_sprite_pointer_address` byte 2**: Sprite pointer base address (bits 16–23). |
| **`$D06F`** | `_reserved5[2]` | **`vicii_first_raster` / NTSC / VGA60 select**. |
| **`$D070`** | `palette_sel` | **Palette Bank Select**: Chargen, Sprite, Alternate palette bank routing. |
| **`$D071`** | `_reserved6[0]` | **`bitplane_sixteen_colour_mode_flags`**: Enables 16-color mode for bitplanes. |
| **`$D072`** | `_reserved6[1]` | **`sprite_y_adjust`**: Sprite Y adjustment offset. |
| **`$D073`** | `_reserved6[2]` | **`reg_alpha_delay`** (bits 0-3) & **`ycounter_scale`** (bits 4-7). |
| **`$D074`** | `sprite_alpha` | **`sprite_alpha_blend_enables`**: Alpha blend enable flags. |
| **`$D075`** | `_reserved7[0]` | **`sprite_alpha_blend_value`**: Global sprite transparency value. |
| **`$D076`** | `_reserved7[1]` | **`sprite_v400s`**: Sprite vertical V400 enable. |
| **`$D077`** | `_reserved7[2]` | **`sprite_v400_msbs`**: Sprite V400 Y position MSBs. |
| **`$D078`** | `dat[4]` (first byte) | **`sprite_v400_super_msbs`** |
| **`$D079`** | `dat[4]` (second byte)| **`vicii_raster_compare` LSB** |
| **`$D07A`** | `dat[4]` (third byte)| **`vicii_raster_compare` MSB & Raster source selection** |
| **`$D07B`** | `dat[4]` (fourth byte)| **`display_row_count`**: Number of text rows to display. |
| **`$D07C`** | `version` | **Bitplane bank select, HSYNC/VSYNC polarity**. |

---

## 5. Audio Mixer & Digital Audio Registers ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b)

**Changes applied:**
- `lib/include/mega65.h`: Removed incorrect `struct audio_mixer_regs` at $D63C (that address is C65 UART space). Added correct defines at $D6F4-$D6FD: `AUDIO_MIX_SEL`, `AUDIO_MIX_DATA`, `AUDIO_DIGI_LEFT/RIGHT`, `AUDIO_READBACK`. Added `set_mixer_coefficient()` helper function.

### Audio Mixer & Digital Audio Mappings

| Register Address | Name | VHDL Signal / Behavior |
| :--- | :--- | :--- |
| **`$D6F4`** | `AUDIO_MIX_SEL` | **Mixer Register Select**. Selects one of the 128 internal volume coefficients. Writing an even address `2 * N` targets the low byte of coefficient `N`; writing an odd address `2 * N + 1` targets the high byte. |
| **`$D6F5`** | `AUDIO_MIX_DATA`| **Mixer Register Data**. Reads/writes the selected coefficient byte. |
| **`$D6F8`–`$D6F9`**| `AUDIO_DIGI_L` | **Digital Audio Left Channel**. 16-bit PCM digital audio input (LSB first). |
| **`$D6FA`–`$D6FB`**| `AUDIO_DIGI_R` | **Digital Audio Right Channel**. 16-bit PCM digital audio input (LSB first). |
| **`$D6FC`–`$D6FD`**| `AUDIO_READBACK`| **Audio Loopback Read-Back**. 16-bit PCM audio output (source channel selected by `$D6F4`, < 16 reads input, < 24 reads mixed output). |

### Mixer Coefficient Layout
The audio mixer takes 16 input sources, applies a 16-bit coefficient to each, and sums them for 8 output channels.

There are $16 \times 8 = 128$ coefficients stored in internal RAM. The coefficient index is:
$$\text{Index} = \text{input\_channel} + (\text{output\_channel} \times 16)$$

#### 16 Input Channels:
*   `0`: Left SID
*   `1`: Right SID
*   `2`–`3`: Communications Modems
*   `4`–`5`: Bluetooth audio (Left/Right)
*   `6`–`7`: Headphones/FM
*   `8`–`9`: PCM Digital Audio (inputs from `$D6F8`–`$D6FB`)
*   `10`–`13`: MEMS microphones
*   `14`–`15`: Headphone Microphone / Master Volume (shadowed)

#### 8 Output Channels:
*   `0`: Left Speaker / HDMI left
*   `1`: Right Speaker / HDMI right
*   `2`–`3`: Modem outputs
*   `4`–`5`: Bluetooth / Internal speaker
*   `6`–`7`: Headphone jack Left / Right

---

## 6. Ethernet Controller Registers ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b)

**Changes applied:**
- `lib/include/mega65.h`: Replaced incorrect `struct eth_regs` with correct VHDL layout: ctrl, irq_ctrl, txsz_lo/hi, cmd, mode, miim_reg, miim_val_lo/hi, mac[6]. Updated control bit defines. Added documentation that RX/TX packet data is at `$FFDE800-$FFDEFFF` (memory-mapped window), not register ports.

### Ethernet Memory Map Comparison

| Register Address | Old Library Mapping (`struct eth_regs`) | VHDL Hardware Reality (`ethernet.vhdl`) |
| :--- | :--- | :--- |
| **`$D6E0`** | `ctrl` | **Control**: Bit 0 = reset, Bit 1 = transmit reset, Bit 7 = transmit idle. |
| **`$D6E1`** | `txszlo` | **Interrupt status/control**: RX/TX interrupt flags and enables, buffer count. |
| **`$D6E2`** | `txszhi` | **TX Frame size LSB**. |
| **`$D6E3`** | Reserved | **TX Frame size MSB**. |
| **`$D6E4`** | Reserved | **Command**: `$01` = trigger transmission. |
| **`$D6E5`** | Reserved | **Mode**: Promiscuous, CRC check, and phase adjustments. |
| **`$D6E6`** | Reserved | **MIIM Register & PHY Select**. |
| **`$D6E7`** | Reserved | **MIIM Register Value (LSB)**. |
| **`$D6E8`** | `rxbuf` | **MIIM Register Value (MSB)**. |
| **`$D6E9`–`$D6EE`** | `txbuf` / `mac` | **MAC Address** (6 bytes). |

---

## 7. Real-Time Clock (RTC) Calendar Index Mismatch ✅ FIXED

**Fixed in:** PR #267 (commit c6bc3c2b) for ZP version; stack version was already correct.

**Changes applied:**
- `lib/stdlib_zp/rtc.c`: Fixed field order to match VHDL — `rtc[3]`=day, `rtc[4]`=month, `rtc[5]`=year, `rtc[6]`=wday. Updated register layout comment.
- `lib/stdlib/rtc.c`: Already had correct order (with debounced reads).

### Correct RTC Layout
The calendar indices are mapped according to VHDL (`mega65r3_i2c.vhdl` / `mega65r4_i2c.vhdl`):
*   `rtc[0]` ($FFD7110) → **`sec` (Seconds)**
*   `rtc[1]` ($FFD7111) → **`min` (Minutes)**
*   `rtc[2]` ($FFD7112) → **`hour` (Hours)**
*   `rtc[3]` ($FFD7113) → **`day` (Day of Month)**
*   `rtc[4]` ($FFD7114) → **`month` (Month)**
*   `rtc[5]` ($FFD7115) → **`year` (Year)**
*   `rtc[6]` ($FFD7116) → **`wday` (Day of Week)**

---

## 8. C Library Header Additions ✅ COMPLETE

All proposed additions from this section have been applied to `lib/include/mega65.h` in PR #267:

- **8.1** Corrected VIC-IV struct ✅
- **8.2** Audio Mixer & PCM defines ✅
- **8.3** Corrected Ethernet struct ✅ (UART struct deferred — not in issue scope)
- **8.4** UUID and ADC registers — deferred (not in issue #264 scope)
- **8.5** Keyboard Event Queue mappings ✅

---

## 9. Compiler C++ Code Changes ✅ COMPLETE

- **9.1** `include/Mega65Registers.hpp` register constants ✅ (math accel in af3849cd, DMA in c6bc3c2b)
- **9.2** `src/main/AssemblerSimulatedOps.cpp` DMA triggers ✅ (c6bc3c2b)
- **9.3** `src/main/IRBuilder.cpp` DMA intrinsics ✅ (c6bc3c2b)

---

## 10. Platform Headers and Assembly Libraries ✅ COMPLETE

- **10.1** RTC library code ✅ (stack version already correct; ZP version fixed in c6bc3c2b)
- **10.2** Division assembly libraries ✅ (already used correct VHDL addresses)
