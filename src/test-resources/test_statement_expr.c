/* test_statement_expr.c — Test GCC-style statement expressions */

#include <stdio.h>

int main() {
    /* Test 1: Simple integer result */
    int a = ({ 42; });
    printf("Test 1: %d\n", a);  /* Expected: 42 */

    /* Test 2: Computation in statement expression */
    int b = ({ int x = 10; int y = 20; x + y; });
    printf("Test 2: %d\n", b);  /* Expected: 30 */

    /* Test 3: Multiple statements with final expression */
    int c = ({ int i = 5; i = i * 2; i + 3; });
    printf("Test 3: %d\n", c);  /* Expected: 13 */

    /* Test 4: Statement expression in larger expression */
    int d = 100 + ({ 50; });
    printf("Test 4: %d\n", d);  /* Expected: 150 */

    /* Test 5: Nested statement expressions */
    int e = ({ int inner = ({ 7; }); inner * 2; });
    printf("Test 5: %d\n", e);  /* Expected: 14 */

    /* Test 6: Statement expression with no explicit result (should be 0) */
    int f = ({ int x = 99; x + 1; }) + 1;
    printf("Test 6: %d\n", f);  /* Expected: 101 */

    /* Test 7: Conditional in statement expression */
    int x = 5;
    int g = ({ if (x > 3) x = 20; else x = 10; x; });
    printf("Test 7: %d\n", g);  /* Expected: 20 */

    /* Test 8: Loop in statement expression */
    int h = ({ int sum = 0; int i; for (i = 1; i <= 5; i++) sum += i; sum; });
    printf("Test 8: %d\n", h);  /* Expected: 15 */

    /* Test 9: Member access on statement expression result */
    struct Point { int x; int y; } p = ({ struct Point pt = {3, 4}; pt; });
    printf("Test 9: x=%d, y=%d\n", p.x, p.y);  /* Expected: x=3, y=4 */

    /* Test 10: Array indexing on statement expression result */
    int arr[5] = {10, 20, 30, 40, 50};
    int i = ({ 2; });
    int val = ({ arr; })[i];
    printf("Test 10: %d\n", val);  /* Expected: 30 */

    printf("All tests passed!\n");
    return 0;
}
