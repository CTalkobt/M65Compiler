/* load.c — a file from disk to any 28-bit address, packed or not (#444;
 * first written for the Midwinter demo). See <load.h>. */
#include <cbm.h>
#include <dma.h>
#include <unpack.h>
#include <load.h>

#define ATTIC 0x8000000UL

unsigned char load_error;
static unsigned long stage = 0x40000UL;
static unsigned int stage_size = 0;          /* 0: up to the end of its bank */

void load_stage(unsigned long lin, unsigned int size) {
    stage = lin;
    stage_size = size;
}

/* a KERNAL LOAD to chip RAM at lin (bank and address); the bytes loaded */
static unsigned int kernal_load(const char *name, unsigned char len, unsigned char device, unsigned long lin) {
    unsigned int end;
    unsigned char a;
    if (!device) {
        /* the drive the program came from (the KERNAL's $BA: four digits,
           absolute, not the program's own base page) */
        device = *(volatile unsigned char *)0x00BA;
        if (device < 8) device = 8;
    }
    cbm_setbnk((char)(lin >> 16), 0);
    cbm_setnam((char *)name, (char)len);
    cbm_setlfs(0, (char)device, 0);
    if (__jsr(KERNAL_LOAD, .A = 0, .XY = (unsigned int)lin, .A -> a, .XY -> end).Carry) {
        cbm_setbnk(0, 0);
        load_error = a ? a : 4;
        return 0;
    }
    cbm_setbnk(0, 0);
    load_error = 0;
    return end - (unsigned int)lin;          /* the end is the byte after the last */
}

unsigned int load_far(const char *name, unsigned char len, unsigned char device, unsigned long lin) {
    unsigned int n;
    if (lin < ATTIC) return kernal_load(name, len, device, lin);
    n = kernal_load(name, len, device, stage);
    if (!n) return 0;
    if (stage_size && n > stage_size) {
        load_error = LOAD_TOO_BIG;
        return 0;
    }
    dma_copy_far(lin, stage, n);
    return n;
}

unsigned long load_unpack(const char *name, unsigned char len, unsigned char device, unsigned long at) {
    unsigned long n;
    if (!load_far(name, len, device, at)) return 0;
    n = unpack(at);
    if (!n) load_error = LOAD_NOT_PACKED;
    return n;
}
