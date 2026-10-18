/* overlay.h — code overlays (cc45 #pragma cc45 overlay(name), ln45)
 *
 * A file with #pragma cc45 overlay(name) (or compiled with -foverlay=name)
 * has its code in the code overlay name. ln45 links every overlay to run in
 * one region of bank 0 (below the stack's share; the heap ends there) and
 * writes their code to <program>.ov1, which the startup code loads into
 * far memory (bank 5 by default: ln45 --overlay-store) from the drive the
 * program came from, under the name <PROGRAM>.OV1 (--overlay-name). Calls
 * into an overlay go through resident stubs, which copy it into the region
 * by DMA when another is there; the overlays' variables stay resident.
 *
 * These are only defined in a program that has overlays.
 */
#pragma once

/* The overlay in the region: 1, 2, ... in the order ln45 meets them, 0
   before the first call into one */
extern unsigned char __ovl_current;

/* Where the overlays run */
extern char __ovl_region[];
