// BasicStructured — basic45's extensions, lowered to plain BASIC 65
//
//   if <cond> then        (THEN ends the line)   if (<cond>)=0 then __else<N>
//     ...                                          ...
//   else                                           goto __endif<N>
//                                                  __else<N>:
//     ...                                          ...
//   endif                                          __endif<N>:   (__else<N>: without else)
//
//   proc name[(a, b$, ...)]                        goto __endproc<N>
//                                                  name:
//     ...                                          ...
//   endproc                                        return
//                                                  __endproc<N>:
//   call name[(x, y$, ...)]                        a = x : b$ = y$ : gosub name
//
//   select <expr>                                  zselN = <expr>     (zselN$ for a string)
//   case 1, 2                                      if (zselN=1 or zselN=2)=0 then __case<N>_2
//     ...                                          ...
//   case 3 to 5                                    goto __endsel<N> / __case<N>_2: / if (zselN>=3 and zselN<=5)=0 then ...
//   case is > 10                                   ... (zselN > 10) ...
//   case else                                      goto __endsel<N> / __case<N>_k:
//   endselect                                      __case<N>_k: / __endsel<N>:
//
//   .name                 a label local to its proc (or to the code outside procs)
//
//   asm ... endasm        assembly (ca45), linked after the program; a label
//                         at the start of a line is global: SYS {label}
//   incbin "file" as name the file's bytes after the program; {name} is their
//                         address, {name_size} their size
//   datainc "file"[, n] [as label]   DATA lines with the file's bytes (n per line, 16)
//
// (<cond>)=0 tests "false" for any value (BASIC's NOT is bitwise: NOT 5 is
// -6). A single-line IF ... THEN ... [ELSE ...] is BASIC's own and is left
// alone. Blocks nest.
#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

struct BasicStructuredResult {
    std::string text;
    std::vector<int> origin;                              // output line -> input line (1-based)
    std::vector<std::pair<int, std::string>> errors;      // (input line, message)
    std::string asmText;                                  // asm blocks and incbin data (ca45, -c)
    std::set<std::string> asmLabels;                      // labels there: called directly by SYS
    std::set<std::string> dataLabels;                     // incbin names (and _size)
    std::vector<std::pair<std::string, int>> procs;       // (name, input line)
};

// fileOf(line): the source file of an input line, for incbin/datainc paths
BasicStructuredResult lowerBasicStructured(const std::string& source,
                                           const std::function<std::string(int)>& fileOf = nullptr);
