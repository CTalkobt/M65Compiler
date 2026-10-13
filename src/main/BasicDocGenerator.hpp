#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <map>

struct DocumentationEntry {
    std::string name;
    std::string type;  // "label", "subroutine", "section"
    int lineNumber;
    std::vector<std::string> comments;
};

class BasicDocGenerator {
public:
    // Generate documentation from BASIC source
    std::string generateMarkdown(const std::string& source, const std::string& title = "BASIC Program");

    // The BASIC line number of each label (upper case), from the parser;
    // without it, lines are counted 10, 20, ... in the source given
    void setLabelLines(const std::unordered_map<std::string, uint16_t>& m) { labelLines = m; }

    // Get extracted documentation entries
    const std::vector<DocumentationEntry>& getEntries() const { return entries; }

private:
    std::vector<DocumentationEntry> entries;
    std::unordered_map<std::string, uint16_t> labelLines;

    void extract(const std::string& source, bool useLabels);
    std::string renderMarkdown(const std::string& title) const;
};
