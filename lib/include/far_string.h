/* far_string.h — memory and string functions for any mix of local and far
 * pointers (lib/stdlib/stdlib_overloads.c)
 *
 * cc45 has no overloading by argument type: each combination is a function
 * of its own. The suffix names the parameter types in order: pv void *,
 * pc char *, fp far_ptr_t, i int, st / z size_t. A function whose
 * destination is far returns the far pointer. Far data moves by DMA; strings
 * have no length limit.
 *
 *   memcpy__fpvst(buf, far, n)      far -> local
 *   strcpy__fppc(far, "text")       local string -> far
 *   strcmp__fpfp(far1, far2)        two far strings
 */
#ifndef FAR_STRING_H
#define FAR_STRING_H

#include <far_ptr.h>
#include <stddef.h>
#include <stdint.h>

/* memcpy */
void *memcpy__pvpvst(void *dst, const void *src, size_t len);
void *memcpy__fpvst(void *dst, far_ptr_t src, size_t len);
far_ptr_t memcpy__pvfpst(far_ptr_t dst, const void *src, size_t len);
far_ptr_t memcpy__fpfpst(far_ptr_t dst, far_ptr_t src, size_t len);

/* memset */
void *memset__pvist(void *ptr, int val, size_t len);
far_ptr_t memset__fpist(far_ptr_t ptr, int val, size_t len);

/* strlen */
size_t strlen__pc(const char *str);
size_t strlen__fp(far_ptr_t str);

/* strcpy */
char *strcpy__pcpc(char *dst, const char *src);
char *strcpy__pcfp(char *dst, far_ptr_t src);
far_ptr_t strcpy__fppc(far_ptr_t dst, const char *src);
far_ptr_t strcpy__fpfp(far_ptr_t dst, far_ptr_t src);

/* strcmp: <0, 0, >0 by the first differing (unsigned) byte */
int strcmp__pcpc(const char *str1, const char *str2);
int strcmp__fppc(far_ptr_t str1, const char *str2);
int strcmp__pcfp(const char *str1, far_ptr_t str2);
int strcmp__fpfp(far_ptr_t str1, far_ptr_t str2);

/* strcat */
char *strcat__pcpc(char *dst, const char *src);
char *strcat__pcfp(char *dst, far_ptr_t src);
far_ptr_t strcat__fppc(far_ptr_t dst, const char *src);
far_ptr_t strcat__fpfp(far_ptr_t dst, far_ptr_t src);

/* memcmp */
int memcmp__pvpvz(const void *ptr1, const void *ptr2, size_t len);
int memcmp__fpvz(far_ptr_t ptr1, const void *ptr2, size_t len);
int memcmp__pvfz(const void *ptr1, far_ptr_t ptr2, size_t len);
int memcmp__fpfz(far_ptr_t ptr1, far_ptr_t ptr2, size_t len);

/* Typed elements of a far array (element index; any index up to 65535) */
int16_t far_array_read_int16(far_ptr_t base, int index);
void far_array_write_int16(far_ptr_t base, int index, int16_t val);
uint8_t far_array_read_uint8(far_ptr_t base, int index);
void far_array_write_uint8(far_ptr_t base, int index, uint8_t val);
uint32_t far_array_read_uint32(far_ptr_t base, int index);
void far_array_write_uint32(far_ptr_t base, int index, uint32_t val);

#endif
