// deploy45 — send a program or a disk image to a MEGA65 or an Ultimate 64 (#145)
//
//   deploy45 m65ether  [options] <prog.prg>     etherload (mega65-tools): the MEGA65's Ethernet
//   deploy45 m65serial [options] <prog.prg>     m65 (mega65-tools): the serial monitor, through
//                                               the JTAG adapter's serial port
//   deploy45 m65mount  [options] <image.d81>    mega65_ftp: the image onto the SD card, mounted;
//                                               --virtual: m65 -d serves it from this computer
//   deploy45 u64       [options] <file>         the Ultimate 64's HTTP API (built in): a PRG
//                                               run or loaded, a CRT, a SID, a disk image mounted
//
// The MEGA65 side runs the mega65-tools programs (as C64IDE does): their
// protocols stay theirs. The Ultimate 64 side is an HTTP/1.1 client on POSIX
// sockets. --dry-run prints what would be run or sent.

#include "Logger.hpp"
#include "Version.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

const char* TOOL = "deploy45";

int fail(const std::string& msg, int code = 1) {
    std::cerr << TOOL << ": " << msg << std::endl;
    return code;
}

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string extOf(const std::string& path) {
    size_t slash = path.find_last_of('/');
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
    return lower(path.substr(dot + 1));
}

std::string baseName(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

const char* envOr(const char* name, const char* dflt) {
    const char* v = std::getenv(name);
    return v && *v ? v : dflt;
}

std::string shellQuote(const std::string& s) {
    if (!s.empty() && s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-./:=@%+,") == std::string::npos)
        return s;
    std::string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    return r + "'";
}

// ---- the mega65-tools programs ---------------------------------------------

// A program: the option's path, else $ENV, else on the PATH, else
// ~/mega65-tools/bin/<name>, ~/mega65-tools/<name>
std::string findTool(const std::string& option, const char* env, const std::string& name) {
    if (!option.empty()) return option;
    if (const char* e = std::getenv(env); e && *e) return e;
    auto exec = [](const std::string& p) { return access(p.c_str(), X_OK) == 0; };
    if (const char* path = std::getenv("PATH")) {
        std::string ps = path;
        size_t from = 0;
        while (from <= ps.size()) {
            size_t to = ps.find(':', from);
            if (to == std::string::npos) to = ps.size();
            std::string dir = ps.substr(from, to - from);
            if (!dir.empty() && exec(dir + "/" + name)) return dir + "/" + name;
            from = to + 1;
        }
    }
    if (const char* home = std::getenv("HOME")) {
        for (const char* sub : {"/mega65-tools/bin/", "/mega65-tools/"}) {
            std::string p = std::string(home) + sub + name;
            if (exec(p)) return p;
        }
    }
    return name;   // (execvp's error names it)
}

std::string commandLine(const std::vector<std::string>& argv) {
    std::string s;
    for (size_t i = 0; i < argv.size(); i++) s += (i ? " " : "") + shellQuote(argv[i]);
    return s;
}

// Runs argv (its output to ours); timeoutSec 0: no limit. The exit status, or
// 124 when it timed out, 127 when it could not be started.
int run(const std::vector<std::string>& argv, int timeoutSec, bool dryRun) {
    if (dryRun) {
        std::cout << commandLine(argv) << std::endl;
        return 0;
    }
    LOG_INFO(TOOL, "running: " << commandLine(argv));
    pid_t pid = fork();
    if (pid < 0) return fail(std::string("fork: ") + std::strerror(errno), 127);
    if (pid == 0) {
        std::vector<char*> av;
        for (auto& a : argv) av.push_back(const_cast<char*>(a.c_str()));
        av.push_back(nullptr);
        execvp(av[0], av.data());
        std::cerr << TOOL << ": cannot run " << argv[0] << ": " << std::strerror(errno)
                  << " (install mega65-tools, or name it with --tool)" << std::endl;
        _exit(127);
    }
    auto start = std::chrono::steady_clock::now();
    for (;;) {
        int status = 0;
        pid_t r = waitpid(pid, &status, timeoutSec > 0 ? WNOHANG : 0);
        if (r == pid) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
        }
        if (r < 0 && errno != EINTR) return fail(std::string("waitpid: ") + std::strerror(errno), 127);
        if (timeoutSec > 0 && std::chrono::steady_clock::now() - start > std::chrono::seconds(timeoutSec)) {
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return fail(argv[0] + " did not finish in " + std::to_string(timeoutSec) +
                        " s (is the MEGA65 on and reachable?); --timeout changes the limit", 124);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// The PRG's mode: $0801 is a C64 program; anything else MEGA65 mode
// (cc45's are $2001). force: "c64", "m65" or "".
bool c64Mode(const std::string& prg, const std::string& force) {
    if (force == "c64") return true;
    if (force == "m65") return false;
    std::ifstream f(prg, std::ios::binary);
    unsigned char b[2] = {0, 0};
    if (!f.read(reinterpret_cast<char*>(b), 2)) return false;
    return (b[0] | (b[1] << 8)) == 0x0801;
}

// An SD card name for an image: 8.3, upper case, letters, digits and _
std::string cardName(const std::string& path) {
    std::string b = baseName(path);
    std::string ext = extOf(b);
    std::string stem = ext.empty() ? b : b.substr(0, b.size() - ext.size() - 1);
    std::string s;
    for (char c : stem) {
        if (std::isalnum((unsigned char)c) || c == '_') s += (char)std::toupper((unsigned char)c);
        if (s.size() == 8) break;
    }
    if (s.empty()) s = "DISK";
    std::string e;
    for (char c : ext) if (std::isalnum((unsigned char)c) && e.size() < 3) e += (char)std::toupper((unsigned char)c);
    return s + "." + (e.empty() ? "D81" : e);
}

// ---- the Ultimate 64's HTTP API --------------------------------------------

struct HttpResult {
    int status = 0;
    std::string body;
    std::string error;
};

// host: name[:port] (default port 80); "http://" in front is allowed
HttpResult http(const std::string& hostIn, const std::string& method, const std::string& path,
                const std::string& password, const std::string& body, int timeoutSec) {
    HttpResult res;
    std::string host = hostIn;
    if (host.rfind("http://", 0) == 0) host = host.substr(7);
    while (!host.empty() && host.back() == '/') host.pop_back();
    std::string port = "80";
    size_t colon = host.rfind(':');
    if (colon != std::string::npos && host.find(']') == std::string::npos && host.find(':') == colon) {
        port = host.substr(colon + 1);
        host = host.substr(0, colon);
    }
    if (host.size() > 2 && host.front() == '[' && host.back() == ']') host = host.substr(1, host.size() - 2);

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* ai = nullptr;
    if (int e = getaddrinfo(host.c_str(), port.c_str(), &hints, &ai); e != 0) {
        res.error = "cannot resolve " + host + ": " + gai_strerror(e);
        return res;
    }
    int fd = -1;
    std::string lastErr = "no address";
    for (addrinfo* p = ai; p; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        int r = connect(fd, p->ai_addr, p->ai_addrlen);
        if (r < 0 && errno == EINPROGRESS) {
            pollfd pf{fd, POLLOUT, 0};
            r = poll(&pf, 1, timeoutSec * 1000);
            int soerr = 0;
            socklen_t len = sizeof soerr;
            if (r == 1 && getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len) == 0 && soerr == 0) r = 0;
            else {
                lastErr = r == 0 ? "timed out" : std::strerror(soerr ? soerr : errno);
                r = -1;
            }
        } else if (r < 0) {
            lastErr = std::strerror(errno);
        }
        if (r == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(ai);
    if (fd < 0) {
        res.error = "cannot connect to " + host + ":" + port + ": " + lastErr;
        return res;
    }

    std::string req = method + " /" + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n" +
                      "User-Agent: deploy45\r\n";
    if (!password.empty()) req += "X-Password: " + password + "\r\n";
    if (method == "POST" || method == "PUT") {
        if (!body.empty()) req += "Content-Type: application/octet-stream\r\n";
        req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }
    req += "\r\n";
    std::string out = req + body;
    size_t sent = 0;
    while (sent < out.size()) {
        pollfd pf{fd, POLLOUT, 0};
        if (poll(&pf, 1, timeoutSec * 1000) != 1) { res.error = "timed out sending"; close(fd); return res; }
        ssize_t n = send(fd, out.data() + sent, out.size() - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            res.error = std::string("send: ") + std::strerror(errno);
            close(fd);
            return res;
        }
        sent += (size_t)n;
    }
    std::string in;
    char buf[4096];
    for (;;) {
        pollfd pf{fd, POLLIN, 0};
        int r = poll(&pf, 1, timeoutSec * 1000);
        if (r != 1) { if (in.empty()) res.error = "no answer (timed out)"; break; }
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        if (n <= 0) break;
        in.append(buf, (size_t)n);
    }
    close(fd);
    if (in.empty()) {
        if (res.error.empty()) res.error = "no answer";
        return res;
    }
    // HTTP/1.x NNN ...
    if (in.rfind("HTTP/", 0) != 0 || in.size() < 12) {
        res.error = "not an HTTP answer";
        return res;
    }
    res.status = std::atoi(in.c_str() + in.find(' ') + 1);
    size_t hdrEnd = in.find("\r\n\r\n");
    std::string headers = lower(in.substr(0, hdrEnd == std::string::npos ? in.size() : hdrEnd));
    res.body = hdrEnd == std::string::npos ? "" : in.substr(hdrEnd + 4);
    // (a chunked body: its chunks joined)
    if (headers.find("transfer-encoding: chunked") != std::string::npos) {
        std::string joined;
        size_t p = 0;
        while (p < res.body.size()) {
            size_t eol = res.body.find("\r\n", p);
            if (eol == std::string::npos) break;
            size_t n = std::strtoul(res.body.substr(p, eol - p).c_str(), nullptr, 16);
            if (n == 0) break;
            joined += res.body.substr(eol + 2, n);
            p = eol + 2 + n + 2;
        }
        res.body = joined;
    }
    return res;
}

// ---- the commands ---------------------------------------------------------

struct Options {
    std::string file;
    std::string host;          // -i
    std::string device;        // -l
    std::string speed;         // -s
    std::string mode;          // c64 / m65 / ""
    std::string tool;          // --tool
    std::string password;      // u64
    std::string drive = "a";   // u64 mount
    std::string name;          // m65mount: the SD card name
    std::string onCard;        // m65mount --on-card NAME
    std::string song;          // u64 sid
    int timeout = -1;
    bool run = true;
    bool reset = true;
    bool dryRun = false;
    bool virtualDisk = false;
    bool info = false;
    bool resetOnly = false;
    bool ether = false;        // m65mount --ether
};

int cmdM65Ether(const Options& o) {
    if (o.file.empty()) return fail("m65ether: no program (deploy45 m65ether [options] prog.prg)");
    if (!o.dryRun && access(o.file.c_str(), R_OK) != 0) return fail("cannot read " + o.file);
    std::vector<std::string> a{findTool(o.tool, "ETHERLOAD", "etherload")};
    std::string host = o.host.empty() ? envOr("M65_HOST", "") : o.host;
    if (!host.empty()) { a.push_back("-i"); a.push_back(host); }
    if (o.reset) a.push_back(c64Mode(o.file, o.mode) ? "-4" : "-5");
    if (o.run) a.push_back("-r");
    a.push_back(o.file);
    return run(a, o.timeout < 0 ? 60 : o.timeout, o.dryRun);
}

void serialArgs(std::vector<std::string>& a, const Options& o) {
    std::string dev = o.device.empty() ? envOr("M65_DEVICE", "") : o.device;
    if (!dev.empty()) { a.push_back("-l"); a.push_back(dev); }
    std::string speed = o.speed.empty() ? envOr("M65_SPEED", "") : o.speed;
    if (!speed.empty()) { a.push_back("-s"); a.push_back(speed); }
}

int cmdM65Serial(const Options& o) {
    if (o.file.empty()) return fail("m65serial: no program (deploy45 m65serial [options] prog.prg)");
    if (!o.dryRun && access(o.file.c_str(), R_OK) != 0) return fail("cannot read " + o.file);
    std::vector<std::string> a{findTool(o.tool, "M65", "m65")};
    serialArgs(a, o);
    if (o.reset) a.push_back("-F");
    if (c64Mode(o.file, o.mode)) a.push_back("-4");
    if (o.run) a.push_back("-r");
    a.push_back(o.file);
    return run(a, o.timeout < 0 ? 60 : o.timeout, o.dryRun);
}

int cmdM65Mount(const Options& o) {
    // Ethernet with -i or --ether (or $M65_HOST and no serial port named), else serial
    std::string host = o.host.empty() ? envOr("M65_HOST", "") : o.host;
    bool ether = o.ether || !o.host.empty() || (o.device.empty() && !host.empty() && !std::getenv("M65_DEVICE"));
    if (o.virtualDisk) {
        // m65 -d: the MEGA65's drive reads and writes the local image through m65, while it runs
        if (o.file.empty()) return fail("m65mount --virtual: no image");
        if (ether) return fail("m65mount --virtual works through the serial monitor (-l), not Ethernet");
        if (!o.dryRun && access(o.file.c_str(), R_OK | W_OK) != 0) return fail("cannot read and write " + o.file);
        std::vector<std::string> a{findTool(o.tool, "M65", "m65")};
        serialArgs(a, o);
        a.push_back("-d");
        a.push_back(o.file);
        if (!o.dryRun) std::cerr << TOOL << ": serving " << o.file << " as the MEGA65's drive; Ctrl-C ends it" << std::endl;
        return run(a, o.timeout < 0 ? 0 : o.timeout, o.dryRun);
    }
    std::string name;
    std::vector<std::string> cmds;
    if (!o.onCard.empty()) {
        name = o.onCard;
    } else {
        if (o.file.empty()) return fail("m65mount: no image (deploy45 m65mount [options] image.d81)");
        std::string ext = extOf(o.file);
        if (ext != "d81" && ext != "d65") return fail("m65mount: the MEGA65 mounts .d81 (and .d65) images, not ." + ext);
        if (!o.dryRun && access(o.file.c_str(), R_OK) != 0) return fail("cannot read " + o.file);
        if (o.file.find(' ') != std::string::npos)
            return fail("m65mount: mega65_ftp takes no spaces in a file name: copy " + o.file + " to a name without them");
        name = o.name.empty() ? cardName(o.file) : o.name;
        cmds.push_back("put " + o.file + " " + name);
    }
    cmds.push_back("mount " + name);
    cmds.push_back("exit");
    std::vector<std::string> a{findTool(o.tool, "MEGA65_FTP", "mega65_ftp")};
    if (ether) {
        a.push_back("-e");
        if (!host.empty()) { a.push_back("-i"); a.push_back(host); }
    } else {
        serialArgs(a, o);
    }
    for (auto& c : cmds) { a.push_back("-c"); a.push_back(c); }
    return run(a, o.timeout < 0 ? 0 : o.timeout, o.dryRun);
}

int cmdU64(const Options& o) {
    std::string host = o.host.empty() ? envOr("U64_HOST", "") : o.host;
    if (host.empty()) return fail("u64: no host (-i HOST or U64_HOST)");
    std::string password = o.password.empty() ? envOr("U64_PASSWORD", "") : o.password;
    int timeout = o.timeout < 0 ? 30 : o.timeout;
    std::string method = "POST", path, body, what;
    if (o.info) {
        method = "GET"; path = "v1/info"; what = "info";
    } else if (o.resetOnly && o.file.empty()) {
        method = "PUT"; path = "v1/machine:reset"; what = "reset";
    } else {
        if (o.file.empty()) return fail("u64: no file (deploy45 u64 [options] prog.prg|cart.crt|tune.sid|disk.d64)");
        if (!readFile(o.file, body)) return fail("cannot read " + o.file);
        std::string ext = extOf(o.file);
        if (ext == "prg") {
            path = o.run ? "v1/runners:run_prg" : "v1/runners:load_prg";
            what = o.run ? "run" : "loaded";
        } else if (ext == "crt") {
            path = "v1/runners:run_crt"; what = "cartridge started";
        } else if (ext == "sid") {
            path = "v1/runners:sidplay" + (o.song.empty() ? "" : "?songnr=" + o.song); what = "playing";
        } else if (ext == "d64" || ext == "d71" || ext == "d81" || ext == "g64" || ext == "g71") {
            path = "v1/drives/" + o.drive + ":mount?type=" + ext + "&mode=readwrite"; what = "mounted on drive " + o.drive;
        } else {
            return fail("u64: ." + ext + " is not a PRG, CRT, SID or disk image (d64, d71, d81, g64, g71)");
        }
    }
    if (o.dryRun) {
        std::cout << method << " http://" << host << "/" << path
                  << (body.empty() ? "" : " (" + std::to_string(body.size()) + " bytes)")
                  << (password.empty() ? "" : " X-Password") << std::endl;
        return 0;
    }
    LOG_INFO(TOOL, method << " " << host << "/" << path << " " << body.size() << " bytes");
    HttpResult r = http(host, method, path, password, body, timeout);
    if (!r.error.empty()) return fail("u64: " + r.error);
    if (r.status == 403) return fail("u64: refused (403): wrong network password (--password or U64_PASSWORD)");
    if (r.status < 200 || r.status >= 300) {
        std::string msg = r.body.substr(0, 300);
        return fail("u64: " + path + ": HTTP " + std::to_string(r.status) + (msg.empty() ? "" : ": " + msg));
    }
    if (o.info) std::cout << r.body << (r.body.empty() || r.body.back() == '\n' ? "" : "\n");
    else std::cout << TOOL << ": " << (o.file.empty() ? "" : baseName(o.file) + ": ") << what << std::endl;
    return 0;
}

void usage() {
    std::cout <<
        "Usage: deploy45 <command> [options] <file>\n"
        "\n"
        "Commands:\n"
        "  m65ether  <prog.prg>   load (and run) on a MEGA65 over Ethernet (etherload)\n"
        "  m65serial <prog.prg>   load (and run) through the JTAG adapter's serial port (m65)\n"
        "  m65mount  <image.d81>  put a disk image on the MEGA65's SD card and mount it\n"
        "                         (mega65_ftp, serial or -i Ethernet); --virtual: serve it from\n"
        "                         this computer while deploy45 runs (m65 -d, serial)\n"
        "  u64       <file>       an Ultimate 64 over its HTTP API: a PRG run (or --no-run:\n"
        "                         loaded), a CRT started, a SID played, a d64/d71/d81/g64\n"
        "                         mounted; --info, --reset\n"
        "\n"
        "Options:\n"
        "  -i, --host HOST        the MEGA65's address (etherload / mega65_ftp -i; default\n"
        "                         $M65_HOST, else discovered) or the U64's (name[:port];\n"
        "                         $U64_HOST)\n"
        "  -l, --device DEV       the serial port (/dev/ttyUSB1; default $M65_DEVICE, else\n"
        "                         what m65 finds)\n"
        "  -s, --speed BPS        the serial speed ($M65_SPEED; m65's default otherwise)\n"
        "  --c64, --m65           the mode to start the program in (default: C64 for a\n"
        "                         $0801 program, MEGA65 otherwise)\n"
        "  --no-run               load only\n"
        "  --no-reset             no reset before loading (m65serial, m65ether)\n"
        "  --name NAME.D81        m65mount: the name on the SD card (default: the file's,\n"
        "                         8.3 upper case)\n"
        "  --on-card NAME.D81     m65mount: mount an image already on the SD card\n"
        "  --virtual              m65mount: m65 -d, the image stays on this computer\n"
        "  --ether                m65mount: over Ethernet (mega65_ftp -e) without -i: the\n"
        "                         MEGA65 discovered\n"
        "  --password PW          u64: the network password ($U64_PASSWORD)\n"
        "  --drive a|b            u64: the drive a disk image is mounted on (a)\n"
        "  --song N               u64: a SID's song number\n"
        "  --info                 u64: the device's information (v1/info)\n"
        "  --reset                u64: reset the machine (with no file)\n"
        "  --tool PATH            the mega65-tools program to run (or $ETHERLOAD, $M65,\n"
        "                         $MEGA65_FTP; else the PATH, ~/mega65-tools)\n"
        "  --timeout SEC          give up after SEC seconds (0: never; default 60 for\n"
        "                         loading, 30 for u64, none for m65mount)\n"
        "  -n, --dry-run          print the command or request, do nothing\n"
        "  -v ... -vvvv           log on stderr\n"
        "  -V, --version          the version\n"
        "  -h, --help             this help\n";
}

}  // namespace

int main(int argc, char** argv) {
    Logger::setTool(TOOL);
    argc = Logger::takeFlags(argc, argv);
    if (argc < 2) { usage(); return 1; }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help" || cmd == "-?") { usage(); return 0; }
    if (cmd == "-V" || cmd == "--version") { std::cout << suiteVersionString(TOOL) << std::endl; return 0; }

    Options o;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        auto value = [&](const char* what) -> std::string {
            if (i + 1 >= argc) { fail(std::string(a) + " needs " + what); std::exit(1); }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (a == "-i" || a == "--host") o.host = value("a host");
        else if (a == "-l" || a == "--device") o.device = value("a serial port");
        else if (a == "-s" || a == "--speed") o.speed = value("a speed");
        else if (a == "--c64") o.mode = "c64";
        else if (a == "--m65") o.mode = "m65";
        else if (a == "--no-run") o.run = false;
        else if (a == "--no-reset") o.reset = false;
        else if (a == "--reset") o.resetOnly = true;
        else if (a == "--ether") o.ether = true;
        else if (a == "--name") o.name = value("a name");
        else if (a == "--on-card") o.onCard = value("a name");
        else if (a == "--virtual") o.virtualDisk = true;
        else if (a == "--password") o.password = value("a password");
        else if (a == "--drive") o.drive = lower(value("a or b"));
        else if (a == "--song") o.song = value("a number");
        else if (a == "--info") o.info = true;
        else if (a == "--tool") o.tool = value("a path");
        else if (a == "--timeout") o.timeout = std::atoi(value("seconds").c_str());
        else if (a == "-n" || a == "--dry-run") o.dryRun = true;
        else if (!a.empty() && a[0] == '-') return fail("unknown option " + a + " (deploy45 --help)");
        else if (o.file.empty()) o.file = a;
        else return fail("one file at a time (" + o.file + ", " + a + ")");
    }
    if (o.drive != "a" && o.drive != "b") return fail("--drive is a or b");

    if (cmd == "m65ether") return cmdM65Ether(o);
    if (cmd == "m65serial") return cmdM65Serial(o);
    if (cmd == "m65mount") return cmdM65Mount(o);
    if (cmd == "u64") return cmdU64(o);
    return fail("unknown command " + cmd + " (m65ether, m65serial, m65mount, u64)");
}
