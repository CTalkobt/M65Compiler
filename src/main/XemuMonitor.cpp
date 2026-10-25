// XemuMonitor.cpp — xemu's monitor socket for dbg45 (XemuMonitor.hpp)
#include "Logger.hpp"
#include "XemuMonitor.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using Clock = std::chrono::steady_clock;

static int freePort() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0;
    int port = 0;
    if (bind(s, (sockaddr*)&a, sizeof a) == 0) {
        socklen_t l = sizeof a;
        getsockname(s, (sockaddr*)&a, &l);
        port = ntohs(a.sin_port);
    }
    close(s);
    return port;
}

bool XemuMonitor::launch(const std::string& xemu, const std::string& prg, int port, bool headless, bool fast,
                         const std::vector<std::string>& extra, std::string& err) {
    if (port == 0) port = freePort();
    if (port == 0) { err = "no free port for xemu's monitor"; return false; }
    std::vector<std::string> args = {xemu, "-besure", "-prgmode", "65", "-uartmon", ":" + std::to_string(port)};
    if (headless) args.push_back("-headless");
    if (fast) args.push_back("-sleepless");
    for (const auto& e : extra) args.push_back(e);
    args.push_back("-prg");
    args.push_back(prg);
    if (Logger::isEnabled(LogLevel::INFO)) {
        std::string line;
        for (const auto& a : args) line += (line.empty() ? "" : " ") + a;
        LOG_INFO("dbg45", "starting " << line);
    }
    pid_ = fork();
    if (pid_ < 0) { err = std::string("fork: ") + strerror(errno); return false; }
    if (pid_ == 0) {
        int nul = open("/dev/null", O_RDWR);
        if (nul >= 0) { dup2(nul, 0); dup2(nul, 1); dup2(nul, 2); }
        std::vector<char*> av;
        for (auto& a : args) av.push_back(const_cast<char*>(a.c_str()));
        av.push_back(nullptr);
        execvp(av[0], av.data());
        _exit(127);
    }
    if (!connect(port, 15000, err)) {
        stop();
        err = "xemu (" + xemu + ") did not open its monitor: " + err;
        return false;
    }
    return true;
}

bool XemuMonitor::connect(int port, int timeoutMs, std::string& err) {
    auto until = Clock::now() + std::chrono::milliseconds(timeoutMs);
    while (Clock::now() < until) {
        if (pid_ > 0 && !running()) { err = "xemu ended"; return false; }
        int s = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons((uint16_t)port);
        if (::connect(s, (sockaddr*)&a, sizeof a) == 0) {
            fd_ = s;
            LOG_INFO("dbg45", "connected to xemu's monitor at port " << port);
            return true;
        }
        close(s);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    err = "no monitor at port " + std::to_string(port);
    return false;
}

void XemuMonitor::stop() {
    if (fd_ >= 0) { close(fd_); fd_ = -1; }
    if (pid_ > 0) {
        kill(pid_, SIGTERM);
        for (int i = 0; i < 100 && running(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (running()) kill(pid_, SIGKILL);
        waitpid(pid_, nullptr, 0);
        pid_ = -1;
    }
}

bool XemuMonitor::running() const {
    if (pid_ <= 0) return fd_ >= 0;
    int st;
    return waitpid(pid_, &st, WNOHANG) == 0;
}

bool XemuMonitor::fill(int timeoutMs) {
    if (fd_ < 0) return false;
    pollfd p{fd_, POLLIN, 0};
    if (poll(&p, 1, timeoutMs) <= 0) return false;
    char b[4096];
    ssize_t n = recv(fd_, b, sizeof b, 0);
    if (n <= 0) { close(fd_); fd_ = -1; return false; }
    buf_.append(b, (size_t)n);
    return true;
}

// The reply ends at the prompt: a line that is only "."
std::string XemuMonitor::command(const std::string& cmd, int timeoutMs) {
    if (fd_ < 0) return "";
    std::string line = cmd + "\n";
    LOG_DEBUG("dbg45", "monitor> " << cmd);
    if (send(fd_, line.data(), line.size(), MSG_NOSIGNAL) != (ssize_t)line.size()) return "";
    auto until = Clock::now() + std::chrono::milliseconds(timeoutMs);
    // (the echo of the command first, then its output)
    size_t from = buf_.find(cmd);
    for (;;) {
        if (from == std::string::npos) from = buf_.find(cmd);
        if (from != std::string::npos) {
            size_t p = from;
            for (;;) {
                size_t nl = buf_.find('\n', p);
                if (nl == std::string::npos) break;
                size_t q = nl + 1;
                size_t e = buf_.find('\n', q);
                std::string next = buf_.substr(q, e == std::string::npos ? std::string::npos : e - q);
                while (!next.empty() && (next.back() == '\r' || next.back() == ' ')) next.pop_back();
                if (next == "." && e != std::string::npos) {
                    std::string out = buf_.substr(from, q - from);
                    buf_.erase(0, e + 1);
                    // (the reply, after the command's echo)
                    if (Logger::isEnabled(LogLevel::DEEP)) {
                        size_t r = out.find('\n');
                        for (size_t b = r == std::string::npos ? out.size() : r + 1; b < out.size(); ) {
                            size_t eol = out.find('\n', b);
                            std::string l = out.substr(b, eol == std::string::npos ? std::string::npos : eol - b);
                            while (!l.empty() && l.back() == '\r') l.pop_back();
                            if (!l.empty()) LOG_DEEP("dbg45", "monitor< " << l);
                            b = eol == std::string::npos ? out.size() : eol + 1;
                        }
                    }
                    return out;
                }
                p = q;
            }
        }
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(until - Clock::now()).count();
        if (left <= 0 || !fill((int)left)) {
            if (left <= 0 || fd_ < 0) {
                LOG_DEBUG("dbg45", "monitor: no reply to " << cmd << (fd_ < 0 ? " (connection closed)" : " (timeout)"));
                return "";
            }
        }
    }
}

// "PC   A  X  Y  Z  B  SP   MAPH MAPL LAST-OP     P ..." then the values
bool XemuMonitor::parseRegs(const std::string& text, CpuRegs& r) {
    size_t h = text.find("PC   A  X");
    if (h == std::string::npos) return false;
    size_t nl = text.find('\n', h);
    if (nl == std::string::npos) return false;
    unsigned pc, a, x, y, z, b, sp, maph, mapl, op, p;
    if (sscanf(text.c_str() + nl + 1, "%x %x %x %x %x %x %x %x %x %x %x", &pc, &a, &x, &y, &z, &b, &sp, &maph, &mapl, &op, &p) < 11)
        return false;
    r.pc = (uint16_t)pc; r.a = (uint8_t)a; r.x = (uint8_t)x; r.y = (uint8_t)y; r.z = (uint8_t)z;
    r.b = (uint8_t)b; r.sp = (uint16_t)sp; r.lastOp = (uint8_t)op; r.p = (uint8_t)p;
    return true;
}

bool XemuMonitor::regs(CpuRegs& r) { return parseRegs(command("r"), r); }

bool XemuMonitor::read(uint32_t addr, size_t n, std::vector<uint8_t>& out) {
    out.clear();
    while (out.size() < n) {
        uint32_t a = addr + (uint32_t)out.size();
        // (the CPU's view of bank 0: $777xxxx)
        uint32_t ma = a < 0x10000 ? 0x7770000 | a : a;
        char cmd[32];
        snprintf(cmd, sizeof cmd, "m %07x", ma);
        std::string rep = command(cmd);
        size_t c = rep.find(':');
        if (c == std::string::npos) return false;
        size_t c2 = rep.find(':', c + 1);
        if (c2 == std::string::npos) return false;
        for (size_t i = 0; i < 16 && out.size() < n; i++) {
            if (c2 + 1 + 2 * i + 2 > rep.size()) return false;
            out.push_back((uint8_t)std::stoul(rep.substr(c2 + 1 + 2 * i, 2), nullptr, 16));
        }
    }
    return true;
}

bool XemuMonitor::write(uint32_t addr, const std::vector<uint8_t>& bytes) {
    for (size_t i = 0; i < bytes.size(); i += 16) {
        uint32_t a = addr + (uint32_t)i;
        char cmd[96];
        int n = snprintf(cmd, sizeof cmd, "s %07x", a < 0x10000 ? 0x7770000 | a : a);
        for (size_t k = i; k < bytes.size() && k < i + 16; k++) n += snprintf(cmd + n, sizeof cmd - n, " %02x", bytes[k]);
        if (command(cmd).empty()) return false;
    }
    return true;
}

bool XemuMonitor::hasBreakpointList() { return command("k").find("BREAKPOINTS:") != std::string::npos; }

bool XemuMonitor::addBreak(uint16_t pc) {
    char cmd[16];
    snprintf(cmd, sizeof cmd, "k+%04x", pc);
    return !command(cmd).empty();
}

bool XemuMonitor::removeBreak(uint16_t pc) {
    char cmd[16];
    snprintf(cmd, sizeof cmd, "k-%04x", pc);
    return !command(cmd).empty();
}

bool XemuMonitor::clearBreaks() { return !command("k-").empty(); }

bool XemuMonitor::pause() {
    paused_ = !command("t1").empty();
    return paused_;
}

bool XemuMonitor::resume() {
    // (a breakpoint's registers that came while paused are stale)
    buf_.clear();
    if (command("t0").empty()) return false;
    paused_ = false;
    return true;
}

bool XemuMonitor::step(CpuRegs& r) { return parseRegs(command("t", 5000), r); }

bool XemuMonitor::waitStop(CpuRegs& r, int timeoutMs) {
    auto until = Clock::now() + std::chrono::milliseconds(timeoutMs < 0 ? 1000000000 : timeoutMs);
    for (;;) {
        size_t h = buf_.find("PC   A  X");
        if (h != std::string::npos) {
            size_t nl = buf_.find('\n', h);
            size_t end = nl == std::string::npos ? nl : buf_.find('\n', nl + 1);
            if (end != std::string::npos) {
                bool ok = parseRegs(buf_.substr(h, end - h + 1), r);
                buf_.erase(0, end + 1);
                if (ok) { paused_ = true; return true; }
            }
        }
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(until - Clock::now()).count();
        if (left <= 0) return false;
        if (!fill((int)std::min<long long>(left, 200)) && (fd_ < 0 || !running())) return false;
    }
}
