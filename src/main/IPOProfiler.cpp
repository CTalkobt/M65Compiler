#include "IPOProfiler.hpp"
#include <iostream>

void IPOProfiler::recordFunctionDefinition(
    const std::string& functionName,
    int paramCount,
    int localVarCount,
    bool hasNoCalls,
    bool isLeafCandidate,
    const std::string& moduleName) {

    currentModule_ = moduleName;

    // Create function build state
    FunctionBuildState state;
    state.name = functionName;
    state.paramCount = paramCount;
    state.localVarCount = localVarCount;
    state.hasNoCalls = hasNoCalls;
    state.isLeafCandidate = isLeafCandidate;

    buildStates_[functionName] = state;
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
}

void IPOProfiler::finalizeProfiles() {
    // Convert build states to function profiles in database
    auto& database = GlobalFunctionDatabase::instance();
    for (auto& [funcName, state] : buildStates_) {
        GlobalFunctionProfile profile;
        profile.name = funcName;
        profile.module = currentModule_;

        // Estimate code size based on heuristics
        // Base: 10 bytes per parameter, 5 bytes per local, 20 bytes base
        profile.codeSize = 20 + (state.paramCount * 10) + (state.localVarCount * 5);
        profile.codeSize += state.callSites.size() * 10;  // Call overhead

        profile.isLeaf = state.hasNoCalls && (state.localVarCount == 0);
        profile.totalCallSites = state.callSites.size();
        profile.hasExternalCaller = true;  // Conservative default
        profile.isDeadCode = false;

        // Add to database
        database.addFunctionProfile(profile);
    }
}
