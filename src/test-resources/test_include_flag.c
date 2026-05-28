/* test_include_flag: validates -I flag with space-separated path.
 * Must be compiled with -I lib/include (space) to find string.h.
 * If the include fails, memset will be undeclared and compilation fails.
 */
#include <string.h>

char buf[10];

int main() {
    memset(buf, 0, 10);
    buf[0] = strlen("hello");
    return 0;
}
