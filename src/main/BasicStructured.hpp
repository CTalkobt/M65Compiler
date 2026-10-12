// BasicStructured — block IF and procedures for basic45, lowered to BASIC
//
//   if <cond> then        (THEN ends the line)   if (<cond>)=0 then __else<N>
//     ...                                          ...
//   else                                           goto __endif<N>
//                                                  __else<N>:
//     ...                                          ...
//   endif                                          __endif<N>:   (__else<N>: without else)
//
//   proc name                                      goto __endproc<N>
//                                                  name:
//     ...                                          ...
//   endproc                                        return
//                                                  __endproc<N>:
//
//   call name                                      gosub name
//
// (<cond>)=0 tests "false" for any value (BASIC's NOT is bitwise: NOT 5 is
// -6). A single-line IF ... THEN ... [ELSE ...] is BASIC's own and is left
// alone. Blocks nest.
#pragma once

#include <string>
#include <vector>

struct BasicStructuredResult {
    std::string text;
    std::vector<int> origin;                              // output line -> input line (1-based)
    std::vector<std::pair<int, std::string>> errors;      // (input line, message)
};

BasicStructuredResult lowerBasicStructured(const std::string& source);
