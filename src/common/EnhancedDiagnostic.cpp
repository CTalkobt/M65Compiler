/* EnhancedDiagnostic.cpp — Enhanced Error Messages Implementation
 *
 * Provides context-rich error reporting with suggestions.
 */

#include "EnhancedDiagnostic.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace cc45 {

/**
 * Levenshtein distance for typo detection
 */
static int levenshteinDistance(const std::string& s1, const std::string& s2) {
    size_t len1 = s1.size();
    size_t len2 = s2.size();

    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; i++) d[i][0] = i;
    for (size_t i = 0; i <= len2; i++) d[0][i] = i;

    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1,
                               d[i - 1][j - 1] + cost});
        }
    }

    return d[len1][len2];
}

/**
 * Format error for display
 */
std::string EnhancedDiagnostic::format() const {
    std::ostringstream oss;

    /* Location: file:line:column: */
    oss << location_.filename << ":"
        << location_.line << ":"
        << location_.column << ": ";

    /* Severity and message */
    oss << DiagnosticCategories::categoryName(severity_) << ": "
        << message_;

    return oss.str();
}

/**
 * Format error with source context
 */
std::string EnhancedDiagnostic::formatWithContext() const {
    std::ostringstream oss;

    /* Header line */
    oss << format() << "\n";

    /* Source context lines */
    if (!sourceLines_.empty() && errorLineIndex_ >= 0) {
        int startLine = std::max(0, errorLineIndex_ - 1);
        int endLine = std::min((int)sourceLines_.size() - 1, errorLineIndex_ + 1);

        for (int i = startLine; i <= endLine; i++) {
            int actualLine = (i - errorLineIndex_) + location_.line;

            /* Line number + content */
            if (i < (int)sourceLines_.size()) {
                oss << " " << actualLine << " | " << sourceLines_[i] << "\n";

                /* Error indicator for main line */
                if (i == errorLineIndex_) {
                    oss << "   | ";
                    for (int j = 0; j < location_.column - 1; j++) oss << " ";
                    oss << "^\n";
                    if (location_.endColumn > location_.column) {
                        oss << "   | ";
                        for (int j = 0; j < location_.column - 1; j++) oss << " ";
                        for (int j = location_.column; j < location_.endColumn; j++) {
                            oss << "~";
                        }
                        oss << "\n";
                    }
                }
            }
        }
    }

    /* Suggestions */
    if (!suggestions_.empty()) {
        oss << "\nSuggestions:\n";
        for (const auto& sugg : suggestions_) {
            oss << "  • " << sugg.message;
            if (!sugg.replacement.empty()) {
                oss << " (use: " << sugg.replacement << ")";
            }
            oss << "\n";
        }
    }

    /* Related locations */
    if (!relatedLocations_.empty()) {
        oss << "\nRelated locations:\n";
        for (const auto& [msg, loc] : relatedLocations_) {
            oss << "  " << loc.filename << ":" << loc.line << ": " << msg << "\n";
        }
    }

    return oss.str();
}

/**
 * Suggestion builders
 */
namespace suggestions {

ErrorSuggestion typoSuggestion(const std::string& undefined,
                               const std::string& suggested) {
    std::string msg = "Did you mean '" + suggested + "'?";
    ErrorSuggestion sugg(msg, true);
    sugg.replacement = suggested;
    return sugg;
}

ErrorSuggestion typeMismatchSuggestion(const std::string& expected,
                                       const std::string& got) {
    return ErrorSuggestion(
        "Type mismatch: expected '" + expected + "' but got '" + got + "'");
}

ErrorSuggestion castSuggestion(const std::string& type) {
    return ErrorSuggestion(
        "Use explicit cast: (" + type + ")expression", true);
}

ErrorSuggestion missingSemicolonSuggestion() {
    return ErrorSuggestion("Did you forget a semicolon?", true);
}

ErrorSuggestion assignmentVsComparisonSuggestion() {
    return ErrorSuggestion("Did you mean == instead of =?", true);
}

ErrorSuggestion missingReturnSuggestion(const std::string& functionName) {
    return ErrorSuggestion(
        "Function '" + functionName + "' should have a return statement");
}

ErrorSuggestion unreachableCodeSuggestion() {
    return ErrorSuggestion("This code is unreachable", true);
}

ErrorSuggestion implicitConversionSuggestion(const std::string& from,
                                             const std::string& to) {
    return ErrorSuggestion(
        "Implicit conversion from '" + from + "' to '" + to + "'");
}

} // namespace suggestions

/**
 * Category names
 */
const char* DiagnosticCategories::categoryName(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Note:
            return "note";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Fatal:
            return "fatal error";
    }
    return "unknown";
}

const char* DiagnosticCategories::categoryColor(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Note:
            return "\033[36m";  /* Cyan */
        case DiagnosticSeverity::Warning:
            return "\033[33m";  /* Yellow */
        case DiagnosticSeverity::Error:
            return "\033[31m";  /* Red */
        case DiagnosticSeverity::Fatal:
            return "\033[35m";  /* Magenta */
    }
    return "";
}

} // namespace cc45
