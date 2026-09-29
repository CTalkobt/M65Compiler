/* strlcpy.c — Size-bounded string copy (BSD)
 *
 * Copies at most size-1 characters, always NUL-terminates.
 * Returns strlen(src) (total length of string it tried to create).
 */

int strlcpy(char *dst, const char *src, int size) {
    int i = 0;
    if (size > 0) {
        while (i < size - 1 && src[i]) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = 0;
    }
    /* Count remaining src length for return value */
    int len = i;
    while (src[len]) len++;
    return len;
}
