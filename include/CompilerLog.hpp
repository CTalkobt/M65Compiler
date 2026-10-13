// CompilerLog.hpp — cc45's diagnostic verbosity (-v, -vv)
//
// cc45 prints only errors and warnings by default; -v adds informational
// notes (SAC parameter thunks, devirtualized calls) and the summaries of
// the ca45/ln45 runs of a one-step build.
#pragma once

extern int g_cc45Verbosity;   // 0 (default), 1 (-v), 2+ (-vv ...)
