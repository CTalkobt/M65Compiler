#pragma once
// cc45's temporary files (#367): a private directory ($TMPDIR, else /tmp,
// from mkdtemp), removed at exit and on SIGINT/SIGTERM/SIGHUP (the signal
// then raised again, so make sees the interruption), and the ca45 runs cc45
// starts itself, which are stopped with it.
#include <string>
#include <vector>

namespace cctemp {

// <dir>/<name> in the private directory (made on the first call)
std::string path(const std::string& name);

// Runs argv[0] (a path, or found on PATH) with argv (no shell), stdout and stderr to /dev/null, and
// waits: its exit status, or -1. The child is signalled when cc45 is
// interrupted (and, on Linux, when cc45 dies).
int run(const std::vector<std::string>& argv);

// Removes the directory now (also done at exit)
void cleanup();

}  // namespace cctemp
