// DbgFile.hpp — a program's debug information, ln45 -g's <program>.dbg
// (#202; doc/bin/ln45.md, "Debug File"), loaded for dbg45: source lines,
// functions, types, variables and symbols, and the lookups a debugger needs.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct DbgFunction {
    std::string name;          // its label (_main, _sum__file for a static one)
    uint32_t start = 0, end = 0;
    int overlay = 0;           // 1..n (DbgFile::overlays), 0: resident
    int object = 0;
};

struct DbgLine {
    uint32_t addr = 0;
    int file = 0, line = 0;
    int overlay = 0;
};

struct DbgMember {
    std::string name;
    int offset = 0;
    int bit = -1, width = 0;   // a bit-field
    std::string type;
};

struct DbgType {
    int size = 0;
    std::vector<DbgMember> members;
};

struct DbgVar {
    std::string kind, scope, name, type;   // scope: a function's label, "" for a file's
    enum Loc { NONE, ADDR, ZP, FRAME, REG } loc = NONE;
    long value = 0;            // ADDR: the address; ZP: the base-page byte; FRAME: the offset
    std::string reg;           // REG: AX, X, Y, Z
    int object = 0;
};

class DbgFile {
public:
    bool load(const std::string& path, std::string& err);

    std::string program;
    uint32_t zpPage = 0, zpPageIsr = 0;
    bool hasZpPage = false;
    std::vector<std::string> overlays;
    std::vector<std::string> objects, files;
    std::vector<DbgLine> lines;                 // by address
    std::vector<DbgFunction> functions;         // by start
    std::map<std::string, DbgType> types;
    std::vector<DbgVar> variables;
    std::map<std::string, uint32_t> symbols;

    // the function holding an address (resident code, or the overlay's)
    const DbgFunction* functionAt(uint32_t addr, int overlay = 0) const;
    // a function by its label or C name (sum: _sum or _sum__file)
    const DbgFunction* functionNamed(const std::string& name) const;
    // its C name: the label less the leading _ and a static's __file
    std::string cName(const std::string& label) const;
    // the source line an address is in (the last line start at or before it
    // in the same function), nullptr where there is none
    const DbgLine* lineAt(uint32_t addr, int overlay = 0) const;
    // where a source line's code starts (a file given by its name, its path
    // or a path's end); the next line with code when it has none: line set
    std::vector<uint32_t> addressesOf(const std::string& file, int& line) const;
    int fileIndex(const std::string& file) const;   // -1: not one of the files
    // a function's own variables (locals, parameters, its statics)
    std::vector<const DbgVar*> variablesOf(const std::string& function) const;
    // a variable by name as code in a function sees it: its own, then the
    // file's (its object's), then any global
    const DbgVar* lookup(const std::string& name, const std::string& function) const;
    // a type descriptor's size in bytes (lib45.md 1.12)
    int sizeOf(const std::string& type) const;
    // a symbol's address, or -1
    long symbol(const std::string& name) const;
};
