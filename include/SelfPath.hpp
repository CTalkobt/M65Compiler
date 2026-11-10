// SelfPath.hpp — the running program's own path, on every platform (#503)
//
// The tools find their headers, libraries, symbol files and each other
// relative to where they are (bin/../lib/...). Linux: /proc/self/exe;
// macOS: _NSGetExecutablePath; FreeBSD: sysctl KERN_PROC_PATHNAME;
// otherwise argv[0], looked up on PATH when it has no '/' (a tool started
// from PATH). Symbolic links resolved, as by /proc/self/exe.
#pragma once

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#if defined(__APPLE__)
// (not <mach-o/dyld.h>: its <mach-o/loader.h> #defines SEG_TEXT / SEG_DATA,
// which clash with O45Segment's names, #503)
#include <cstdint>
extern "C" int _NSGetExecutablePath(char* buf, uint32_t* bufsize);
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace selfpath {

inline std::string resolved(const std::string& p) {
    char* r = realpath(p.c_str(), nullptr);
    if (!r) return "";
    std::string s = r;
    free(r);
    return s;
}

// The program's path, "" when not found
inline std::string exePath(const char* argv0 = nullptr) {
#if defined(__linux__)
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n > 0) { buf[n] = 0; return buf; }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if (size && _NSGetExecutablePath(&buf[0], &size) == 0) {
        std::string s = resolved(buf.c_str());
        if (!s.empty()) return s;
    }
#elif defined(__FreeBSD__)
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    char buf[4096];
    size_t len = sizeof buf;
    if (sysctl(mib, 4, buf, &len, nullptr, 0) == 0 && len > 0) return buf;
#endif
    if (!argv0 || !*argv0) return "";
    std::string a = argv0;
    if (a.find('/') != std::string::npos) return resolved(a);
    // started from PATH: the first directory with an executable of that name
    const char* path = getenv("PATH");
    std::string ps = path ? path : "";
    size_t at = 0;
    while (at <= ps.size()) {
        size_t e = ps.find(':', at);
        if (e == std::string::npos) e = ps.size();
        std::string dir = ps.substr(at, e - at);
        if (dir.empty()) dir = ".";
        std::string cand = dir + "/" + a;
        struct stat st;
        if (stat(cand.c_str(), &st) == 0 && S_ISREG(st.st_mode) && access(cand.c_str(), X_OK) == 0)
            return resolved(cand);
        at = e + 1;
    }
    return "";
}

// The program's directory with a trailing '/', "" when not found
inline std::string exeDir(const char* argv0 = nullptr) {
    std::string p = exePath(argv0);
    size_t sep = p.find_last_of('/');
    return sep == std::string::npos ? "" : p.substr(0, sep + 1);
}

} // namespace selfpath
