/* strlcpy.c — Safe string copy for cc45 / MEGA65 */

#include <string.h>

/* strlcpy: copy src to dst, truncating if needed
 * BSD extension, safer than strcpy.
 * Returns: length of src (same as strlen(src))
 */
int strlcpy(char *dst, const char *src, int size) {
    int src_len = strlen((char *)src);

    if (size > 0) {
        int copy_len = src_len;
        if (copy_len >= size) {
            copy_len = size - 1;
        }
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }

    return src_len;
}
