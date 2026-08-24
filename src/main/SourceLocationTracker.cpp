#include "SourceLocationTracker.hpp"

// Inline implementation - most methods are defined in header
// This file exists for potential future expansion or specialized implementations

// Helper to convert SourceLocation to string for debugging
std::string sourceLocationToString(const SourceLocationTracker::SourceLocation& loc) {
    if (!loc.isValid()) {
        return "<invalid>";
    }
    return loc.filename + ":" + std::to_string(loc.line) + ":" + std::to_string(loc.column);
}
