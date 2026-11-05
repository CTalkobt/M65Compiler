// BasicLister — a tokenized CBM BASIC program as LIST shows it
//
// BASIC V2 (C64, $0801), V7 (C128, $1C01; its $CE xx and $FE xx tokens) and
// BASIC 65 (MEGA65, $2001; the tokens of basic45's tokenizer). Keywords and
// letters come out in upper case; in strings and REM text PETSCII control
// codes are {name} escapes (as basic45 and petcat write them), other bytes
// above $7F {$xx}.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class BasicDialect { AUTO, V2, V7, B65 };

struct BasicListing {
    std::string text;
    BasicDialect dialect = BasicDialect::AUTO;   // the one used
    std::vector<std::string> warnings;
    bool ok = false;
    std::string error;
};

// prg: the file with its 2-byte load address; AUTO picks the dialect from it
BasicListing listBasic(const std::vector<uint8_t>& prg, BasicDialect dialect = BasicDialect::AUTO);
const char* basicDialectName(BasicDialect d);

// One BASIC V2 / V7 keyword token at p[i] ($80-$CB, $CC-$FD, $CE xx, $FE xx;
// v7 false: V2's only): its name in upper case, i moved past it; false when
// p[i] is not one
bool decodeV7Token(const std::vector<uint8_t>& p, size_t& i, std::string& out, bool v7 = true);

// The CBM BASICs another dialect's program can be in (basic45 --convert)
enum class CbmBasic { V2, V35, V7, SIMONS };

// One keyword token of BASIC V2, 3.5 (C16 / Plus/4: V7's $CC-$FD, $CE is
// RLUM), 7.0 or Simons' BASIC ($64 xx, xx 1-127; TSB's extra keywords too) at
// p[i]: its name in upper case, i moved past it; false when p[i] is not one
bool decodeCbmToken(const std::vector<uint8_t>& p, size_t& i, std::string& out, CbmBasic d);
// Simons' keyword xx ($64 xx) that only TSB (Tuned Simons' BASIC) has
bool isTsbOnlyToken(uint8_t xx);
// A keyword (upper case) is one of Simons' BASIC's (or TSB's)
bool isSimonsKeyword(const std::string& name);

// A BASIC V7 text listing ("10 PRINT ...", any case) tokenized as the C128
// would: keywords outside strings, REM and DATA text, the longest first;
// in strings {name} / {$xx} escapes (basic45's) are their PETSCII bytes,
// letters unshifted. load: the program's address ($1C01). Problems in
// warnings ("line N: ...").
std::vector<uint8_t> crunchV7(const std::string& listing, uint16_t load, std::vector<std::string>& warnings);
// The same for BASIC 3.5 or Simons' BASIC (whose keywords are matched first,
// in its table's order, as its tokenizer does)
// (tsb: TSB's own Simons' keywords too: DO, CLS, X!, MAP, D!)
std::vector<uint8_t> crunchCbm(const std::string& listing, uint16_t load, std::vector<std::string>& warnings, CbmBasic d,
                               bool tsb = false);
