#pragma once
#include <string>

// Static-allocation locals never live at the same time share their storage:
// the assembly text with each SAC function's block (_f__local_<id>: .res n)
// laid out again by a liveness of its code (#324)
std::string shareSacSlots(const std::string& text);
