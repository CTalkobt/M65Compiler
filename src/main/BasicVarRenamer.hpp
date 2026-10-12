// BasicVarRenamer — long BASIC variable names to unique short ones
//
// BASIC 65 tells variables apart by their first two characters (plus the
// $ / % type suffix and array-ness): SCORE and SCALE are one variable SC,
// and STORE is the reserved status variable ST. A name longer than two
// characters is renamed to a short name no other variable uses, that is not
// a reserved variable (ST TI DS ER EL) and not a keyword of one or two
// characters (TO IF ON OR GO FN DO ...). Names of one or two characters are
// kept as written.
#pragma once

#include <map>
#include <string>
#include <vector>
#include "BasicTokenizer.hpp"

class BasicVarRenamer {
public:
    // Rename the variables in all lines (tokens of each BASIC line)
    void rename(std::vector<std::vector<BasicToken>>& lines);

    // long name (upper case, with its $ / % suffix) -> short name
    const std::map<std::string, std::string>& mapping() const { return map_; }

private:
    std::map<std::string, std::string> map_;
};
