#include "SymbolSuggester.hpp"
#include <cmath>
#include <cctype>

void SymbolSuggester::addSymbol(const std::string& symbol) {
    // Avoid duplicates
    if (std::find(symbols_.begin(), symbols_.end(), symbol) == symbols_.end()) {
        symbols_.push_back(symbol);
    }
}

void SymbolSuggester::addSymbols(const std::vector<std::string>& symbols) {
    for (const auto& sym : symbols) {
        addSymbol(sym);
    }
}

// Levenshtein distance using dynamic programming
// Returns similarity as 0-100% (100 = identical, 0 = completely different)
int SymbolSuggester::levenshteinSimilarity(const std::string& s1, const std::string& s2) const {
    // Case-insensitive comparison for fuzzy matching
    std::string a, b;
    a.reserve(s1.size());
    b.reserve(s2.size());
    for (char c : s1) a += std::tolower(c);
    for (char c : s2) b += std::tolower(c);

    size_t m = a.length();
    size_t n = b.length();

    // Quick wins: identical or one is empty
    if (a == b) return 100;
    if (m == 0 || n == 0) return 0;

    // dp[i][j] = edit distance between a[0..i-1] and b[0..j-1]
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    // Initialize: distance from empty string
    for (size_t i = 0; i <= m; i++) dp[i][0] = i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = j;

    // Fill DP table
    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,      // deletion
                dp[i][j - 1] + 1,      // insertion
                dp[i - 1][j - 1] + cost // substitution
            });
        }
    }

    int distance = dp[m][n];
    int maxLen = std::max(m, n);

    // Convert distance to similarity percentage
    // Similarity = (1 - distance/maxLen) * 100
    int similarity = static_cast<int>(100.0 * (1.0 - static_cast<double>(distance) / maxLen));
    return std::max(0, similarity);
}

std::vector<std::string> SymbolSuggester::suggest(const std::string& misspelled,
                                                  int maxSuggestions,
                                                  int minSimilarity) const {
    // Collect candidates with similarity scores
    std::vector<std::pair<int, std::string>> candidates; // (similarity, symbol)

    for (const auto& symbol : symbols_) {
        int sim = levenshteinSimilarity(misspelled, symbol);
        if (sim >= minSimilarity) {
            candidates.push_back({sim, symbol});
        }
    }

    // Sort by similarity (descending), then by length (prefer shorter matches)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
            if (a.first != b.first) return a.first > b.first;  // Higher similarity first
            return a.second.length() < b.second.length();      // Shorter symbol
        });

    // Return top suggestions
    std::vector<std::string> result;
    for (int i = 0; i < maxSuggestions && i < (int)candidates.size(); i++) {
        result.push_back(candidates[i].second);
    }
    return result;
}

std::string SymbolSuggester::suggestBest(const std::string& misspelled, int minSimilarity) const {
    auto suggestions = suggest(misspelled, 1, minSimilarity);
    return suggestions.empty() ? "" : suggestions[0];
}
