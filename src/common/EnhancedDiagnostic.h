/* EnhancedDiagnostic.h — Improved Error Messages with Context
 *
 * Provides rich error context, column positions, and suggestions.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cc45 {

/**
 * Error severity levels
 */
enum class DiagnosticSeverity {
    Note,       // Informational
    Warning,    // Non-fatal issue
    Error,      // Compilation error
    Fatal       // Unrecoverable error
};

/**
 * Source location with file, line, column
 */
struct SourceLocation {
    std::string filename;
    int line = 0;       // 1-based
    int column = 0;     // 1-based
    int endColumn = 0;  // For multi-column errors

    SourceLocation() = default;
    SourceLocation(const std::string& f, int l, int c = 0, int ec = 0)
        : filename(f), line(l), column(c), endColumn(ec) {}

    bool isValid() const { return line > 0; }
};

/**
 * Suggestion for fixing an error
 */
struct ErrorSuggestion {
    std::string message;           // "Did you mean...?"
    std::string replacement = "";  // Optional replacement text
    bool isAutoFixable = false;    // Can be automatically applied

    ErrorSuggestion(const std::string& msg, bool fixable = false)
        : message(msg), isAutoFixable(fixable) {}
};

/**
 * Enhanced diagnostic with context and suggestions
 */
class EnhancedDiagnostic {
public:
    EnhancedDiagnostic(DiagnosticSeverity severity, const std::string& message,
                       const SourceLocation& location)
        : severity_(severity), message_(message), location_(location) {}

    /* Severity and message */
    DiagnosticSeverity severity() const { return severity_; }
    const std::string& message() const { return message_; }
    const SourceLocation& location() const { return location_; }

    /* Error suggestions */
    void addSuggestion(const ErrorSuggestion& suggestion) {
        suggestions_.push_back(suggestion);
    }

    const std::vector<ErrorSuggestion>& suggestions() const {
        return suggestions_;
    }

    /* Source context lines */
    void setSourceContext(const std::vector<std::string>& lines,
                         int errorLineIndex) {
        sourceLines_ = lines;
        errorLineIndex_ = errorLineIndex;
    }

    const std::vector<std::string>& sourceLines() const {
        return sourceLines_;
    }

    int errorLineIndex() const { return errorLineIndex_; }

    /* Related errors/locations */
    void addRelatedLocation(const std::string& message,
                           const SourceLocation& location) {
        relatedLocations_.push_back({message, location});
    }

    const std::vector<std::pair<std::string, SourceLocation>>&
    relatedLocations() const {
        return relatedLocations_;
    }

    /* Format for display */
    std::string format() const;
    std::string formatWithContext() const;

private:
    DiagnosticSeverity severity_;
    std::string message_;
    SourceLocation location_;
    std::vector<ErrorSuggestion> suggestions_;
    std::vector<std::string> sourceLines_;
    int errorLineIndex_ = -1;
    std::vector<std::pair<std::string, SourceLocation>> relatedLocations_;
};

/**
 * Error suggestion builders
 */
namespace suggestions {
    /* Undefined identifier suggestions */
    ErrorSuggestion typoSuggestion(const std::string& undefined,
                                   const std::string& suggested);

    /* Type mismatch suggestions */
    ErrorSuggestion typeMismatchSuggestion(const std::string& expected,
                                           const std::string& got);
    ErrorSuggestion castSuggestion(const std::string& type);

    /* Common mistakes */
    ErrorSuggestion missingSemicolonSuggestion();
    ErrorSuggestion assignmentVsComparisonSuggestion();
    ErrorSuggestion missingReturnSuggestion(const std::string& functionName);
    ErrorSuggestion unreachableCodeSuggestion();

    /* Type conversion */
    ErrorSuggestion implicitConversionSuggestion(const std::string& from,
                                                 const std::string& to);
}

/**
 * Error category helpers
 */
class DiagnosticCategories {
public:
    static const char* categoryName(DiagnosticSeverity severity);
    static const char* categoryColor(DiagnosticSeverity severity);
};

} // namespace cc45
