/* load.h — a file from disk to any 28-bit address, packed or not (#444)
 *
 *   load_far     a file (a PRG: its first two bytes, the load address,
 *                skipped) to lin: chip RAM ($0000000-$005FFFF) by the
 *                KERNAL's LOAD into that bank, attic RAM ($8000000 on)
 *                through the stage and a DMA copy. The bytes loaded; 0 on an
 *                error (load_error says which)
 *   load_unpack  a pack45 stream loaded to at (as load_far) and unpacked to
 *                the destination in its header (<unpack.h>); the bytes
 *                unpacked; 0 on an error
 *   load_stage   where a file for attic RAM is loaded first: chip RAM at lin,
 *                size bytes (0: up to the end of its bank), which the file
 *                must fit (default: $40000, the whole of bank 4)
 *
 * device 0 is the drive the program was loaded from (the KERNAL's $BA), or 8
 * when none is known. name is in bank 0, len characters long, in PETSCII:
 * with cc45's default encoding a file the directory lists as LEVEL1 is
 * "level1" in C (lowercase letters are PETSCII's unshifted ones).
 *
 * Unlike cbm_load_far (<cbm.h>), these give the length loaded (from the end
 * address LOAD returns), which a copy to attic RAM needs and a caller can
 * check. The KERNAL runs the load: a program that has taken over the
 * machine (interrupts off, memory mapped otherwise) hands it back around the
 * call.
 *
 * load_error: 0 none, else the KERNAL's error (4 file not found, 5 device
 * not present, ...), LOAD_TOO_BIG (a file for attic RAM larger than the
 * stage) or LOAD_NOT_PACKED (load_unpack: not a pack45 stream). It is its own
 * variable, not _oserror: two of its values are not the KERNAL's, and
 * _oserror would bring cbm_kernal.s45 in.
 *
 *   #include <load.h>
 *   if (!load_far("level1", 6, 0, 0x50000UL)) error(load_error);
 *   load_unpack("tiles", 5, 0, 0x8000000UL);   // packed, staged, unpacked
 */
#ifndef LOAD_H
#define LOAD_H

#define LOAD_TOO_BIG    0x80    /* a file for attic RAM larger than the stage */
#define LOAD_NOT_PACKED 0x81    /* load_unpack: not a pack45 stream */

extern unsigned char load_error;

unsigned int load_far(const char *name, unsigned char len, unsigned char device, unsigned long lin);
unsigned long load_unpack(const char *name, unsigned char len, unsigned char device, unsigned long at);
void load_stage(unsigned long lin, unsigned int size);

#endif
