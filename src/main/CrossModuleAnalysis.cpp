#include "CrossModuleAnalysis.hpp"
#include <sstream>
#include <iomanip>

namespace ipo {

// Format a call site pattern as a .callsite directive
// Example: .callsite _multiply_by_n paramMask=0x2 values=0x4,0x0,0x0,0x0
std::string CallSiteCollector::formatDirective(const CallSiteInfo& info) const {
    std::ostringstream oss;
    oss << ".callsite " << info.pattern.functionName;

    // Build parameter mask (bit i = 1 if param i is constant)
    uint32_t paramMask = 0;
    for (int idx : info.pattern.constantParamIndices) {
        if (idx >= 0 && idx < 32) {
            paramMask |= (1u << idx);
        }
    }
    oss << " paramMask=0x" << std::hex << paramMask << std::dec;

    // Emit constant values (max 4 for 45GS02 long returns)
    oss << " values=";
    for (size_t i = 0; i < info.pattern.constantValues.size(); i++) {
        if (i > 0) oss << ",";
        oss << "0x" << std::hex << info.pattern.constantValues[i] << std::dec;
    }

    // Source location for debugging
    if (info.sourceLine > 0) {
        oss << " line=" << info.sourceLine;
    }

    return oss.str();
}

} // namespace ipo
