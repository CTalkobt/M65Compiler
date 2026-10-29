#include "CcTemp.hpp"
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace cctemp {

// The directory and the running children, as the signal handler reads them:
// fixed storage, no allocation
static char dir_[4096];
static std::atomic<bool> haveDir_{false};
static constexpr int MAX_CHILDREN = 64;
static std::atomic<pid_t> children_[MAX_CHILDREN];
static std::mutex mutex_;

// Removes the directory's files and the directory (only cc45's own files
// are in it: no subdirectories)
static void removeDir() {
    if (!haveDir_.exchange(false)) return;
    if (DIR* d = opendir(dir_)) {
        char p[sizeof dir_ + 256];
        while (struct dirent* e = readdir(d)) {
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
            size_t n = strlen(dir_), m = strlen(e->d_name);
            if (n + 1 + m >= sizeof p) continue;
            memcpy(p, dir_, n); p[n] = '/'; memcpy(p + n + 1, e->d_name, m + 1);
            unlink(p);
        }
        closedir(d);
    }
    rmdir(dir_);
}

static void onSignal(int sig) {
    for (auto& c : children_) {
        pid_t pid = c.load();
        if (pid > 0) kill(pid, SIGTERM);
    }
    for (auto& c : children_) {
        pid_t pid = c.exchange(0);
        if (pid > 0) waitpid(pid, nullptr, 0);
    }
    removeDir();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void atExit() { removeDir(); }

std::string path(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!haveDir_) {
        const char* t = getenv("TMPDIR");
        std::string base = t && *t ? t : "/tmp";
        std::string tmpl = base + "/cc45-XXXXXX";
        if (tmpl.size() >= sizeof dir_) tmpl = "/tmp/cc45-XXXXXX";
        memcpy(dir_, tmpl.c_str(), tmpl.size() + 1);
        if (!mkdtemp(dir_)) {
            // (no temp directory: the current one, as before)
            return name;
        }
        haveDir_ = true;
        atexit(atExit);
        for (int sig : {SIGINT, SIGTERM, SIGHUP}) {
            struct sigaction sa;
            memset(&sa, 0, sizeof sa);
            sa.sa_handler = onSignal;
            sigemptyset(&sa.sa_mask);
            sigaction(sig, &sa, nullptr);
        }
    }
    return std::string(dir_) + "/" + name;
}

int run(const std::vector<std::string>& argv) {
    if (argv.empty()) return -1;
    std::vector<char*> args;
    for (const auto& a : argv) args.push_back(const_cast<char*>(a.c_str()));
    args.push_back(nullptr);
    pid_t parent = getpid();
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
#ifdef __linux__
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() != parent) _exit(127);   // (cc45 died before the prctl)
#else
        (void)parent;
#endif
        int null = open("/dev/null", O_WRONLY);
        if (null >= 0) { dup2(null, 1); dup2(null, 2); close(null); }
        execvp(args[0], args.data());
        _exit(127);
    }
    int slot = -1;
    for (int k = 0; k < MAX_CHILDREN && slot < 0; k++) {
        pid_t none = 0;
        if (children_[k].compare_exchange_strong(none, pid)) slot = k;
    }
    int status = 0;
    pid_t r;
    do r = waitpid(pid, &status, 0); while (r < 0 && errno == EINTR);
    if (slot >= 0) children_[slot] = 0;
    if (r < 0) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void cleanup() { removeDir(); }

}  // namespace cctemp
