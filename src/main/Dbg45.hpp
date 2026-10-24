// Dbg45.hpp — dbg45's debugger: a cc45 program in xemu, by its debug file
// (#202): breakpoints by source line and function, stepping by line,
// the call stack, variables by C name and type. The command line and the
// Debug Adapter Protocol (dbg45_main.cpp) drive it.
#pragma once
#include "DbgFile.hpp"
#include "XemuMonitor.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Dbg45Options {
    std::string xemu = "xemu-xmega65";
    bool headless = false, fast = false;
    std::vector<std::string> xemuArgs;    // more of xemu's options (a disk: -8 x.d81)
    int timeoutMs = -1;                   // a run / continue without a stop (tests); -1: none
    // while it runs, asked now and then: true pauses it (the editor's pause)
    std::function<bool()> interrupt;
};

struct Dbg45Frame {
    const DbgFunction* function = nullptr;
    uint32_t pc = 0;                      // the innermost frame's PC, a caller's call
    const DbgLine* line = nullptr;
};

// A value: its type descriptor and bytes (and where they are, -1: nowhere)
struct Dbg45Value {
    std::string type;
    std::vector<uint8_t> bytes;
    long addr = -1;
};

class Dbg45 {
public:
    Dbg45Options opt;
    DbgFile dbg;
    std::vector<uint8_t> image;           // the program as loaded (PRG body)
    uint32_t imageBase = 0;

    bool open(const std::string& prg, std::string& err);   // the program and its .dbg

    // breakpoints: a function, file:line, a line of the current file or *ADDR
    struct Breakpoint { int id; std::string where; std::vector<uint16_t> addrs; };
    std::map<int, Breakpoint> breakpoints;
    int addBreakpoint(const std::string& where, std::string& err);   // its id, or 0
    bool deleteBreakpoint(int id);

    // execution; each stops (true) or reports why not in err
    enum Stop { NONE, BREAKPOINT, STEP, EXITED, TIMEOUT, PAUSED, ERROR };
    Stop run(bool toMain, std::string& err);          // start (again): to main, or to a breakpoint
    Stop cont(std::string& err);
    Stop stepLine(bool over, std::string& err);       // step / next
    Stop finish(std::string& err);
    Stop stepInstruction(std::string& err);
    Stop pause(std::string& err);
    void kill();
    bool started() const { return started_; }
    bool exited() const { return exited_; }
    int exitStatus() const { return exitStatus_; }
    int lastBreakpoint() const { return lastBp_; }

    // state where it stopped
    CpuRegs regs;
    std::vector<Dbg45Frame> backtrace();
    // variables: those of frame n (0: innermost), as name -> value
    std::vector<std::pair<const DbgVar*, std::string>> locals(size_t frame, bool params);
    bool evaluate(const std::string& expr, size_t frame, Dbg45Value& out, std::string& err);
    std::string format(const Dbg45Value& v, int depth = 0);
    // a variable's members / elements as (name, value) pairs (structs, arrays)
    std::vector<std::pair<std::string, Dbg45Value>> children(const Dbg45Value& v);
    bool readMemory(uint32_t addr, size_t n, std::vector<uint8_t>& out) { return mon.read(addr, n, out); }
    bool evaluateVar(const DbgVar& v, size_t frame, Dbg45Value& out, std::string& err);
    std::string sourceLine(int file, int line);
    std::string location(const Dbg45Frame& f);       // "sum (p=...) at file.c:20"

private:
    XemuMonitor mon;
    std::string prg_;
    bool started_ = false, exited_ = false;
    int exitStatus_ = 0, nextBp_ = 1, lastBp_ = 0;
    long exitAddr_ = -1;
    std::set<uint16_t> active_;                       // breakpoints in xemu
    std::map<int, std::vector<std::string>> sources_;

    void syncBreakpoints(const std::set<uint16_t>& temp);
    Stop runUntilStop(const std::set<uint16_t>& temp, std::string& err);
    bool genuine(uint16_t pc);                        // the program's code at pc (not the ROM mapped there)
    bool readVar(const DbgVar& v, size_t frame, Dbg45Value& out, std::string& err);
    long returnAddressOf(const DbgFunction* f, uint16_t sp, size_t* at = nullptr);
    uint8_t byteAt(uint32_t addr);
    std::map<uint32_t, std::vector<uint8_t>> cache_;  // 16-byte lines of code (cleared at each stop)
};
