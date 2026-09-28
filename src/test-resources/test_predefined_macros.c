/* test_predefined_macros.c — Verify predefined compiler macros
 *
 * Tests that all standard and platform-specific predefined macros
 * are defined with expected values.
 */

/* === Compiler identification === */
#ifndef __cc45__
#error "__cc45__ not defined"
#endif

#ifndef __CC45_VERSION__
#error "__CC45_VERSION__ not defined"
#endif

/* === Platform identification === */
#ifndef __MEGA65__
#error "__MEGA65__ not defined"
#endif

#ifndef __45GS02__
#error "__45GS02__ not defined"
#endif

#ifndef __6502__
#error "__6502__ not defined"
#endif

/* === C standard conformance === */
#ifndef __STDC__
#error "__STDC__ not defined"
#endif

#ifndef __STDC_VERSION__
#error "__STDC_VERSION__ not defined"
#endif

#ifndef __STDC_HOSTED__
#error "__STDC_HOSTED__ not defined"
#endif

/* === C11+ optional feature-test macros === */
#ifndef __STDC_NO_VLA__
#error "__STDC_NO_VLA__ not defined"
#endif

#ifndef __STDC_NO_THREADS__
#error "__STDC_NO_THREADS__ not defined"
#endif

#ifndef __STDC_NO_ATOMICS__
#error "__STDC_NO_ATOMICS__ not defined"
#endif

/* === Type sizes (GCC-compatible) === */
#ifndef __SIZEOF_CHAR__
#error "__SIZEOF_CHAR__ not defined"
#endif

#ifndef __SIZEOF_INT__
#error "__SIZEOF_INT__ not defined"
#endif

#ifndef __SIZEOF_LONG__
#error "__SIZEOF_LONG__ not defined"
#endif

#ifndef __SIZEOF_POINTER__
#error "__SIZEOF_POINTER__ not defined"
#endif

/* === Verify expected values via static globals === */
/* If any value is wrong, the generated code will have the wrong constant */

int stdc_version_ok;
int type_sizes_ok;
int platform_ok;

int main() {
    /* C17 */
    stdc_version_ok = (__STDC_VERSION__ == 201710L);

    /* 8-bit char, 16-bit int, 32-bit long, 16-bit pointers */
    type_sizes_ok = (__SIZEOF_CHAR__ == 1) &&
                    (__SIZEOF_INT__ == 2) &&
                    (__SIZEOF_LONG__ == 4) &&
                    (__SIZEOF_POINTER__ == 2);

    /* Freestanding */
    platform_ok = (__STDC_HOSTED__ == 0) &&
                  (__MEGA65__ == 1) &&
                  (__45GS02__ == 1);

    if (!stdc_version_ok) return 1;
    if (!type_sizes_ok) return 2;
    if (!platform_ok) return 3;

    return 0;
}
