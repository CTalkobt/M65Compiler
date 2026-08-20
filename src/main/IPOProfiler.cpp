#include "IPOProfiler.hpp"
#include <iostream>
#include <map>
#include <set>

void IPOProfiler::recordFunctionDefinition(
    const std::string& functionName,
    int paramCount,
    int localVarCount,
    bool hasNoCalls,
    bool isLeafCandidate,
    const std::string& moduleName) {

    // Ensure consistent naming: add "_" prefix if not already present
    // This matches the naming used in recordFunctionCall
    std::string normalizedName = (functionName.empty() || functionName[0] != '_')
        ? ("_" + functionName)
        : functionName;

    currentModule_ = moduleName;

    // Create function build state
    FunctionBuildState state;
    state.name = normalizedName;
    state.paramCount = paramCount;
    state.localVarCount = localVarCount;
    state.hasNoCalls = hasNoCalls;
    state.isLeafCandidate = isLeafCandidate;

    buildStates_[normalizedName] = state;
}

void IPOProfiler::recordFunctionCall(
    const std::string& callerId,
    const std::string& calleeName,
    int argumentCount,
    bool hasConstantArgs) {

    // Record call in caller's build state
    auto it = buildStates_.find(callerId);
    if (it != buildStates_.end()) {
        it->second.hasNoCalls = false;  // Mark as having calls

        // Record call site information
        CallAnalysisInfo callInfo;
        callInfo.callerId = callerId;
        callInfo.calleeName = calleeName;
        callInfo.argumentCount = argumentCount;
        callInfo.hasConstantArgs = hasConstantArgs;
        callInfo.inLoop = false;  // Will be determined later if needed
        callInfo.loopNestDepth = 0;

        it->second.callSites.push_back(callInfo);
    }

    // Also ensure callee exists in buildStates_ so we can track incoming calls
    if (buildStates_.find(calleeName) == buildStates_.end()) {
        FunctionBuildState state;
        state.name = calleeName;
        buildStates_[calleeName] = state;
    }
}

void IPOProfiler::finalizeProfiles() {
    // Convert build states to function profiles in database
    auto& database = GlobalFunctionDatabase::instance();

    // First pass: build a map of incoming calls for each function
    std::map<std::string, std::set<std::string>> incomingCallsPerFunction;
    for (auto& [callerId, state] : buildStates_) {
        for (const auto& callInfo : state.callSites) {
            incomingCallsPerFunction[callInfo.calleeName].insert(callerId);
        }
    }

    // Second pass: create profiles with correct call site counts
    for (auto& [funcName, state] : buildStates_) {
        GlobalFunctionProfile profile;
        profile.name = funcName;
        profile.module = currentModule_;

        // Estimate code size based on heuristics
        // Base: 10 bytes per parameter, 5 bytes per local, 20 bytes base
        profile.codeSize = 20 + (state.paramCount * 10) + (state.localVarCount * 5);
        profile.codeSize += state.callSites.size() * 10;  // Call overhead (for calls made BY this function)

        profile.isLeaf = state.hasNoCalls && (state.localVarCount == 0);

        // Populate call sites vector with incoming calls to this function
        // Each caller that calls this function creates a call site entry
        const auto& callers = incomingCallsPerFunction[funcName];
        for (const auto& callerId : callers) {
            CallSite site;
            site.callerId = callerId;
            site.lineNumber = 0;  // Not tracked at this stage
            site.isLoopNested = false;  // Not tracked at this stage
            site.frequency = 1;  // Default: once
            profile.callSites.push_back(site);
        }

        // Also set totalCallSites for backward compatibility
        profile.totalCallSites = callers.size();
        profile.hasExternalCaller = true;  // Conservative default
        profile.isDeadCode = false;

        // Add to database
        database.addFunctionProfile(profile);
    }
}
