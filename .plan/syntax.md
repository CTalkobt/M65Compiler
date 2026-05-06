# Syntax Improvement Recommendations for cc45 and ca45

Based on a review of the `cc45` and `ca45` documentation, here are several recommendations to make the syntax more straightforward, standard-compliant, and easier for users to remember:

## `cc45` (C Compiler) Recommendations

[W] ### 1. Align Default Signedness with C Standards
*   **Issue:** The documentation notes that all types default to **unsigned**. This is a major deviation from standard C (where `int` is signed by default) and will almost certainly trip up developers porting code or writing standard algorithms.
*   WON'T DO: **Recommendation:** Make `int` and `char` signed by default to match user expectations. You can introduce a compiler flag (e.g., `-funsigned-char` or a global unsigned mode if necessary) for those who rely on the current behavior.

[ ] ### 2. Modernize Inline Assembly Variable Interpolation
*   **Issue:** Users must manually apply internal compiler mangling prefixes (`_p_`, `_l_`, `_`) and manually calculate struct offsets in `__asm__` statements. This is highly error-prone.
*   **Recommendation:** Introduce a template syntax for variable interpolation in `__asm__`.
    *   Allow `%{varname}` to automatically resolve to `_p_varname`, `_l_varname`, or `_varname` based on scope.
    *   Allow `%{struct_var.member}` to automatically compute the correct `+ offset` suffix.
    *   *Example:* `__asm__("lda.sp %{s.id}")` instead of `__asm__("lda.sp _l_s+0")`.

[ ] ### 3. Expose CRT Pragmas as Command-Line Flags
*   **Issue:** Configuring the environment via `#pragma crt` requires modifying source code, which complicates builds that target different environments.
*   **Recommendation:** Mirror the `#pragma crt` options as compiler command-line arguments (e.g., `-mheap`, `-mno-bssinit`). This makes Makefile integration much cleaner.

---

## `ca45` (Assembler) Recommendations

[ ] ### 1. Unify Instruction Width Identifiers
*   **Issue:** Simulated opcodes use a mix of naming conventions. Some use `.width` suffixes (`add.16`, `mul.32`), while others use embedded register hints (`ldax`, `stax`).
*   **Recommendation:** Standardize the approach. If you keep the `.16` suffix style, consider `ld.16 .ax, src`. Alternatively, adopt the 68k/KickAssembler style with `w` (word) and `l` (long) suffixes (e.g., `addw`, `addl`, `ldw`, `stw`) to make the entire simulated ISA uniform.

[ ] ### 2. Resolve `.text` Directive Overloading
*   **Issue:** `.text` is used as both a segment switch shortcut (alias for `.segment "code"`) and as a data emission directive for PETSCII strings (`.text "Hello"`). This dual-meaning is confusing.
*   **Recommendation:** Deprecate `.text "string"` in favor of explicit directives like `.petscii "string"` or `.string "string"`. Reserve `.text` exclusively for segment management to match GNU `as` conventions.

[W] ### 3. Soften the `nop` Restriction
*   **Issue:** The assembler actively disallows `nop` because `$EA` is an instruction prefix on the 45GS02, telling the user to use `clv` instead. This breaks deep-rooted 6502 muscle memory.
*   **Recommendation:** Instead of failing, allow `nop` but have it automatically emit `$B8` (`clv`). It acts as a safe 1-byte NOP, honoring the developer's intent without breaking the CPU's prefix behavior.

[ ] ### 4. Refine Procedure Parameter Syntax
*   **Issue:** The parameter type prefixes in `proc _name, w#p1, b#p2` are cryptic (`w#`, `b#`).
*   **Recommendation:** Adopt a clearer typed syntax, such as `proc _name, p1:word, p2:byte`.

[ ] ### 5. Clarify Stack Addressing Aliases
*   **Issue:** The documentation lists both `lda offset, s` and `lda.sp offset`. Having overlapping simulated opcodes for the exact same function creates mental overhead.
*   **Recommendation:** Standardize on one as the primary canonical syntax. `lda.sp offset` aligns nicely with the frame-relative `lda.fp offset` syntax, making the entire suite of SP/FP simulated opcodes feel cohesive.
