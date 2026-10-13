// IRCodeGenPhases.cpp — phase 4.2's IPO hints from GlobalFunctionDatabase (never collected)
//
// Moved out of IRCodeGen.cpp: nothing in the tools calls these, so
// they are built into lib45-phases.a, which no tool links.

#include "CompilerLog.hpp"
#include "IRCodeGen.hpp"
#include "OpEffect.hpp"
#include "GlobalFunctionDatabase.hpp"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>
#include "CBMFloat.hpp"

// Phase 4.2: Collect inter-TU optimization hints from GlobalFunctionDatabase
O45IPOHints IRCodeGen::collectIPOHints() const {
    O45IPOHints hints;
    hints.version = O45_IPO_HINTS_VERSION;

    auto& db = GlobalFunctionDatabase::instance();

    // Iterate through all function profiles in the database
    for (const auto& profile : db.getExternalFunctions()) {
        if (!profile) continue;

        O45IPOFunctionHints funcHints;
        funcHints.functionName = profile->name;
        funcHints.callCount = profile->totalCallSites;
        funcHints.estimatedCodeSize = profile->codeSize;
        funcHints.flags = 0;

        // Set leaf flag if function doesn't call others
        if (profile->isLeaf) {
            funcHints.flags |= FUNC_FLAG_LEAF;
        }

        // Count external call sites
        funcHints.externalCallCount = 0;
        for (const auto& callSite : profile->callSites) {
            // Simplified: count non-local callers as external
            // More sophisticated cross-module detection done at link time
            funcHints.externalCallCount++;
        }

        // Collect specialization patterns from call sites (future: Phase 52)
        // For now, reserve the field for future use

        hints.functions.push_back(funcHints);
    }

    return hints;
}

