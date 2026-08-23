#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>

// Symbol suggestion engine using Levenshtein distance for fuzzy matching
class SymbolSuggester {
public:
    // Initialize with available symbols
    void addSymbol(const std::string& symbol);
    void addSymbols(const std::vector<std::string>& symbols);

    // Find suggestions for a misspelled symbol
    // Returns up to maxSuggestions candidates with similarity >= minSimilarity (0-100)
    std::vector<std::string> suggest(const std::string& misspelled,
                                     int maxSuggestions = 3,
                                     int minSimilarity = 60) const;

    // Get best single suggestion (or empty string if none found)
    std::string suggestBest(const std::string& misspelled, int minSimilarity = 60) const;

    // Clear all symbols
    void clear() { symbols_.clear(); }

    // Get all stored symbols
    const std::vector<std::string>& getSymbols() const { return symbols_; }

private:
    std::vector<std::string> symbols_;

    // Levenshtein distance: measures edit distance between two strings (0-100%)
    // Returns similarity as 0-100, where 100 = identical
    int levenshteinSimilarity(const std::string& s1, const std::string& s2) const;
};
