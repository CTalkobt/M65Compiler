#pragma once

#include "AST.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Captured variable bindings from a successful pattern match.
// Keys are idiom-specific, e.g. "accum"/"array"/"idx" for sum-reduction,
// "array"/"target"/"idx"/"result" for search.
struct IdiomMatch {
    std::unordered_map<std::string, std::string> vars;
    int intValue = 0;  // for memset's constant value
};

// Abstract base class for loop idiom patterns.
// Each idiom detector/transformer pair is encapsulated as a subclass.
class LoopIdiom {
public:
    virtual ~LoopIdiom() = default;

    // Return a human-readable name for this idiom (e.g., "memcpy", "sum-reduction").
    virtual const char* name() const = 0;

    // Detect whether the given loop matches this idiom's pattern.
    // If it does, populate 'match' with the captured variable bindings and return true.
    // If it doesn't match, return false.
    virtual bool detect(const ForStatement& loop, IdiomMatch& match) const = 0;

    // Transform the loop to a call to the idiom's library routine.
    // Assumes the loop was successfully detected by this idiom.
    // Returns an AST node (typically an ExpressionStatement wrapping a FunctionCall)
    // to replace the loop, or nullptr on error.
    virtual std::unique_ptr<Statement> transform(const ForStatement& loop, const IdiomMatch& match) const = 0;
};

// Singleton registry of loop idioms.
// Manages a set of registered idiom detectors and provides a unified interface
// to try matching and transforming a loop against all known idioms.
class LoopIdiomRegistry {
public:
    // Get the global singleton instance.
    static LoopIdiomRegistry& instance();

    // Try every registered idiom in order until one matches and transforms successfully.
    // Returns the transformed AST node (to replace the loop) if any idiom matches,
    // or nullptr if none match.
    std::unique_ptr<Statement> tryTransform(const ForStatement& loop) const;

private:
    // Private constructor: registers all built-in idioms.
    LoopIdiomRegistry();

    std::vector<std::unique_ptr<LoopIdiom>> idioms_;
};
