# Adjustments for Real MEGA65 VHDL Hardware Correctness

This document outlines the detailed steps and code changes required to make the `cc45` compiler suite and its libraries correct according to the official VHDL code of the MEGA65 processor core and peripheral modules.

---

## 1. Math Accelerator: Simulator vs. Hardware Reality

The compiler suite and standard libraries currently target an emulation model from the `mmsim` simulator. This model makes assumptions about register layouts and modulo outputs that do not exist or behave differently on the actual MEGA65 FPGA hardware core.

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

## 2. DMA Controller: Simulator vs. Hardware Reality

The DMA controller trigger mechanism is also mapped incorrectly in the current compiler and libraries. The emulator assumed a standard linear 32-bit address mapping for the DMA list pointer, which conflicts with the actual hardware layout.

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

### Core Bug in Current Code
Because the compiler writes the address LSB to `$D701` (`DMA_ADDR_LO`), the MSB to `$D702` (`DMA_ADDR_MI`), and then writes `0` to `$D700` (`DMA_CONTROL`) to trigger:
*   The DMA is triggered with LSB = `$00`.
*   The MSB is set to the LSB of the address.
*   The Bank is set to the MSB of the address.
This shifts the entire DMA list address left by 8 bits and selects the wrong memory bank, causing list-fetch corruption on physical hardware.

---

## 3. Keyboard Handling: ASCIIKEY, PETSCIIKEY & Modifiers

The MEGA65 keyboard controller implements a typing event queue accessible directly via hardware I/O registers in the `$D600` range (implemented in `c65uart.vhdl` and `iomapper.vhdl`). Neither the compiler nor the libraries currently expose or utilize these registers.

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

## 4. Video Chip (VIC-IV) Extended Registers

The compiler platform header [lib/include/mega65.h](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/include/mega65.h) defines the structure `struct vic4_regs` for extended registers (`$D040`–`$D07F`) incorrectly. The struct layout contains shifted addresses, wrong names, and targets registers that do not exist or perform completely different functions in `viciv.vhdl`.

### Offset Mapping Discrepancy Table

| Address Offset | Library Header Definition (`struct vic4_regs`) | Actual Hardware VHDL Register (`viciv.vhdl`) |
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

## 5. Audio Mixer & Digital Audio Registers

The C header [lib/include/mega65.h](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/include/mega65.h) defines a struct `struct audio_mixer_regs` mapped to `$D63C`–`$D63F`. This mapping is completely incorrect. 

On real hardware, `$D600`–`$D63F` is **reserved for C65 serial UART emulation**. The real audio mixer and digital audio registers are mapped to `$D6F4`–`$D6FD` (implemented in `sdcardio.vhdl`, `audio_complex.vhdl`, and `audio_mixer.vhdl`).

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

## 6. Ethernet Controller Registers

The compiler standard library defines `struct eth_regs` incorrectly. The structure is offset-shifted and misses critical control flags. Most importantly, it incorrectly places `rxbuf` and `txbuf` at `$D6E8`/`$D6E9` (which actually map to MIIM and MAC address registers). 

On real hardware, packets are read/written through a 2KB memory-mapped page window at `$FFDE800`–`$FFDEFFF` (read-only for RX, write-only for TX).

### Ethernet Memory Map Comparison

| Register Address | Incorrect Library Mapping (`struct eth_regs`) | VHDL Hardware Reality (`ethernet.vhdl`) |
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

## 7. Real-Time Clock (RTC) Calendar Index Mismatch

A critical alignment bug exists in the calendar-reading library functions in [lib/stdlib/rtc.c](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/rtc.c) and [lib/stdlib_zp/rtc.c](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/rtc.c). 

The C library code reads calendar fields sequentially from `$FFD7110` but applies the wrong offsets compared to the real VHDL mapping (`mega65r3_i2c.vhdl` / `mega65r4_i2c.vhdl`), scrambling the date fields:

```c
    /* Current Incorrect Reading Layout in rtc.c */
    unsigned char sec = rtc[0];   /* $FFD7110 -> Correct */
    unsigned char min = rtc[1];   /* $FFD7111 -> Correct */
    unsigned char hour = rtc[2];  /* $FFD7112 -> Correct */
    unsigned char wday = rtc[3];  /* $FFD7113 -> WRONG: reads Day of Month */
    unsigned char day = rtc[4];   /* $FFD7114 -> WRONG: reads Month */
    unsigned char month = rtc[5]; /* $FFD7115 -> WRONG: reads Year */
    unsigned char year = rtc[6];  /* $FFD7116 -> WRONG: reads Day of Week */
```

### Correct RTC Layout
The calendar indices must be mapped according to VHDL:
*   `rtc[3]` ($FFD7113) $\rightarrow$ **`day` (Day of Month)**
*   `rtc[4]` ($FFD7114) $\rightarrow$ **`month` (Month)**
*   `rtc[5]` ($FFD7115) $\rightarrow$ **`year` (Year)**
*   `rtc[6]` ($FFD7116) $\rightarrow$ **`wday` (Day of Week)**

---

## 8. Proposed C Library Header Additions

Add the following structures and defines to [lib/include/mega65.h](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/include/mega65.h) to expose these missing/corrected peripherals.

### 8.1. Corrected VIC-IV Struct
```c
struct vic4_regs {
    /* --- VIC-II compatible ($D000-$D02E) --- */
    struct vic4_sprite_pos sprite[8]; /* $D000: sprite positions */
    unsigned char sprite_x_msb;    /* $D010: sprite X position bit 8 */
    unsigned char ctrl1;           /* $D011: RST8|ECM|BMM|DEN|RSEL|YSCROLL */
    unsigned char raster;          /* $D012: raster counter (low 8 bits) */
    unsigned char lightpen_x;      /* $D013: light pen X */
    unsigned char lightpen_y;      /* $D014: light pen Y */
    unsigned char sprite_enable;   /* $D015: sprite enable bits */
    unsigned char ctrl2;           /* $D016: RES|MCM|CSEL|XSCL */
    unsigned char sprite_y_expand; /* $D017: sprite Y expansion */
    unsigned char memptr;          /* $D018: screen/char memory pointers */
    unsigned char irq_status;      /* $D019: interrupt status (write to ack) */
    unsigned char irq_enable;      /* $D01A: interrupt enable mask */
    unsigned char sprite_priority; /* $D01B: sprite-to-bg priority */
    unsigned char sprite_multicolor; /* $D01C: sprite multicolor enable */
    unsigned char sprite_x_expand; /* $D01D: sprite X expansion */
    unsigned char sprite_coll;     /* $D01E: sprite-sprite collision */
    unsigned char sprite_bg_coll;  /* $D01F: sprite-bg collision */
    unsigned char border;          /* $D020: border colour */
    unsigned char bg0;             /* $D021: background colour 0 */
    unsigned char bg1;             /* $D022: background colour 1 */
    unsigned char bg2;             /* $D023: background colour 2 */
    unsigned char bg3;             /* $D024: background colour 3 */
    unsigned char sprite_mcol0;    /* $D025: sprite multicolour 0 */
    unsigned char sprite_mcol1;    /* $D026: sprite multicolour 1 */
    unsigned char sprite_color[8]; /* $D027: sprite colours 0-7 */

    /* --- VIC-III registers ($D02F-$D03F) --- */
    unsigned char key;             /* $D02F: VIC-III key */
    unsigned char ctrl_a;          /* $D030: VIC-III control A */
    unsigned char ctrl_b;          /* $D031: VIC-III/IV control B */
    unsigned char bitplane_enable; /* $D032: bitplane enable */
    unsigned char bitplane_addr[8]; /* $D033: bitplane 0-7 addresses */
    unsigned char bitplane_comp;   /* $D03B: bitplane companion bits */
    unsigned char bitplane_x;      /* $D03C: bitplane X offset */
    unsigned char bitplane_y;      /* $D03D: bitplane Y offset */
    unsigned char bitplanes_x_start; /* $D03E: bitplanes X start screen offset */
    unsigned char bitplanes_y_start; /* $D03F: bitplanes Y start screen offset */

    /* --- VIC-IV extended registers ($D040-$D07F) --- */
    unsigned char c65_dac[8];      /* $D040-$D047: Unimplemented C65 DACs */
    unsigned char border_y_top_lo; /* $D048: top border LSB */
    unsigned char border_y_top_hi; /* $D049: top border MSB & sprite BP enables */
    unsigned char border_y_bot_lo; /* $D04A: bottom border LSB */
    unsigned char border_y_bot_hi; /* $D04B: bottom border MSB & sprite BP enables */
    unsigned char x_chargen_lo;    /* $D04C: char generator X offset LSB */
    unsigned char x_chargen_hi;    /* $D04D: char generator X offset MSB & sprite tile enables */
    unsigned char y_chargen_lo;    /* $D04E: char generator Y offset LSB */
    unsigned char y_chargen_hi;    /* $D04F: char generator Y offset MSB & sprite tile enables */
    unsigned char xpos_lo;         /* $D050: physical scanline X position LSB */
    unsigned char xpos_hi;         /* $D051: physical scanline X position MSB & Double Raster */
    unsigned char ypos_lo;         /* $D052: physical scanline Y position LSB */
    unsigned char ypos_hi;         /* $D053: physical scanline Y position MSB & upscaler */
    unsigned char ctrl_c;          /* $D054: VIC-IV Mode Control C */
    unsigned char sprite_extheight_en; /* $D055: sprite extended height enables */
    unsigned char sprite_extheight_sz; /* $D056: sprite extended height size */
    unsigned char sprite_extwidth_en;  /* $D057: sprite extended width enables */
    unsigned char virtual_row_width_lo; /* $D058: memory width/row step low */
    unsigned char virtual_row_width_hi; /* $D059: memory width/row step high */
    unsigned char chargen_x_scale; /* $D05A: character generator X scaling */
    unsigned char chargen_y_scale; /* $D05B: character generator Y scaling */
    unsigned char side_border_lo;  /* $D05C: left side border LSB */
    unsigned char side_border_hi;  /* $D05D: left side border MSB & hot registers */
    unsigned char display_row_width_lo; /* $D05E: active display width LSB */
    unsigned char sprite_h640_msbs; /* $D05F: sprite H640 MSBs */
    unsigned char screen_ram_base_lo; /* $D060: screen RAM base bits 0-7 */
    unsigned char screen_ram_base_mid; /* $D061: screen RAM base bits 8-15 */
    unsigned char screen_ram_base_hi;  /* $D062: screen RAM base bits 16-23 */
    unsigned char screen_ram_base_bank; /* $D063: screen RAM base bank, row width MSB & FCM_MCM */
    unsigned char colour_ram_base_lo; /* $D064: colour RAM base bits 0-7 */
    unsigned char colour_ram_base_hi; /* $D065: colour RAM base bits 8-15 */
    unsigned char xcounter_delay;  /* $D066: xcounter pipeline delay / test pattern */
    unsigned char sprite_first_x;  /* $D067: sprite start X coordinate LSB */
    unsigned char charset_base_lo; /* $D068: character generator base bits 0-7 */
    unsigned char charset_base_mid; /* $D069: character generator base bits 8-15 */
    unsigned char charset_base_hi;  /* $D06A: character generator base bits 16-23 */
    unsigned char sprite_16col_en; /* $D06B: sprite 16-colour mode flags */
    unsigned char sprite_ptr_lo;   /* $D06C: sprite pointer base bits 0-7 */
    unsigned char sprite_ptr_mid;  /* $D06D: sprite pointer base bits 8-15 */
    unsigned char sprite_ptr_hi;   /* $D06E: sprite pointer base bits 16-23 */
    unsigned char first_raster_vga60; /* $D06F: first raster & VGA60 select */
    unsigned char palette_sel;     /* $D070: palette bank select routing */
    unsigned char bitplane_16col_en; /* $D071: bitplane 16-colour mode flags */
    unsigned char sprite_y_adjust; /* $D072: sprite Y adjust offset */
    unsigned char alpha_delay_scale; /* $D073: alpha delay & ycounter scale */
    unsigned char sprite_alpha_blend; /* $D074: sprite alpha blend enables */
    unsigned char sprite_alpha_val;  /* $D075: sprite alpha blend global value */
    unsigned char sprite_v400_en;  /* $D076: sprite V400 enables */
    unsigned char sprite_v400_y_msb; /* $D077: sprite V400 Y position MSBs */
    unsigned char sprite_v400_y_smsb; /* $D078: sprite V400 Y position super MSBs */
    unsigned char raster_compare_lo; /* $D079: raster compare LSB */
    unsigned char raster_compare_hi; /* $D07A: raster compare MSB & source select */
    unsigned char display_row_count; /* $D07B: display row count */
    unsigned char bitplane_bank;   /* $D07C: bitplane bank & polarity select */
    unsigned char debug_x_lo;      /* $D07D: Debug X position LSB / value read-back */
    unsigned char debug_y_lo;      /* $D07E: Debug Y position LSB */
    unsigned char debug_hi;        /* $D07F: Debug X/Y MSB & out-of-frame */
};
```

### 8.2. Added Audio Mixer & PCM Defines
```c
#define AUDIO_MIX_SEL      (*(volatile unsigned char *)0xD6F4)
#define AUDIO_MIX_DATA     (*(volatile unsigned char *)0xD6F5)

#define AUDIO_DIGI_LEFT    (*(volatile unsigned short *)0xD6F8)
#define AUDIO_DIGI_RIGHT   (*(volatile unsigned short *)0xD6FA)
#define AUDIO_READBACK     (*(volatile unsigned short *)0xD6FC)

/* Helper to set the volume coefficient N (0-127) for mixing */
static inline void set_mixer_coefficient(unsigned char index, unsigned short volume) {
    /* Select low byte of coefficient index */
    AUDIO_MIX_SEL = index << 1;
    AUDIO_MIX_DATA = volume & 0xFF;
    /* Select high byte of coefficient index */
    AUDIO_MIX_SEL = (index << 1) | 1;
    AUDIO_MIX_DATA = volume >> 8;
}
```

### 8.3. Corrected Ethernet and Added UART Structs
```c
struct eth_regs {
    unsigned char ctrl;           /* $D6E0: Ethernet control */
    unsigned char irq_ctrl;       /* $D6E1: Interrupt / RX buffer control */
    unsigned char txsz_lo;        /* $D6E2: TX frame size low byte */
    unsigned char txsz_hi;        /* $D6E3: TX frame size high byte */
    unsigned char cmd;            /* $D6E4: Command register */
    unsigned char mode;           /* $D6E5: Phase / Filter mode */
    unsigned char miim_reg;       /* $D6E6: MIIM register select */
    unsigned char miim_val_lo;    /* $D6E7: MIIM value LSB */
    unsigned char miim_val_hi;    /* $D6E8: MIIM value MSB */
    unsigned char mac[6];         /* $D6E9-$D6EE: MAC address */
};
#define eth ((volatile struct eth_regs *)0xD6E0)

struct uart_regs {
    unsigned char data;           /* $D600: UART data register */
    unsigned char status;         /* $D601: UART status register */
    unsigned char ctrl;           /* $D602: UART control register */
    unsigned char divisor_lo;     /* $D603: UART divisor LSB */
    unsigned char divisor_hi;     /* $D604: UART divisor MSB */
    unsigned char irq_mask;       /* $D605: UART interrupt mask */
    unsigned char irq_flags;      /* $D606: UART interrupt flags */
    unsigned char port_data;      /* $D607: UART port data (caps/col8) */
    unsigned char port_ddr;       /* $D608: UART port DDR */
    unsigned char ext_ctrl;       /* $D609: Extended UART control */
};
#define uart ((volatile struct uart_regs *)0xD600)
```

### 8.4. Added UUID and ADC Registers
```c
/* 64-bit Hardware UUID ($FFD7100-$FFD7107) */
#define UUID64   ((volatile unsigned char *)0xFFD7100L)

/* 16-bit smoothed ADC registers ($FFD70F0-$FFD70F5) */
#define ADC1     (*(volatile unsigned short *)0xFFD70F0L)
#define ADC2     (*(volatile unsigned short *)0xFFD70F2L)
#define ADC3     (*(volatile unsigned short *)0xFFD70F4L)
```

### 8.5. Corrected Keyboard Event Queue Mappings
```c
#define ASCII_KEY_QUEUE    (*(volatile unsigned char *)0xD610)
#define PETSCII_KEY_QUEUE  (*(volatile unsigned char *)0xD619)
#define KEY_MODIFIERS      (*(volatile unsigned char *)0xD611)

#define KEY_MOD_LSHIFT     0x01
#define KEY_MOD_RSHIFT     0x02
#define KEY_MOD_CTRL       0x04
#define KEY_MOD_MEGA       0x08
#define KEY_MOD_ALT        0x10
#define KEY_MOD_NOSCRL     0x20
#define KEY_MOD_CAPS       0x40
#define KEY_MOD_DISABLE    0x80

static inline char getkey_ascii(void) {
    char k = ASCII_KEY_QUEUE;
    if (k) {
        ASCII_KEY_QUEUE = 0xFF; /* pulses write strobe to advance queue */
    }
    return k;
}

static inline char getkey_petscii(void) {
    char k = PETSCII_KEY_QUEUE;
    if (k != 0xFF) { /* returns 0xFF when empty */
        PETSCII_KEY_QUEUE = 0xFF; /* pulses write strobe to advance queue */
    }
    return k;
}
```

---

## 9. Step-by-Step Changes to Compiler C++ Code

### 9.1. Update Register Constants in `include/Mega65Registers.hpp`
- Redefine `DIV_ARG1` to `$D770` (aliased with `MULT_ARG1`).
- Redefine `DIV_ARG2` to `$D774` (aliased with `MULT_ARG2`).
- Redefine `DIV_RES` to `$D76C` (the integer quotient/whole part).
- Remove `DIV_REM` since there is no remainder register.
- Define `DIV_FRAC` as `$D768` to access the fractional part.
- Rename `DMA_CONTROL` to `DMA_ADDR_LSB_TRIG` (`0xD700`).
- Rename `DMA_ADDR_LO` to `DMA_ADDR_MSB` (`0xD701`).
- Rename `DMA_ADDR_MI` to `DMA_ADDR_BANK` (`0xD702`).
- Rename `DMA_ADDR_HI` to `DMA_MODE` (`0xD703`).
- Rename `DMA_ADDR_MB` to `DMA_ADDR_MB` (`0xD704`).

---

### 9.2. Update Code Generation in `src/main/AssemblerSimulatedOps.cpp`

*   **Modulo / Remainder (`mod.16`, `mod.32`, `emitMod16Code`, `emitMod32Code`, `emitSignedMathOp` op == 2)**:
    Implement the software subtraction step `remainder = dividend - quotient * divisor` instead of reading from unmapped hardware registers.
*   **Simulated DMA Triggers (`emitFillCode`, `emitMoveCode`, `emitCopyCode`)**:
    Correct the trigger sequences to write Bank (`$D702`) and MSB (`$D701`) first, then LSB (`$D700`) last.

---

### 9.3. Update DMA Intrinsics in `src/main/IRBuilder.cpp`
Correct the inline assembly generation in `__dma_copy` and `__dma_fill`:
```assembly
        lda #$00
        sta $D702       ; DMA bank = 0
        lda #$01
        sta $D701       ; DMA MSB = $01 (stack page)
        tsx
        txa
        clc
        adc #1
        sta $D700       ; DMA LSB & trigger (from stack)
```

---

## 10. Step-by-Step Changes to platform headers and Assembly Libraries

### 10.1. Update RTC Library Code
In [lib/stdlib/rtc.c](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/rtc.c) and [lib/stdlib_zp/rtc.c](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/rtc.c), update `rtc_read()` to fetch indices correctly matching VHDL registers:
```diff
-    unsigned char wday = rtc[3];
-    unsigned char day = rtc[4];
-    unsigned char month = rtc[5];
-    unsigned char year = rtc[6];
+    unsigned char day = rtc[3];
+    unsigned char month = rtc[4];
+    unsigned char year = rtc[5];
+    unsigned char wday = rtc[6];
```

### 10.2. Update Division Assembly Libraries
In [lib/stdlib/div.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/div.s), [lib/stdlib_zp/div.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/div.s), [lib/stdlib/ldiv.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/ldiv.s), [lib/stdlib_zp/ldiv.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/ldiv.s), [lib/stdlib/itoa.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/itoa.s), [lib/stdlib_zp/itoa.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/itoa.s), [lib/stdlib/ltoa.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib/ltoa.s), and [lib/stdlib_zp/ltoa.s](file:///home/duck/m65/inpg/m65compiler.dev_v1.0.5/lib/stdlib_zp/ltoa.s):
- Correct dividend/divisor writes to `$D770`/`$D774` respectively.
- Correct quotient reads from `$D76C`–`$D76F`.
- Perform multiplication of quotient and denominator via `$D770`/`$D774`.
- Calculate `remainder = numerator - product` in software.
