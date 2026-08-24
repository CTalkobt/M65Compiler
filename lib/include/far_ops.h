#ifndef FAR_OPS_H
#define FAR_OPS_H

#include <far_ptr.h>
#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Phase 27: Far Pointer Operations
 *
 * Element access, bulk transfers, and arithmetic on 24-bit far pointers
 * ========================================================================== */

/* ============================================================================
 * Element-Wise Access (Single Values)
 * ========================================================================== */

/* Read operations: fetch value from far memory */
uint8_t far_read_uint8(far_ptr_t ptr, int offset);
int8_t far_read_int8(far_ptr_t ptr, int offset);

uint16_t far_read_uint16(far_ptr_t ptr, int offset);
int16_t far_read_int16(far_ptr_t ptr, int offset);

uint32_t far_read_uint32(far_ptr_t ptr, int offset);
int32_t far_read_int32(far_ptr_t ptr, int offset);

float far_read_float(far_ptr_t ptr, int offset);
double far_read_double(far_ptr_t ptr, int offset);

/* Write operations: store value to far memory */
void far_write_uint8(far_ptr_t ptr, int offset, uint8_t val);
void far_write_int8(far_ptr_t ptr, int offset, int8_t val);

void far_write_uint16(far_ptr_t ptr, int offset, uint16_t val);
void far_write_int16(far_ptr_t ptr, int offset, int16_t val);

void far_write_uint32(far_ptr_t ptr, int offset, uint32_t val);
void far_write_int32(far_ptr_t ptr, int offset, int32_t val);

void far_write_float(far_ptr_t ptr, int offset, float val);
void far_write_double(far_ptr_t ptr, int offset, double val);

/* ============================================================================
 * Bulk Memory Operations
 * ========================================================================== */

/* Copy from far source to far destination (may cross banks via DMA) */
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len);

/* Copy from far source to local destination (bank-aware) */
void far_memcpy_to_local(void* dst, far_ptr_t src, size_t len);

/* Copy from local source to far destination (bank-aware) */
void far_memcpy_from_local(far_ptr_t dst, const void* src, size_t len);

/* Fill far memory with pattern (bank-aware) */
void far_memset(far_ptr_t ptr, int val, size_t len);

/* Zero-initialize far memory */
void far_memzero(far_ptr_t ptr, size_t len);

/* ============================================================================
 * Pointer Arithmetic
 * ========================================================================== */

/* Add offset to far pointer (stays in same bank) */
far_ptr_t far_add(far_ptr_t ptr, int offset);

/* Subtract offset from far pointer (stays in same bank) */
far_ptr_t far_sub(far_ptr_t ptr, int offset);

/* Difference between two far pointers (must be same bank) */
int far_diff(far_ptr_t ptr1, far_ptr_t ptr2);

/* Increment pointer by 1 byte */
far_ptr_t far_inc(far_ptr_t ptr);

/* Decrement pointer by 1 byte */
far_ptr_t far_dec(far_ptr_t ptr);

/* ============================================================================
 * Comparison and Validation
 * ========================================================================== */

/* Compare two far pointers (equal, less than, greater than) */
int far_cmp(far_ptr_t ptr1, far_ptr_t ptr2);
int far_eq(far_ptr_t ptr1, far_ptr_t ptr2);
int far_lt(far_ptr_t ptr1, far_ptr_t ptr2);
int far_gt(far_ptr_t ptr1, far_ptr_t ptr2);

/* ============================================================================
 * Array-Style Access Helpers
 * ========================================================================== */

/* Access array element: base_ptr[index * element_size] */
far_ptr_t far_array_element(far_ptr_t base, int index, size_t element_size);

/* Read array element */
void far_array_read(far_ptr_t base, int index, size_t element_size,
                    void* dst, size_t dst_size);

/* Write array element */
void far_array_write(far_ptr_t base, int index, size_t element_size,
                     const void* src, size_t src_size);

/* ============================================================================
 * String Operations (in far memory)
 * ========================================================================== */

/* Get length of null-terminated string in far memory */
size_t far_strlen(far_ptr_t str);

/* Copy null-terminated string from far to local */
void far_strcpy_to_local(char* dst, far_ptr_t src, size_t max_len);

/* Copy null-terminated string from local to far */
void far_strcpy_from_local(far_ptr_t dst, const char* src, size_t max_len);

/* Compare null-terminated strings (one in far, one local) */
int far_strcmp(far_ptr_t str1, const char* str2);

/* ============================================================================
 * Bulk Transfer Variants (Performance Hints)
 * ========================================================================== */

/* Fast copy if both in same bank, cross-bank copy otherwise */
void far_memcpy_smart(far_ptr_t dst, far_ptr_t src, size_t len);

/* Streaming copy (may be optimized by DMA hardware) */
void far_memcpy_stream(far_ptr_t dst, far_ptr_t src, size_t len);

#endif
