// XemuMonitor.hpp — xemu's MEGA65 monitor socket (-uartmon :port), as dbg45
// drives it: start xemu with a program, registers, memory, breakpoints that
// stop before the opcode (the 'k' list: xemu with CTalkobt/xemu's monitor
// breakpoint list), trace steps, pause and resume.
#pragma once
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

struct CpuRegs {
    uint16_t pc = 0, sp = 0;
    uint8_t a = 0, x = 0, y = 0, z = 0, b = 0, p = 0, lastOp = 0;
};

class XemuMonitor {
public:
    ~XemuMonitor() { stop(); }

    // start xemu with the program (a free port when port is 0), connected
    bool launch(const std::string& xemu, const std::string& prg, int port, bool headless, bool fast,
                const std::vector<std::string>& extra, std::string& err);
    // connect to a monitor already listening (127.0.0.1:port)
    bool connect(int port, int timeoutMs, std::string& err);
    void stop();                       // close the socket, end the xemu we started
    bool running() const;              // the xemu we started still runs

    // a command and its reply (up to the monitor's "." prompt); "" on a timeout
    std::string command(const std::string& cmd, int timeoutMs = 3000);
    bool regs(CpuRegs& r);
    // memory: an address below $10000 is the CPU's view (MAP, overlays mapped
    // in), above it a 28-bit address
    bool read(uint32_t addr, size_t n, std::vector<uint8_t>& out);
    bool write(uint32_t addr, const std::vector<uint8_t>& bytes);

    bool hasBreakpointList();          // the 'k' command (stops before the opcode)
    bool addBreak(uint16_t pc);
    bool removeBreak(uint16_t pc);
    bool clearBreaks();

    bool pause();                      // trace mode: the emulation stops
    bool resume();                     // out of trace mode
    bool step(CpuRegs& r);             // one opcode (paused)
    // after resume(): wait until a breakpoint stops it (its registers);
    // false on a timeout (timeoutMs < 0: none) or when xemu ends
    bool waitStop(CpuRegs& r, int timeoutMs);
    bool paused() const { return paused_; }

private:
    int fd_ = -1;
    pid_t pid_ = -1;
    bool paused_ = false;
    std::string buf_;                  // received, not yet consumed
    bool fill(int timeoutMs);          // more of the socket into buf_
    static bool parseRegs(const std::string& text, CpuRegs& r);
};
