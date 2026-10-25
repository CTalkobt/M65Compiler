// dbg45 — a source-level debugger for cc45 programs in xemu (#202)
//
//   dbg45 [options] program.prg          (program.dbg next to it: ln45 -g / cc45 -g)
//
// Commands as gdb names them: run, start, continue, next, step, finish,
// stepi, break, delete, info breakpoints|locals|args|registers, backtrace,
// frame, up, down, print, x, list, kill, quit. --dap speaks the Debug
// Adapter Protocol on stdin/stdout instead (an editor's debugger: VS Code).
#include "Logger.hpp"
#include "Dbg45.hpp"
#include "Version.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

int dbg45Dap(Dbg45& d);   // Dbg45Dap.cpp

static void usage(std::ostream& o) {
    o << "Usage: dbg45 [options] program.prg\n"
         "A source-level debugger for cc45 programs, in xemu (program.dbg: link with ln45 -g, or cc45 -g).\n"
         "Options:\n"
         "  --xemu <path>      The xemu to run (default $DBG45_XEMU, else xemu-xmega65; it needs the\n"
         "                     monitor's breakpoint list, the 'k' command)\n"
         "  --xemu-arg <arg>   Pass an option to xemu (again for more: --xemu-arg -8 --xemu-arg x.d81)\n"
         "  --headless         Run xemu without a window\n"
         "  --fast             Run xemu as fast as it can (-sleepless)\n"
         "  --timeout <s>      Stop waiting for a breakpoint after s seconds (pause there)\n"
         "  -ex <command>      Run a command first (again for more)\n"
         "  -x <file>          Run the commands in a file first\n"
         "  --batch            Quit after the -ex / -x commands\n"
         "  --dap              Speak the Debug Adapter Protocol on stdin/stdout (for an editor)\n"
         "  -v ... -vvvv       Log to stderr ([dbg45:INFO] ...): xemu's start and the program (-v),\n"
         "                     each monitor command (-vv) and its reply (-vvv); -v0 off\n"
         "  -V, --version      Version\n"
         "  -h, --help         This help\n"
         "Commands: run, start, continue (c), next (n), step (s), finish, stepi (si), break (b) LOC,\n"
         "  delete (d) [N], info breakpoints|locals|args|registers, backtrace (bt), frame (f) N, up,\n"
         "  down, print (p) EXPR, x ADDR [N], list (l), kill, quit (q), help.\n"
         "  LOC: a function, file:line, a line of the current file, or *ADDR.\n"
         "  EXPR: a variable, *e, &e, e.member, e->member, e[i], numbers ($hex).\n";
}

namespace {
struct Cli {
    Dbg45& d;
    size_t frame = 0;
    const DbgFunction* lastFn = nullptr;
    explicit Cli(Dbg45& d_) : d(d_) {}

    void showLine(const Dbg45Frame& f) {
        if (!f.line) { printf("$%04x in %s\n", f.pc, f.function ? d.dbg.cName(f.function->name).c_str() : "??"); return; }
        printf("%d\t%s\n", f.line->line, d.sourceLine(f.line->file, f.line->line).c_str());
    }

    void report(Dbg45::Stop s, const std::string& err) {
        frame = 0;
        switch (s) {
            case Dbg45::ERROR: printf("error: %s\n", err.c_str()); return;
            case Dbg45::EXITED: printf("[program exited with status %d]\n", d.exitStatus()); lastFn = nullptr; return;
            case Dbg45::TIMEOUT: printf("[no stop within the timeout: paused]\n"); break;
            case Dbg45::PAUSED: printf("[paused]\n"); break;
            default: break;
        }
        auto bt = d.backtrace();
        if (bt.empty()) return;
        const Dbg45Frame& f = bt[0];
        if (s == Dbg45::BREAKPOINT) printf("\nBreakpoint %d, %s\n", d.lastBreakpoint(), d.location(f).c_str());
        else if (f.function != lastFn || s == Dbg45::TIMEOUT || s == Dbg45::PAUSED) printf("%s\n", d.location(f).c_str());
        lastFn = f.function;
        showLine(f);
    }

    // a command; false to quit
    bool command(const std::string& line) {
        std::istringstream ws(line);
        std::string cmd, rest;
        ws >> cmd;
        std::getline(ws, rest);
        size_t b = rest.find_first_not_of(" \t");
        rest = b == std::string::npos ? "" : rest.substr(b);
        std::string err;
        if (cmd.empty()) return true;
        if (cmd == "q" || cmd == "quit") return false;
        if (cmd == "h" || cmd == "help") { usage(std::cout); return true; }
        if (cmd == "r" || cmd == "run") { report(d.run(false, err), err); return true; }
        if (cmd == "start") { report(d.run(true, err), err); return true; }
        if (cmd == "c" || cmd == "continue") { report(d.cont(err), err); return true; }
        if (cmd == "n" || cmd == "next" || cmd == "s" || cmd == "step" || cmd == "si" || cmd == "stepi") {
            int count = rest.empty() ? 1 : std::max(1, atoi(rest.c_str()));
            Dbg45::Stop s = Dbg45::STEP;
            for (int i = 0; i < count && s == Dbg45::STEP; i++)
                s = cmd[0] == 'n' ? d.stepLine(true, err) : (cmd == "si" || cmd == "stepi") ? d.stepInstruction(err)
                                                                                            : d.stepLine(false, err);
            if (s == Dbg45::STEP && (cmd == "si" || cmd == "stepi")) {
                printf("$%04x\t", d.regs.pc);
                auto bt = d.backtrace();
                if (!bt.empty()) showLine(bt[0]); else printf("\n");
                return true;
            }
            report(s, err);
            return true;
        }
        if (cmd == "finish") {
            auto bt = d.backtrace();
            if (!bt.empty() && bt[0].function) printf("Run till exit from %s\n", d.location(bt[0]).c_str());
            report(d.finish(err), err);
            return true;
        }
        if (cmd == "b" || cmd == "break") {
            int id = d.addBreakpoint(rest, err);
            if (!id) { printf("error: %s\n", err.c_str()); return true; }
            const auto& bp = d.breakpoints[id];
            printf("Breakpoint %d at $%04x: %s\n", id, bp.addrs[0], bp.where.c_str());
            return true;
        }
        if (cmd == "d" || cmd == "delete") {
            if (rest.empty()) d.breakpoints.clear();
            else if (!d.deleteBreakpoint(atoi(rest.c_str()))) printf("error: no breakpoint %s\n", rest.c_str());
            return true;
        }
        if (cmd == "info" || cmd == "i") {
            if (rest.rfind("b", 0) == 0) {
                if (d.breakpoints.empty()) printf("No breakpoints.\n");
                for (const auto& [id, bp] : d.breakpoints) {
                    printf("%-3d %s at", id, bp.where.c_str());
                    for (auto a : bp.addrs) printf(" $%04x", a);
                    printf("\n");
                }
                return true;
            }
            if (!d.started()) { printf("error: the program is not running\n"); return true; }
            if (rest.rfind("r", 0) == 0) {
                const CpuRegs& r = d.regs;
                printf("pc $%04x  a $%02x  x $%02x  y $%02x  z $%02x  b $%02x  sp $%04x  p $%02x %c%c%c%c%c%c%c%c\n",
                       r.pc, r.a, r.x, r.y, r.z, r.b, r.sp, r.p, r.p & 0x80 ? 'N' : '-', r.p & 0x40 ? 'V' : '-',
                       r.p & 0x20 ? 'E' : '-', r.p & 0x10 ? 'B' : '-', r.p & 8 ? 'D' : '-', r.p & 4 ? 'I' : '-',
                       r.p & 2 ? 'Z' : '-', r.p & 1 ? 'C' : '-');
                return true;
            }
            if (rest.rfind("l", 0) == 0 || rest.rfind("a", 0) == 0) {
                auto vs = d.locals(frame, rest[0] == 'a');
                if (vs.empty()) printf(rest[0] == 'a' ? "No arguments.\n" : "No locals.\n");
                for (const auto& [v, val] : vs) printf("%s = %s\n", v->name.c_str(), val.c_str());
                return true;
            }
            if (rest.rfind("f", 0) == 0) {
                auto bt = d.backtrace();
                if (frame < bt.size()) printf("#%zu  %s\n", frame, d.location(bt[frame]).c_str());
                return true;
            }
            printf("error: info breakpoints|locals|args|registers|frame\n");
            return true;
        }
        if (cmd == "bt" || cmd == "backtrace" || cmd == "where") {
            if (!d.started()) { printf("error: the program is not running\n"); return true; }
            auto bt = d.backtrace();
            for (size_t i = 0; i < bt.size(); i++) printf("#%zu  %s\n", i, d.location(bt[i]).c_str());
            return true;
        }
        if (cmd == "f" || cmd == "frame" || cmd == "up" || cmd == "down") {
            auto bt = d.backtrace();
            size_t n = frame;
            if (cmd == "up") n = frame + (rest.empty() ? 1 : atoi(rest.c_str()));
            else if (cmd == "down") n = frame >= (size_t)(rest.empty() ? 1 : atoi(rest.c_str())) ? frame - (rest.empty() ? 1 : atoi(rest.c_str())) : 0;
            else if (!rest.empty()) n = (size_t)atoi(rest.c_str());
            if (n >= bt.size()) { printf("error: no frame %zu\n", n); return true; }
            frame = n;
            printf("#%zu  %s\n", frame, d.location(bt[frame]).c_str());
            showLine(bt[frame]);
            return true;
        }
        if (cmd == "p" || cmd == "print") {
            Dbg45Value v;
            if (!d.evaluate(rest, frame, v, err)) printf("error: %s\n", err.c_str());
            else printf("%s = %s\n", rest.c_str(), d.format(v).c_str());
            return true;
        }
        if (cmd == "x") {
            std::istringstream as(rest);
            std::string a;
            int n = 16;
            as >> a >> n;
            Dbg45Value v;
            long addr;
            if (!a.empty() && a[0] == '$') addr = std::stol(a.substr(1), nullptr, 16);
            else if (d.evaluate(a, frame, v, err)) addr = v.type[0] == '*' ? (long)(v.bytes[0] | (v.bytes[1] << 8)) : v.addr;
            else { printf("error: %s\n", err.c_str()); return true; }
            std::vector<uint8_t> m;
            if (addr < 0 || !d.readMemory((uint32_t)addr, (size_t)std::max(1, n), m)) { printf("error: unreadable\n"); return true; }
            for (size_t i = 0; i < m.size(); i += 16) {
                printf("$%04lx:", addr + (long)i);
                for (size_t k = i; k < m.size() && k < i + 16; k++) printf(" %02x", m[k]);
                printf("\n");
            }
            return true;
        }
        if (cmd == "l" || cmd == "list") {
            auto bt = d.backtrace();
            if (bt.empty() || frame >= bt.size() || !bt[frame].line) { printf("error: no source here\n"); return true; }
            int at = bt[frame].line->line;
            for (int l = std::max(1, at - 4); l <= at + 5; l++) {
                std::string s = d.sourceLine(bt[frame].line->file, l);
                printf("%s%d\t%s\n", l == at ? "=>" : "  ", l, s.c_str());
            }
            return true;
        }
        if (cmd == "kill") { d.kill(); printf("[xemu ended]\n"); return true; }
        printf("error: unknown command '%s' (help)\n", cmd.c_str());
        return true;
    }
};
}  // namespace

int main(int argc, char** argv) {
    Logger::setTool("dbg45");
    int verbosity = 0;
    Dbg45 d;
    if (const char* x = getenv("DBG45_XEMU")) d.opt.xemu = x;
    std::vector<std::string> cmds;
    std::string prg;
    bool batch = false, dap = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { usage(std::cout); return 0; }
        if (a == "-V" || a == "--version") { std::cout << suiteVersionString("dbg45") << std::endl; return 0; }
        if (a == "--xemu" && i + 1 < argc) d.opt.xemu = argv[++i];
        else if (a == "--xemu-arg" && i + 1 < argc) d.opt.xemuArgs.push_back(argv[++i]);
        else if (a == "--headless") d.opt.headless = true;
        else if (a == "--fast") d.opt.fast = true;
        else if (a == "--timeout" && i + 1 < argc) d.opt.timeoutMs = (int)(atof(argv[++i]) * 1000);
        else if (a == "-ex" && i + 1 < argc) cmds.push_back(argv[++i]);
        else if (a == "-x" && i + 1 < argc) {
            std::ifstream in(argv[++i]);
            if (!in) { std::cerr << "dbg45: cannot read " << argv[i] << std::endl; return 1; }
            for (std::string l; std::getline(in, l); ) if (!l.empty() && l[0] != '#') cmds.push_back(l);
        }
        else if (a == "--batch") batch = true;
        else if (a == "--dap") dap = true;
        else if (Logger::parseFlag(a, verbosity)) Logger::setLevel(verbosity);   // -v ... -vvvv, -v0
        else if (!a.empty() && a[0] == '-') { std::cerr << "dbg45: unknown option " << a << std::endl; return 1; }
        else prg = a;
    }
    if (dap) return dbg45Dap(d);
    if (prg.empty()) { usage(std::cerr); return 1; }
    std::string err;
    if (!d.open(prg, err)) { std::cerr << "dbg45: " << err << std::endl; return 1; }
    setvbuf(stdout, nullptr, _IOLBF, 0);
    Cli cli(d);
    for (const auto& c : cmds) {
        if (batch) printf("(dbg45) %s\n", c.c_str());
        if (!cli.command(c)) { d.kill(); return 0; }
    }
    if (batch) { d.kill(); return 0; }
    bool tty = isatty(0);
    std::string line, last;
    for (;;) {
        if (tty) { printf("(dbg45) "); fflush(stdout); }
        if (!std::getline(std::cin, line)) break;
        if (line.empty() && tty) line = last;   // (RETURN repeats the last command)
        last = line;
        if (!cli.command(line)) break;
    }
    d.kill();
    return 0;
}
